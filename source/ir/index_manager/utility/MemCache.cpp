#include <ir/index_manager/utility/MemCache.h>


using namespace izenelib::ir::indexmanager;

MemCache::MemCache(size_t cachesize)
{
    size_ = cachesize;
    begin_ = (uint8_t*) malloc(size_);
    if (begin_ == NULL)
    {
        SF1V5_THROW(ERROR_OUTOFMEM,"MemCache alloc memory failed.");
    }
    end_ = begin_;
    pGrowCache_ = NULL;
}


MemCache::~MemCache()
{
    free(begin_);

    if (pGrowCache_)
    {
        delete pGrowCache_;
        pGrowCache_ = NULL;
    }
}

uint8_t* MemCache::getMem(size_t chunksize)
{
    if ((chunksize < MINPOW) || (chunksize > MAXPOW))
        return NULL;

    size_t size = POW_TABLE[chunksize];

    if ((end_ - begin_) + size > size_)
    {
        if (pGrowCache_)
            return pGrowCache_->getMem(chunksize);
        return NULL;
    }
    uint8_t* curr_ = end_;

    end_ += size;

    return curr_;
}


void MemCache::flushMem()
{
    end_ = begin_;

    if (pGrowCache_)
    {
        delete pGrowCache_;
        pGrowCache_ = NULL;
    }
}

const uint8_t* MemCache::getBegin()
{
    return begin_;
}

const uint8_t* MemCache::getEnd()
{
    return end_;
}

MemCache* MemCache::grow(size_t growSize)
{
    if (pGrowCache_)
        return pGrowCache_->grow(growSize);

    pGrowCache_ = new MemCache(growSize);
    return pGrowCache_;
}

