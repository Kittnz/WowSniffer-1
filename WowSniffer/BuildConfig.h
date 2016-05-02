#include "stdafx.h"
#include "Functions.h"
#include <unordered_map>

struct BuildConfig
{
    BuildConfig();

    bool Load(std::wstring folder, std::string wowBuildType, uint32 build);
    int32 GetProcessMessageAddress();
    int32 GetSend2Address();

    uint8* ProcessMessagePattern;
    uint32 ProcessMessagePatternLen;
    std::string ProcessMessagePatternMask;
    uint32 ProcessMessageHookOffset;
    uint32 ProcessMessageHookLen;
    uint32 ProcessMessageOpcodeSize;
    uint32 ProcessMessageOpcodeOffset;

    uint8* Send2Pattern;
    uint32 Send2PatternLen;
    std::string Send2PatternMask;
    uint32 Send2HookOffset;
    uint32 Send2HookLen;
    uint32 Send2OpcodeSize;
    uint32 Send2OpcodeOffset;

private:
    int32 _FindProcessMessageAddress();
    int32 _FindSend2Address();

    int32 _ProcessMessageAddress;
    int32 _Send2Address;

    bool _ProcessConfigFile(std::wstring file);

    std::unordered_map<std::string, std::string> _configValues;
    bool _ReadPattern(std::string name, uint8*& buffer, uint32& bufferLen, std::string& patternFlags);
    bool _ReadUIntValue(std::string name, uint32& value);
};