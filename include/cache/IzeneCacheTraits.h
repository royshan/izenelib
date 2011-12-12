#ifndef IZENECACHETRAITS_H_
#define IZENECACHETRAITS_H_

#include "cm_basics.h"
#include "CacheHash.h"

#include <ctime>
#include <list>
#include <fstream>

using namespace std;
using namespace __gnu_cxx;
using namespace boost;

namespace izenelib
{
namespace cache
{

enum HASH_TYPE {RDE_HASH, STX_BTREE, CCCR_HASH, LINEAR_HASH};

enum REPLACEMENT_TYPE {LRU, LFU};

template<class KeyType, class ValueType, REPLACEMENT_TYPE policy = LRU > struct IzeneCacheReplaceTrait
{
    typedef list<KeyType> CacheInfoListType;
    typedef typename list<KeyType> :: iterator LIT;

    template<class D> struct _CachedData
    {
        D data;
        LIT lit;
    };
    typedef _CachedData<ValueType> CachedDataType;

};

template<class KeyType, class ValueType> struct IzeneCacheReplaceTrait<KeyType,
            ValueType, LFU>
{
    typedef list< std::pair<KeyType, int> > CacheInfoListType;
    typedef typename list< std::pair<KeyType, int> > :: iterator LIT;

    template<class D> struct _CachedData
    {
        D data;
        LIT lit;
    };
    typedef _CachedData<ValueType> CachedDataType;

};

template<class KeyType, class ValueType, REPLACEMENT_TYPE policy = LRU,
HASH_TYPE hash_type=RDE_HASH> struct IzeneCacheContainerTrait
{

    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CacheInfoListType
    CacheInfoListType;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CachedDataType
    CachedDataType;

    typedef CacheHash<KeyType, CachedDataType*, izenelib::am::rde_hash<KeyType, CachedDataType*> >
    ContainerType;
};

template<class KeyType, class ValueType, REPLACEMENT_TYPE policy> struct IzeneCacheContainerTrait<
            KeyType, ValueType, policy, CCCR_HASH>
{

    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CacheInfoListType
    CacheInfoListType;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CachedDataType
    CachedDataType;
    typedef CacheHash<KeyType, CachedDataType*, izenelib::am::cccr_hash<KeyType, CachedDataType*> >
    ContainerType;
};

template<class KeyType, class ValueType, REPLACEMENT_TYPE policy> struct IzeneCacheContainerTrait<
            KeyType, ValueType, policy, LINEAR_HASH>
{

    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CacheInfoListType
    CacheInfoListType;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CachedDataType
    CachedDataType;

    typedef CacheHash<KeyType, CachedDataType*, izenelib::am::LinearHashTable<KeyType, CachedDataType*> >
    ContainerType;
};

template<class KeyType, class ValueType, REPLACEMENT_TYPE policy> struct IzeneCacheContainerTrait<
            KeyType, ValueType, policy, STX_BTREE>
{

    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CacheInfoListType
    CacheInfoListType;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CachedDataType
    CachedDataType;

    typedef CacheHash<KeyType, CachedDataType*, izenelib::am::stx_btree<KeyType, CachedDataType*> >
    ContainerType;
};

template<class KeyType, class ValueType, class ContainerType,
class CacheInfoListType, REPLACEMENT_TYPE policy=LRU> struct IzeneCacheReplacePolicy
{
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, policy>::CachedDataType
    CachedDataType;

    static void replace_(const KeyType& key, ContainerType& hash_,
                         CacheInfoListType& cacheContainer_)
    {
        CachedDataType **pd1 = hash_.find(key);
        assert(pd1 != 0);

        LIT newit = cacheContainer_.insert(cacheContainer_.end(), key);
        cacheContainer_.erase((*pd1)->lit);
        (*pd1)->lit = newit;

    }
    static void firstInsert_(const KeyType& key, const ValueType& val,
                             ContainerType& hash_, CacheInfoListType& cacheContainer_)
    {
        //assert(hash_.find(key) == NULL);
        LIT newit = cacheContainer_.insert(cacheContainer_.end(), key);
        CachedDataType* cd = new CachedDataType;
        cd->data = val;
        cd->lit = newit;
        hash_.insert(key, cd);
    }

    static inline void evict_(ContainerType& hash_,
                              CacheInfoListType& cacheContainer_)
    {
        KeyType key = cacheContainer_.front();
        cacheContainer_.pop_front();
        CachedDataType* cd = 0;
        hash_.get(key, cd);
        if (cd)
        {
            delete cd;
            cd = 0;
        }
        hash_.del(key);
    }

};

template<class KeyType, class ValueType, class ContainerType,
class CacheInfoListType> struct IzeneCacheReplacePolicy<KeyType,
            ValueType, ContainerType, CacheInfoListType, LFU>
{

    enum {firstFit = false};
    enum {lastFit = false};
    enum {randFit = false};

    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, LFU>::LIT LIT;
    typedef typename IzeneCacheReplaceTrait<KeyType, ValueType, LFU>::CachedDataType
    CachedDataType;

    static inline void replace_(const KeyType& key, ContainerType& hash_,
                                CacheInfoListType& cacheContainer_)
    {
        CachedDataType *pd1 = 0;
        assert(hash_.get(key, pd1));
        LIT lit = pd1->lit;
        ++lit->second;

        int freq = lit->second;

        int a=0;
        int count =rand() & 0x0f;
        while (lit->second <= freq && lit != cacheContainer_.end() )
        {
            ++lit;
            if (firstFit && lit->second == freq)
                break;
            if (randFit && lit->second == freq)
            {
                ++a;
                if (a>count)
                    break;
            }
        }
        LIT newit = cacheContainer_.insert(lit, make_pair(key, freq) );
        cacheContainer_.erase(pd1->lit);
        pd1->lit = newit;

    }

    static inline void firstInsert_(const KeyType& key, const ValueType& val,
                                    ContainerType& hash_, CacheInfoListType& cacheContainer_)
    {
        LIT lit = cacheContainer_.begin();
        int a=0;
        int count =rand() & 0x0f;
        while (lit->second <= 1 && lit != cacheContainer_.end() )
        {
            ++lit;
            if (firstFit && lit->second == 1)
                break;
            if (randFit && lit->second == 1)
            {
                ++a;
                if (a>count)
                    break;
            }
        }
        CachedDataType* cd = new CachedDataType;
        cd->data = val;
        cd->lit = (cacheContainer_.insert(lit, make_pair(key, 1) ) );
        hash_.insert(key, cd);
    }

    static inline void evict_(ContainerType& hash_,
                              CacheInfoListType& cacheContainer_)
    {
        std::pair<KeyType, int> kp = cacheContainer_.front();
        cacheContainer_.pop_front();
        CachedDataType* cd = 0;
        hash_.get(kp.first, cd);
        if (cd)
        {
            delete cd;
            cd = 0;
        }
        hash_.del(kp.first);
    }

};

template<class KeyType, class ValueType, HASH_TYPE hash_type=RDE_HASH,
REPLACEMENT_TYPE policy = LRU> class IzeneCacheTrait
{
public:
    typedef typename IzeneCacheContainerTrait<KeyType, ValueType, policy, hash_type>::CacheInfoListType
    CacheInfoListType;
    typedef typename IzeneCacheContainerTrait<KeyType, ValueType, policy, hash_type>::LIT
    LIT;
    typedef typename IzeneCacheContainerTrait<KeyType, ValueType, policy, hash_type>::CachedDataType
    CachedDataType;
    typedef typename IzeneCacheContainerTrait<KeyType, ValueType, policy, hash_type>::ContainerType
    ContainerType;
    typedef IzeneCacheReplacePolicy<KeyType, ValueType,ContainerType, CacheInfoListType, policy>
    ReplacePolicy;

public:
    IzeneCacheTrait(ContainerType& hash, CacheInfoListType& cacheContainer) :
            hash_(hash), cacheContainer_(cacheContainer)
    {
    }
    ~IzeneCacheTrait()
    {
        clear();
    }

    bool get(const KeyType& key, ValueType& val)
    {
        CachedDataType *pd=0;
        if (hash_.get(key, pd) )
        {
            val = pd->data;
            return true;
        }
        return false;

    }

    bool del(const KeyType& key)
    {
        CachedDataType* pd = 0;
        hash_.get(key, pd);
        if (pd)
        {
            cacheContainer_.erase(pd->lit);
            delete pd;
            pd = 0;
            hash_.del(key);
            return true;
        }
        return false;
    }

    void clear()
    {
        while (!cacheContainer_.empty() )
        {
            evict_();
        }
    }

    bool hasKey(const KeyType& key)
    {
        CachedDataType* pd = 0;
        return (hash_.get(key, pd) );
    }

    int numItems()
    {
        return hash_.numItems();
    }

    inline void replace_(const KeyType& key)
    {
        ReplacePolicy::replace_(key, hash_, cacheContainer_);

    }
    inline void firstInsert_(const KeyType& key, const ValueType& val)
    {
        ReplacePolicy::firstInsert_(key, val, hash_, cacheContainer_);
    }

    void evict_()
    {
        ReplacePolicy::evict_(hash_, cacheContainer_);

    }

private:
    ContainerType& hash_;
    CacheInfoListType& cacheContainer_;

};

}
}
#endif /*IZENECACHETRAITS_H_*/
