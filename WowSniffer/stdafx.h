#pragma once

#include "targetver.h"

#include <Windows.h>
#include <wincon.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <TlHelp32.h>
#include <chrono>
#include <thread>
#include <Psapi.h>

typedef int64_t            int64;
typedef int32_t            int32;
typedef int16_t            int16;
typedef int8_t             int8;
typedef uint64_t           uint64;
typedef uint32_t           uint32;
typedef uint16_t           uint16;
typedef uint8_t            uint8;

struct CDataStore
{
    virtual void vf_table() {};

    char* Buffer;
    uint32 Base;
    uint32 Alloc;
    uint32 Size;
    uint32 Read;
};
