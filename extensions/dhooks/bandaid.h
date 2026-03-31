#pragma once

// Yes this is bad, but whatever dhooks will be nuked soon
#include <../vm/code-allocator.h>

class DhookCodePool : public ke::Refcounted<DhookCodePool> {
public:
    uint8_t* start_;
    uint8_t* ptr_;
    uint8_t* end_;
    size_t size_;
};

struct DhookCodeChunk {
    DhookCodeChunk(uint8_t* addr, size_t bytes)
     : address(addr)
     , bytes(bytes) {
    }

    uint8_t pool[sizeof(ke::RefPtr<DhookCodePool>)];
    uint8_t* address;
    size_t bytes;
};