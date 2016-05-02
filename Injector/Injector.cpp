#include "stdafx.h"

#define CREATE_THREAD_ACCESS (PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ)

std::wstring SnifferDLLName = L"./WowSniffer.dll";

int GetProcessIDByName(std::wstring ProcessName)
{
    PROCESSENTRY32 entry;
    DWORD lastId = 0;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (lstrcmp(entry.szExeFile, ProcessName.c_str()) == 0)
            {
                lastId = entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return lastId;
}

bool Inject(int ProcessID, std::wstring DLLName)
{
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

    if (!process)
         return false;
         
    int stringLen = (DLLName.length() + 1) * sizeof(wchar_t);

    LPTHREAD_START_ROUTINE loadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    void* remoteString = (void*)VirtualAllocEx(process, NULL, stringLen, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(process, (void*)remoteString, DLLName.c_str(), stringLen, NULL);
    HANDLE remoteThread = CreateRemoteThread(process, NULL, NULL, loadLibraryW, remoteString, NULL, NULL);

    if (remoteThread)
        CloseHandle(remoteThread);
    CloseHandle(process);
    return true;
}


int _tmain(int argc, _TCHAR* argv[])
{
    std::wstring possibleNames[] = { L"Wow.exe", L"WowT.exe", L"WowB.exe" };

    for (int i = 0; i < sizeof(possibleNames) / sizeof(std::wstring); ++i)
    {
        int PID = GetProcessIDByName(possibleNames[i]);

        if (!PID)
            continue;

        wchar_t FullName[512];
        GetFullPathNameW(SnifferDLLName.c_str(), sizeof(FullName) / sizeof(wchar_t), FullName, nullptr);

        if (Inject(PID, FullName))
        {
            printf("Sucessfully Attached\n");
	        return 0;
        }
        else
        {
            printf("Error: Attach Failed\n");
	        return 0;
        }
    }

    printf("Error: Process not found!\n");
	return 0;
}
