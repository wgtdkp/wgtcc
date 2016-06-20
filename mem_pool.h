#ifndef _WGTCC_MEM_POOL_H_
#define _WGTCC_MEM_POOL_H_

#include <cstddef>
#include <vector>


class MemPool
{
public:
    MemPool(void): _allocated(0) {}
    
    virtual ~MemPool(void) {}
    
    MemPool(const MemPool& other) = delete;
    
    MemPool& operator=(const MemPool& other) = delete;

    virtual void* Alloc(void) = 0;
    
    virtual void Free(void* addr) = 0;
    
    virtual void Clear(void) = 0;

protected:
    size_t _allocated;
};

template <class T>
class MemPoolImp: public MemPool
{
public:
    MemPoolImp(void) : _root(nullptr) {}
    
    virtual ~MemPoolImp(void) {}

    MemPoolImp(const MemPool& other) = delete;
    
    MemPoolImp& operator=(MemPool& other) = delete;

    virtual void* Alloc(void);
    
    virtual void Free(void* addr);
	
    virtual void Clear(void);

private:
	enum {
        COUNT = (4 * 1024) / sizeof(T)
    };
    
    union Chunk {
        Chunk* _next;
        char _mem[sizeof(T)];
    };
    
    struct Block {
        Block(void) {
            for (size_t i = 0; i < COUNT - 1; i++)
                _chunks[i]._next = &_chunks[i+1];
            _chunks[COUNT-1]._next = nullptr;
        }
        Chunk _chunks[COUNT];
    };

    std::vector<Block*> _blocks;
    Chunk* _root;
};

#include "mem_pool.inc"

#endif
