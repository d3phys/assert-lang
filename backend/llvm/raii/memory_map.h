#pragma once
#include <stdexcept>
#include "iommap.h"

struct MemoryMap
{
    MemoryMap( const char* file_name)
    {
        int error = mmap_in( &data, file_name);
        if ( error )
        {
            throw std::runtime_error{ "memory map failed"};
        }
    }

    ~MemoryMap()
    {
        unmap();
    }

    void unmap() noexcept
    {
        if ( data.buf == nullptr )
        {
            return;
        }

        mmap_free( &data);
        data.size = 0;
        data.buf = nullptr;
    }

    mmap_data data;
};


