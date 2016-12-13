#include "stdafx.h"
#include "PktFile.h"
#include "Functions.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

PktFile::Packet::Packet(bool isSMSG, uint32 connectionID, CDataStore* dataStore, uint32 opcodeSize, uint32 opcodeOffset)
{
      
    Header.DirectionArray[0] = isSMSG ? 'S' : 'C';      
    Header.DirectionArray[1] = 'M';
    Header.DirectionArray[2] = 'S';
    Header.DirectionArray[3] = 'G';
    Header.TickCount = GetTickCount();
    Header.Opcode = 0;
    Header.OptionalDataLen = 0;
    Header.ConnectionID = connectionID;

    memcpy(&Header.Opcode, dataStore->Buffer + opcodeOffset, opcodeSize);
    uint32 bufferStartOffset = opcodeOffset + opcodeSize;
    BufferLen = dataStore->Size - bufferStartOffset;
    Buffer = dataStore->Buffer + bufferStartOffset;
    Header.BufferLenWithOpcode = BufferLen + sizeof(Header.Opcode);
}

PktFile::PktFile() : file(nullptr)
{
}

PktFile::~PktFile()
{
    CloseHandle(file);
}

PktFile* PktFile::Create()
{
    uint32 expVersion;
    uint32 majorVersion;
    uint32 minorVersion;
    uint32 build;
    char timeBuffer[80];

    // Generate file name
    // format: 7_0_3_20100_2015_03_06-14_19_15_00.pkt

    time_t curTime;
    time(&curTime);
    tm* timeInfo = localtime(&curTime);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y_%m_%d-%I_%M_%S", timeInfo);

    GetBuildInfo(&expVersion, &majorVersion, &minorVersion, &build);

    std::wstringstream ss;
    ss << expVersion << "_" << majorVersion << "_" << minorVersion << "_" << build << "_";
    ss << timeBuffer;

    WCHAR const* unfinishedPtr = ss.str().c_str();
    for (int32 i = 0; i < 0xFF; i++)
    {
        WCHAR tempBuff[MAX_PATH];
        wsprintfW(tempBuff, L"%s_%0X.pkt", unfinishedPtr, i);

        if (!FileExists(tempBuff))
        {
            ss << "_" << std::setfill(L'0') << std::setw(2) << std::hex << i;
            ss << ".pkt";
            break;
        }
    }

    PktFile* pkt = new PktFile();

    // FILE_FLAG_WRITE_THROUGH disables caching
    pkt->file = CreateFileW(ss.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);

    if (pkt->file == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }

    // Append PKT 3.0.1 Header
    std::string optionalData = "Sniffed by WoWSniffer By Sovak";

    pkt->WriteBytes("PKT", 3);                                      // File magic - 'PKT'
    *pkt << (uint16)0x301;                                          // PKT Version 3.0.1
    *pkt << (uint8)'W';                                             // Sniffer ID
    *pkt << (uint32)build;                                          // Build
    *pkt << (uint32)GetLocaleName();                                // Locale
    pkt->ZeroFill(40);                                              // Session Key
    *pkt << (uint32)time(nullptr);                                  // Start Time
    *pkt << (uint32)GetTickCount();                                 // Start Tick Count
    *pkt << (uint32)optionalData.length();                          // Optional Data len
    pkt->WriteBytes(optionalData.c_str(), optionalData.length());   // Optional Data

    return pkt;
}

void PktFile::AppendPacket(Packet* packet)
{
    // Header is written at once, because of no caching
    WriteBytes((char*)&packet->Header, sizeof(packet->Header));
    WriteBytes(packet->Buffer, packet->BufferLen);
}

template <typename T>
void PktFile::Write(T const& value)
{
    WriteFile(file, &value, sizeof(value), nullptr, nullptr);
}

template <typename T>
void PktFile::Write(uint32 offset, T const& value)
{
    uint32 currentOffset = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    SetFilePointer(file, offset, nullptr, FILE_BEGIN);

    WriteFile(file, &value, sizeof(value), nullptr, nullptr);

    SetFilePointer(file, currentOffset, nullptr, FILE_BEGIN);
    return;
}

void PktFile::WriteBytes(char const* buffer, uint32 size)
{
    WriteFile(file, buffer, size, nullptr, nullptr);
}

void PktFile::ZeroFill(uint32 count)
{
    char* tempBuffer = new char[count];
    memset(tempBuffer, 0, count);
    WriteBytes(tempBuffer, count);
    delete tempBuffer;
}

PktFile& PktFile::operator<<(uint64 value)
{
    Write(value);
    return *this;
}

PktFile&  PktFile::operator<<(uint32 value)
{
    Write(value);
    return *this;
}

PktFile&  PktFile::operator<<(uint16 value)
{
    Write(value);
    return *this;
}

PktFile&  PktFile::operator<<(uint8 value)
{
    Write(value);
    return *this;
}
