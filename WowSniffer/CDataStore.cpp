#include "stdafx.h"
#include "CDataStore.h"

CDataStore::CDataStore() : vfTable(CDataStoreVFTable), Buffer(nullptr), Base(0), Alloc(-1), Size(0), Read(0)
{   
}

CDataStore::CDataStore(Opcodes opcode, uint32 size) : CDataStore()
{
    InitBuffer(size + 6);   // Size and header
    *this << uint32(0);
    *this << uint16(opcode);
}

CDataStore::~CDataStore()
{
    if (Buffer)
        delete[] Buffer;
}

void CDataStore::InitBuffer(uint32 size)
{
    if (Buffer)
        return;

    Buffer = new char[size];
    Alloc = size;
}

void CDataStore::Resize(uint32 size)
{
    if (!Buffer)
    {
        InitBuffer(size);
        return;
    }

    auto newBuffer = new char[size];
    std::copy_n(Buffer, Alloc, newBuffer);
    delete[] Buffer;
    Buffer = newBuffer;
    Alloc = size;
}

CDataStore& CDataStore::operator<<(uint64 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(int64 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(uint32 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(int32 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(uint16 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(int16 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(uint8 rSide)
{
    return Append(rSide);
}

CDataStore& CDataStore::operator<<(int8 rSide)
{
    return Append(rSide);
}

uint32 CDataStore::SetRPos(uint32 size)
{
    return Read = size;
}

