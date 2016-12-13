#pragma once
#include "stdafx.h"

#define DEFAULT_RESIZE_SIZE 0x100

struct CDataStore
{
    void* vfTable;
    char* Buffer;
    uint32 Base;
    uint32 Alloc;
    uint32 Size;
    uint32 Read;

    CDataStore();
    CDataStore(Opcodes opcode, uint32 size);
    ~CDataStore();

    void InitBuffer(uint32 size);
    void Resize(uint32 size);

    CDataStore& operator<<(uint64 rSide);
    CDataStore& operator<<(int64 rSide);
    CDataStore& operator<<(uint32 rSide);
    CDataStore& operator<<(int32 rSide);
    CDataStore& operator<<(uint16 rSide);
    CDataStore& operator<<(int16 rSide);
    CDataStore& operator<<(uint8 rSide);
    CDataStore& operator<<(int8 rSide);
    uint32 SetRPos(uint32 size);

    template <typename T>
    CDataStore& Append(T const& value)
    {
        if (Size + sizeof(T) > Alloc)
        {
            Resize(Size + DEFAULT_RESIZE_SIZE * (sizeof(T) / DEFAULT_RESIZE_SIZE + 1));
            return* this;
        }

        *reinterpret_cast<T*>(Buffer + Size) = value;
        Size += sizeof(T);
        return *this;
    }

    template <typename T>
    CDataStore& AppendArray(T const* srcArray, uint32 size)
    {
        if (Size + sizeof(T) * size > Alloc)
        {
            Resize(Size + DEFAULT_RESIZE_SIZE * (sizeof(T) * size / DEFAULT_RESIZE_SIZE + 1));
            return* this;
        }

        std::copy_n(reinterpret_cast<char*>(srcArray), sizeof(T) * size, Buffer);
        Size += sizeof(T) * size;
        return *this;
    }

    template <typename T>
    CDataStore& Put(T value, uint32 pos)
    {
        if (pos + sizeof(T) > Alloc)
            return *this;

        *reinterpret_cast<T*>(Buffer + pos) = value;
        return *this;
    }
};
