#pragma once
#include "stdafx.h"

extern MODULEENTRY32 GetMainModule();
extern MODULEENTRY32W GetMainModuleW();
extern int32 GetMainModuleAddress();
extern int32 GetMainModuleSize();
extern void GetBuildInfo(uint32* exp, uint32* majorVersion, uint32* minorVersion, uint32* build);
extern uint32 GetLocaleName();
extern bool FileExists(WCHAR* fileName);
extern std::string GetFilenameWithoutExtension(WCHAR* path);
extern std::wstring GetPathWithoutFilename(WCHAR* path);

template <typename T>
T FindPattern(uint8* Buffer, int Len, uint8* Pattern, int32 PatternLen, std::string PatternMask)
{
    if (PatternLen != PatternMask.length())
        assert(false && "PatternLen and String Len did not match");

    for (int32 i = 0; i < Len; i++)
    {
        int32 matches = 0;

        if (i + PatternLen > Len - 1)
            break;

        for (int32 j = 0; j < PatternLen; j++)
        {
            if (PatternMask[j] == '?' || (Buffer[i + j] == Pattern[j]))
            {
                matches++;
                continue;
            }

            break;
        }

        if (matches == PatternLen)
            return (T)(i + (int32)Buffer);
    }

    return (T)NULL;
}

template <typename T>
T FindPattern(uint8* Pattern, int PatternLen, std::string PatternMask)
{
    return FindPattern<T>((uint8*)GetMainModuleAddress(), GetMainModuleSize(), Pattern, PatternLen, PatternMask);
}
