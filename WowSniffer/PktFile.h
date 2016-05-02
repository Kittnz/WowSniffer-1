#pragma once
#include "stdafx.h"

#include <Windows.h>
#include <string>

class PktFile
{
public:

    struct Packet
    {
        Packet(bool isSMSG, uint32 connectionID, CDataStore* dataStore, uint32 opcodeSize, uint32 opcodeOffset);

        struct Header
        {
            char DirectionArray[4];
            uint32 ConnectionID;
            uint32 TickCount;
            uint32 OptionalDataLen;
            uint32 BufferLenWithOpcode;
            uint32 Opcode;
        };

        Header Header;
        char* Buffer;
        uint32 BufferLen;
    };

    PktFile();
    ~PktFile();

    static PktFile* Create();

    void AppendPacket(Packet* packet);

private:
    template <typename T> void Write(T const& value);
    template <typename T> void Write(uint32 offset, T const& value);
    void WriteBytes(char const* buffer, uint32 count);
    void ZeroFill(uint32 count);

    PktFile& operator<<(uint64 value);
    PktFile& operator<<(uint32 value);
    PktFile& operator<<(uint16 value);
    PktFile& operator<<(uint8 value);

    HANDLE file;
};