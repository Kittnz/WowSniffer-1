#include "stdafx.h"
#include "PktFile.h"
#include "Functions.h"
#include "BuildConfig.h"
#include "Opcodes.h"
#include "TableHashes.h"

PktFile* pktFile;

uint8* processMessageOldCode = nullptr;
uint8* send2OldCode = nullptr;
uint8* processMessageHookCode = nullptr;
uint8* send2HookCode = nullptr;

BuildConfig config;
std::wstring SnifferFolder;
WCHAR SnifferModulePath[4096];

void AllocNewConsole()
{
    if (!AllocConsole())
        return;
        
    freopen("CONIN$", "r", stdin); 
    freopen("CONOUT$", "w", stdout); 
    freopen("CONOUT$", "w", stderr); 
}

struct ProcessMessageArgs
{
    uint32 Unused0;
    uint32 Unused1;
    uint32 Unused2;
    uint32 Opcode;
    CDataStore* Packet;

};

struct Send2Args
{
    uint32 Unused0;
    uint32 Unused1;
    CDataStore* Packet;
    uint32 ConnectionIndex;
};

void __cdecl CMSGHook(Send2Args* args)
{
    PktFile::Packet packet(0, args->ConnectionIndex, args->Packet, config.Send2OpcodeSize, config.Send2OpcodeOffset);
    pktFile->AppendPacket(&packet);
}

void __cdecl SMSGHook(ProcessMessageArgs* args)
{
    PktFile::Packet packet(1, 0, args->Packet, config.ProcessMessageOpcodeSize, config.ProcessMessageOpcodeOffset);
    pktFile->AppendPacket(&packet);
}

uint8* Hook(int32 Address, uint32 OriginalCodeLen, uint8* PreCallCode, uint32 PreCallCodeLen, uint8* PostCallCode, uint32 PostCallCodeLen, int32 FunctionToCallAddress, bool PreserverRegistersAfterCall, uint8* backupOldCode);

enum Instructions
{
    Push        = 0x06,
    Pop         = 0x07,
    Push_Esp    = 0x54,
    Push_Ebp    = 0x55,
    Pop_Esp     = 0x5C,
    Pop_Ebp     = 0x5D,
    PushAd      = 0x60,
    PopAd       = 0x61,
    Nop         = 0x90,
    PushFd      = 0x9C,
    PopFd       = 0x9D,
    Retn        = 0xC3,
    Call_4Byte  = 0xE8,
    Jmp_4Byte   = 0xE9,
};

void Detach()
{
    // Unload WowSniffer.dll
    // FreeLibrary(SnifferModulePath);
}

void* CDataStoreVFTable = reinterpret_cast<void*>(0xDFFD50);
typedef int (__stdcall *pFunc)(CDataStore*);
pFunc sendInternal = reinterpret_cast<pFunc>(0x485CE9);

void WINAPI main(void* args);

void DestroyHook()
{
    // Restore old code
    if (processMessageOldCode)
    {
        DWORD originalPermissions = 0;
        VirtualProtect((void*)config.GetProcessMessageAddress(), config.ProcessMessageHookLen, PAGE_EXECUTE_READWRITE, &originalPermissions);
        memcpy((void*)config.GetProcessMessageAddress(), processMessageOldCode, config.ProcessMessageHookLen);
        VirtualProtect((void*)config.GetProcessMessageAddress(), config.ProcessMessageHookLen, originalPermissions, &originalPermissions);
        delete processMessageOldCode;
        delete processMessageHookCode;
    }

    if (send2OldCode)
    {
        DWORD originalPermissions = 0;
        VirtualProtect((void*)config.GetSend2Address(), config.Send2HookLen, PAGE_EXECUTE_READWRITE, &originalPermissions);
        memcpy((void*)config.GetSend2Address(), send2OldCode, config.Send2HookLen);
        VirtualProtect((void*)config.GetSend2Address(), config.Send2HookLen, originalPermissions, &originalPermissions);
        delete send2OldCode;
        delete send2HookCode;
    }

    delete config.Send2Pattern;
    delete config.ProcessMessagePattern;
    delete pktFile;

    FreeConsole();
}

uint8* Hook(int32 Address, uint32 OriginalCodeLen, uint8* PreCallCode, uint32 PreCallCodeLen, uint8* PostCallCode, uint32 PostCallCodeLen, int32 FunctionToCallAddress, bool PreserveRegistersAfterCall, uint8* backupOldCode)
{
    uint32 hookLen = OriginalCodeLen + PreCallCodeLen + PostCallCodeLen + 5 /*Call instruction size*/ + 1 /*retn*/ + (PreserveRegistersAfterCall ? 4 : 0) /* PushAd, PushFd, PopAd, Popfd */;
    uint8* code = new uint8[hookLen];
    uint32 currentOffset = 0;
    uint8* FunctionCodePointer = (uint8*)Address;

    // Push Registers and their flags to restore them after the call
    if (PreserveRegistersAfterCall)
    {
        code[currentOffset++] = Instructions::PushAd;
        code[currentOffset++] = Instructions::PushFd;
    }

    // Generate hooked Code
    // Copy over pre call code
    if (PreCallCodeLen)
    {
        memcpy(code + currentOffset, PreCallCode, PreCallCodeLen);
        currentOffset += PreCallCodeLen;
    }

    // Generate and insert function call
    int32 functionRelativePosition = FunctionToCallAddress - ((int32)code + currentOffset + 5); // 5 as for instruction size
    code[currentOffset++] = Instructions::Call_4Byte;
    memcpy(code + currentOffset, &functionRelativePosition, sizeof(int32));
    currentOffset += sizeof(int32);

    // Copy over post call code
    if (PostCallCodeLen)
    {
        memcpy(code + currentOffset, PostCallCode, PostCallCodeLen);
        currentOffset += PostCallCodeLen;
    }

    // Pop Registers and their flags to restore them if they changed during the function call
    if (PreserveRegistersAfterCall)
    {
        code[currentOffset++] = Instructions::PopFd;
        code[currentOffset++] = Instructions::PopAd;
    }

    // Add Original code
    memcpy(code + currentOffset, FunctionCodePointer, OriginalCodeLen);
    currentOffset += OriginalCodeLen;

    // Jump back to the execution of the detoured function
    code[currentOffset++] = Instructions::Retn;

    // Enable Execution permissions for the generated code
    DWORD originalPermissions = 0;
    VirtualProtect(code, currentOffset, PAGE_EXECUTE_READWRITE, &originalPermissions);

    // Generate Detour
    uint32 detourLen = max(OriginalCodeLen, 5);

    // Fix permissions - otherwise we get write errors
    originalPermissions = 0;
    VirtualProtect(FunctionCodePointer, detourLen, PAGE_EXECUTE_READWRITE, &originalPermissions);

    backupOldCode = new uint8[detourLen];
    memcpy(backupOldCode, FunctionCodePointer, detourLen);

    // Prevent Corruntion - Nop if bigger than detour code
    if (OriginalCodeLen > 5)
        memset(FunctionCodePointer, Instructions::Nop, OriginalCodeLen);

    // Call the generated code
    int32 detourAddress = (int32)code - (Address + 5);
    FunctionCodePointer[0] = Instructions::Call_4Byte;
    memcpy(FunctionCodePointer + 1, &detourAddress, sizeof(int32));

    // Restore permissions
    DWORD modifiedPermissions = 0;
    VirtualProtect(FunctionCodePointer, detourLen, originalPermissions, &modifiedPermissions);

    return code;
}

void SendPacket(CDataStore* store)
{
    store->Put<uint32>(store->Size, 0);
    store->SetRPos(4);
    sendInternal(store);
}

void RequestQuest(uint32 id)
{
    CDataStore data(Opcodes::CMSG_QUERY_QUEST_INFO, 6);
    data << uint32(1393);
    data << uint16(0);
    SendPacket(&data);
}

void RequestDB2Entry(TableHashes hash, uint32 id)
{
    CDataStore data(Opcodes::CMSG_DB_QUERY_BULK, 4 + 2 + 2 + 4);
    data << uint32(hash);
    data << uint8(0);
    data << uint8(1 << 3);
    data << uint16(0);      // guid
    data << uint32(id);
    SendPacket(&data);
}

void __stdcall main(void* args)
{
    // Allocate a console
    AllocNewConsole();

    // Generate new .pkt file
    pktFile = PktFile::Create();

    if (!pktFile)
    {
        Detach();
        return;
    }

    // Gets either "Wow", "WowB" or "WowT"
    auto wowBuildType = GetFilenameWithoutExtension(GetMainModuleW().szExePath);

    // Load the patterns and info for hooking
    uint32 build;
    GetBuildInfo(nullptr, nullptr, nullptr, &build);

    if (!config.Load(SnifferFolder, wowBuildType, build))
    {
        Detach();
        return;
    }

    // Wait untill Wow properly loads
    while (config.GetProcessMessageAddress() < 0xFF || config.GetSend2Address() < 0xFF)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    printf("ProcessMessageHook: 0x%X\nSend2Hook: 0x%X\n", config.GetProcessMessageAddress(), config.GetSend2Address());
    uint8 preHookCode[] = {Instructions::Push_Ebp};
    uint8 postHooKCode[] = {Instructions::Pop_Ebp};

    send2HookCode = Hook(config.GetProcessMessageAddress(), config.ProcessMessageHookLen, preHookCode, sizeof(preHookCode), postHooKCode, sizeof(postHooKCode), (int32)&SMSGHook, true, send2OldCode);
    processMessageHookCode = Hook(config.GetSend2Address(), config.Send2HookLen, preHookCode, sizeof(preHookCode), postHooKCode, sizeof(postHooKCode), (int32)&CMSGHook, true, processMessageOldCode);

    char line[256];
    while (gets(line))
    {
       /* for (auto i = 0u; i < 1000; ++i)
        {
            RequestDB2Entry(TableHashes::Bounty, i);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }*/
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
        case DLL_PROCESS_ATTACH:
        {
            GetModuleFileNameW(hModule, SnifferModulePath, 4096);
            SnifferFolder = GetPathWithoutFilename(SnifferModulePath);
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&main, nullptr, 0, 0);
            break;
        }
	    case DLL_PROCESS_DETACH:
            DestroyHook();
            break;
	    case DLL_THREAD_ATTACH:
	    case DLL_THREAD_DETACH:
		    break;
	}

	return TRUE;
}
