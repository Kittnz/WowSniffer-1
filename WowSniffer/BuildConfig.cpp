#include "stdafx.h"
#include "BuildConfig.h"
#include <sstream>
#include <map>
#include <regex>

BuildConfig::BuildConfig() : _ProcessMessageAddress(0), _Send2Address(0)
{
}

bool BuildConfig::Load(std::wstring folder, std::string wowBuildType, uint32 build)
{
    std::wstringstream ss;
    ss << folder;
    ss << wowBuildType.c_str() << "\\*.conf";
    std::wstring finalFolder = ss.str();

    WIN32_FIND_DATAW foundFileData;
    HANDLE file = FindFirstFileW(finalFolder.c_str(), &foundFileData);
    std::map<uint32, std::wstring> builds;

    std::wregex configRegex = std::wregex(L".+\\Build_([0-9]+).conf");

    // Iterate all *\Build_*.conf files
    if (file != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstringstream ss2;
            ss2 << folder;
            ss2 << wowBuildType.c_str();
            ss2 << "\\" << foundFileData.cFileName;
            std::wstring name = ss2.str();
            std::wsmatch matches;

            if (std::regex_search(name, matches, configRegex))
                builds.insert({ _wtoi(matches[1].str().c_str()), name });

        }
        while (FindNextFileW(file, &foundFileData));

        FindClose(file);
    }

    // No files found
    if (!builds.size())
    {
        return false;
    }

    uint32 targetBuild = 0;
    std::wstring targetConfig;

    for (auto& itr : builds)
    {
        if (itr.first > build)
            break;

        targetBuild = itr.first;
        targetConfig = itr.second;
    }

    if (!targetBuild)
    {
        return false;
    }

    if (!_ProcessConfigFile(targetConfig))
    {
        return false;
    }

    _ReadPattern("ProcessMessagePattern", ProcessMessagePattern, ProcessMessagePatternLen, ProcessMessagePatternMask);
    _ReadUIntValue("ProcessMessageHookOffset", ProcessMessageHookOffset);
    _ReadUIntValue("ProcessMessageHookLen", ProcessMessageHookLen);
    _ReadUIntValue("ProcessMessageOpcodeSize", ProcessMessageOpcodeSize);
    _ReadUIntValue("ProcessMessageOpcodeOffset", ProcessMessageOpcodeOffset);

    _ReadPattern("Send2Pattern", Send2Pattern, Send2PatternLen, Send2PatternMask);
    _ReadUIntValue("Send2HookOffset", Send2HookOffset);
    _ReadUIntValue("Send2HookLen", Send2HookLen);
    _ReadUIntValue("Send2OpcodeSize", Send2OpcodeSize);
    _ReadUIntValue("Send2OpcodeOffset", Send2OpcodeOffset);
    return true;
}

int32 BuildConfig::GetProcessMessageAddress()
{
    if (!_ProcessMessageAddress)
        _ProcessMessageAddress = _FindProcessMessageAddress();

    return _ProcessMessageAddress;
}

int32 BuildConfig::GetSend2Address()
{
    if (!_Send2Address)
        _Send2Address = _FindSend2Address();

    return _Send2Address;
}

int32 BuildConfig::_FindProcessMessageAddress()
{
    return FindPattern<int32>(ProcessMessagePattern, ProcessMessagePatternLen, ProcessMessagePatternMask) + ProcessMessageHookOffset;
}

int32 BuildConfig::_FindSend2Address()
{
    return FindPattern<int32>(Send2Pattern, Send2PatternLen, Send2PatternMask) + Send2HookOffset;
}

bool BuildConfig::_ProcessConfigFile(std::wstring file)
{
    std::ifstream fileStream;
    fileStream.open(file, std::ifstream::in);

    if (!fileStream.is_open())
    {
        return false;
    }

    std::regex optionRegex(" *(\\S+) *= *(.+)");
    std::string line;
    while (std::getline(fileStream, line))
    {
        // Comment lines
        if (line[0] == '#' || line[0] == ';')
            continue;

        std::smatch matches;

        if (std::regex_search(line, matches, optionRegex))
            _configValues.insert({ matches[1].str(), matches[2].str() });
    }

    fileStream.close();
    return true;
}

bool BuildConfig::_ReadPattern(std::string name, uint8*& buffer, uint32& bufferLen, std::string& patternFlags)
{
    auto const& itr = _configValues.find(name);
    if (itr == _configValues.end())
        return false;

    std::string output = itr->second;
    std::istringstream isstream(output, std::istringstream::in);
    std::string hexNum;

    uint8 tempBuffer[256];
    char tempFlags[256];
    bufferLen = 0;

    while (isstream >> hexNum)
    {
        if (hexNum.length() > 2)
            return false;

        if (hexNum.find("?") != std::string::npos)
        {
            tempBuffer[bufferLen] = 0x00;
            tempFlags[bufferLen++] = '?';
            continue;
        }

        if (!isxdigit(hexNum[0]) || (hexNum.length() > 1 && !isxdigit(hexNum[1])))
            return false;

        tempBuffer[bufferLen] = (uint8)strtoul(hexNum.c_str(), nullptr, 16);
        tempFlags[bufferLen++] = 'x';
    }

    if (!bufferLen)
    {
        return false;
    }

    // Terminate string
    tempFlags[bufferLen] = NULL;

    buffer = new uint8[bufferLen];
    memcpy(buffer, tempBuffer, bufferLen);
    patternFlags = tempFlags;

    return true;
}

bool BuildConfig::_ReadUIntValue(std::string name, uint32& value)
{
    auto const& itr = _configValues.find(name);
    if (itr == _configValues.end())
        return false;

    std::string str = itr->second;
    for (int i = 0; i < str.length(); ++i)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }

    value = atol(str.c_str());
    return true;
}
