template <class T>
void* MemPoolImp<T>::Alloc(void)
{
    if (nullptr == _root) { //空间不够，需要分配空间
        auto block = new Block();
        _root = block->_chunks;
        //如果blocks实现为std::list, 那么push_back实际的overhead更大
        //这也表明，即使我们不需要随机访问功能(那么std::vector的拷贝是一种overhead)，
        //仍然倾向于使用std::vector，
        //当然std::vector的指数级capacity增长会造成内存浪费。
        _blocks.push_back(block);
    }
    
    auto ret = _root;
    _root = _root->_next;

    ++_allocated;
    return ret;
}

template <class T>
void MemPoolImp<T>::Free(void* addr)
{
    if (nullptr == addr) 
        return;

    auto chunk = static_cast<Chunk*>(addr);
    chunk->_next = _root;
    _root = chunk;

    --_allocated;
}

template <class T>
void MemPoolImp<T>::Clear(void)
{
    for (auto block: _blocks)
        delete block;

    _blocks.resize(0);
    _root = nullptr;
    _allocated = 0;
}
