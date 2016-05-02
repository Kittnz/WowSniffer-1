#include "stdafx.h"
#include "Functions.h"
#include <sstream>

MODULEENTRY32 GetMainModule()
{
    MODULEENTRY32 module;
    module.dwSize = sizeof(MODULEENTRY32); 
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, GetCurrentProcessId());
    Module32First(snapshot, &module);
    CloseHandle(snapshot);
    return module;
}

MODULEENTRY32W GetMainModuleW()
{
    MODULEENTRY32W module;
    module.dwSize = sizeof(MODULEENTRY32W); 
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, GetCurrentProcessId());
    Module32FirstW(snapshot, &module);
    CloseHandle(snapshot);
    return module;
}

int32 GetMainModuleAddress()
{
    return (int32)GetMainModule().modBaseAddr;
}

int32 GetMainModuleSize()
{
    return GetMainModule().modBaseSize;
}

void GetBuildInfo(uint32* expVersion, uint32* majorVersion, uint32* minorVersion, uint32* build)
{
    if (expVersion)
        *expVersion = 0;

    if (majorVersion)
        *majorVersion = 0;

    if (minorVersion)
        *minorVersion = 0;

    if (build)
        *build = 0;

    VS_FIXEDFILEINFO* fixedFileInfo;
    WCHAR* path = GetMainModuleW().szExePath;
    DWORD size = GetFileVersionInfoSizeW(path, NULL);

    if (!size)
        return;

    uint8* buffer = new uint8[size];

    if (!GetFileVersionInfoW(path, 0, size, buffer))
        return;

    uint32 fixedFileInfoSize;
    if (!VerQueryValue(buffer, "\\", (void**)&fixedFileInfo, &fixedFileInfoSize))
        return;
        
    if (expVersion)
        *expVersion = HIWORD(fixedFileInfo->dwFileVersionMS);

    if (majorVersion)
        *majorVersion = LOWORD(fixedFileInfo->dwFileVersionMS);

    if (minorVersion)
        *minorVersion = HIWORD(fixedFileInfo->dwFileVersionLS);

    if (build)
        *build = LOWORD(fixedFileInfo->dwFileVersionLS);
}

uint8 Locales[][4]
{
    { 'e', 'n', 'U', 'S' },
    { 'k', 'o', 'K', 'R' },
    { 'f', 'r', 'F', 'R' },
    { 'd', 'e', 'D', 'E' },
    { 'z', 'h', 'C', 'N' },
    { 'z', 'h', 'T', 'W' },
    { 'e', 's', 'E', 'S' },
    { 'e', 's', 'M', 'X' },
    { 'r', 'u', 'R', 'U' },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 'p', 't', 'B', 'R', },
    { 'i', 't', 'I', 'T', }
};

uint32 GetLocaleName()
{
    // Find LUA_GetLocale function the client
    uint8 buffer[] = { 0x55, 0x8b, 0xec, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xff, 0x75, 0x08, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x33, 0xc0, 0x83, 0xc4, 0x0c, 0x40, 0x5d, 0xc3 };
    std::string patternMask = "xxxx????xx????xxxxx????xxxxxxxx";

    // Find GetLocale LUA function, get first call at 4th byte, which contains a function which returns dword_localeID
    uint8* address = FindPattern<uint8*>(buffer, sizeof(buffer), patternMask);
    assert(address && "GetLocale not found!");
   
    int32 relativeAddr = *(int32*)(address + 4);
    int32* localeID = *(int32**)(((int)(address + 3) + 5 + relativeAddr) + 1);
    return *(uint32*)Locales[*localeID];
}

bool FileExists(WCHAR* fileName)
{
    WIN32_FIND_DATAW fileData;
    HANDLE file = FindFirstFileW(fileName, &fileData);

    bool exists = file != INVALID_HANDLE_VALUE;

    if (exists)
        FindClose(file);

    return exists;
}

std::string GetFilenameWithoutExtension(WCHAR* path)
{
    WCHAR name[MAX_PATH];
    _wsplitpath(path, nullptr, nullptr, name, nullptr);

    char buffer[MAX_PATH];
    wcstombs(buffer, name, sizeof(buffer));

    return buffer;
}

std::wstring GetPathWithoutFilename(WCHAR* path)
{
    WCHAR folder[4096];
    WCHAR drive[_MAX_DRIVE];
    _wsplitpath(path, drive, folder, nullptr, nullptr);

    std::wstring output = drive;
    output += folder;
    return output;
}
