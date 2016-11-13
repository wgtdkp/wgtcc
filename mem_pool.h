#ifndef _WGTCC_MEM_POOL_H_
#define _WGTCC_MEM_POOL_H_

#include <cstddef>
#include <vector>


class MemPool {
public:
  MemPool(): allocated_(0) {}
  virtual ~MemPool() {}
  MemPool(const MemPool& other) = delete;
  MemPool& operator=(const MemPool& other) = delete;
  virtual void* Alloc() = 0;
  virtual void Free(void* addr) = 0;
  virtual void Clear() = 0;

protected:
  size_t allocated_;
};


template <class T>
class MemPoolImp: public MemPool {
public:
  MemPoolImp() : root_(nullptr) {}
  virtual ~MemPoolImp() {}
  MemPoolImp(const MemPool& other) = delete;
  MemPoolImp& operator=(MemPool& other) = delete;
  virtual void* Alloc();
  virtual void Free(void* addr);
  virtual void Clear();

private:
  enum {
    COUNT = (4 * 1024) / sizeof(T)
  };
  
  union Chunk {
    Chunk* next_;
    char mem_[sizeof(T)];
  };
  
  struct Block {
    Block() {
      for (size_t i = 0; i < COUNT - 1; ++i)
        chunks_[i].next_ = &chunks_[i+1];
      chunks_[COUNT-1].next_ = nullptr;
    }
    Chunk chunks_[COUNT];
  };

  std::vector<Block*> blocks_;
  Chunk* root_;
};


template <class T>
void* MemPoolImp<T>::Alloc() {
  if (root_ == nullptr) {
    auto block = new Block();
    root_ = block->chunks_;
    blocks_.push_back(block);
  }
  
  auto ret = root_;
  root_ = root_->next_;

  ++allocated_;
  return ret;
}


template <class T>
void MemPoolImp<T>::Free(void* addr) {
  if (nullptr == addr) 
    return;

  auto chunk = static_cast<Chunk*>(addr);
  chunk->next_ = root_;
  root_ = chunk;

  --allocated_;
}


template <class T>
void MemPoolImp<T>::Clear() {
  for (auto block: blocks_)
    delete block;

  blocks_.resize(0);
  root_ = nullptr;
  allocated_ = 0;
}

#endif
