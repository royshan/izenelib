
#ifndef IZENELIB_IR_BTREEINDEXER_H_
#define IZENELIB_IR_BTREEINDEXER_H_

#include <util/BoostVariantUtil.h>
#include <am/bitmap/Ewah.h>
#include <am/luxio/BTree.h>
#include <am/range/AmIterator.h>
#include <ir/index_manager/index/rtype/InMemoryBTreeCache.h>
#include <ir/index_manager/index/rtype/TermEnum.h>
#include <ir/index_manager/index/rtype/Compare.h>
#include <ir/index_manager/index/IndexerDocument.h>
#include <ir/index_manager/utility/BitVector.h>
#include <ir/index_manager/utility/StringUtils.h>
#include <ir/index_manager/store/Directory.h>

#include <boost/variant.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/bind.hpp>
#include <functional>

#include <algorithm>
#include <string>

// #define BT_DEBUG

NS_IZENELIB_IR_BEGIN

namespace indexmanager
{
/**
 * BTreeIndexer
 * @brief BTreeIndexer is a wrapper which takes charges of managing all b-tree handlers.
 */

template <class KeyType>
class CBTreeIndexer
{
    typedef CBTreeIndexer<KeyType> ThisType;
    typedef izenelib::am::EWAHBoolArray<uint32_t> ValueType;
    typedef izenelib::am::luxio::BTree<KeyType, ValueType> DbType;
    typedef InMemoryBTreeCache<KeyType, docid_t> CacheType;
    typedef typename CacheType::ValueType CacheValueType;
    
    typedef izenelib::am::AMIterator<DbType> ForwardIterator;
    typedef izenelib::am::AMReverseIterator<DbType> ReverseIterator;
    
//     typedef BTTermEnum<KeyType, CacheValueType> EnumType1;
//     typedef AMTermEnum<DbType> EnumType2;
    typedef TwoWayTermEnum<KeyType, CacheValueType, DbType, BitVector> EnumType;
    typedef TermEnum<KeyType, BitVector> BaseEnumType;
    typedef boost::function<void (const CacheValueType&,const ValueType&, BitVector&) > EnumCombineFunc;
    
public:
    CBTreeIndexer(const std::string& path, const std::string& property_name, std::size_t cacheSize = 12500000)//5M for uint32_t
    :path_(path), property_name_(property_name)
    {
        cache_.set_max_capacity(cacheSize);
        func_ = &combineValue_;
    }

    ~CBTreeIndexer()
    {
    }
    
    bool open()
    {
        return db_.open(path_);
    }
    
    void close()
    {
        db_.close();
    }
    
    bool seek(const KeyType& key)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        ValueType value;
        return getValue_(key, value);
    }

    void getNoneEmptyList(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        getValue_(key, docs);
    }

    void add(const KeyType& key, docid_t docid)
    {
        boost::lock_guard<boost::shared_mutex> lock(mutex_);
        cache_.add(key, docid);
        checkCache_();
        
    }

    void remove(const KeyType& key, docid_t docid)
    {
        boost::lock_guard<boost::shared_mutex> lock(mutex_);
        cache_.remove(key, docid);
        checkCache_();
        
    }

    void getValue(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        getValue_(key, docs);
    }

    void getValue(const KeyType& key, std::vector<docid_t>& docList)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        BitVector docmap;
        getValue(key, docmap);
        for(std::size_t docid=1;docid<docmap.size();docid++)
        {
            if(docmap.test(docid))
            {
                docList.push_back(docid);
            }
        }
    }
    
    std::size_t getValueBetween(const KeyType& lowKey, const KeyType& highKey, std::size_t maxDoc, KeyType* & data)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::size_t result = 0;
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(lowKey));
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            if( compare_(kvp.first,highKey)>0 ) break;
            for(std::size_t docid=1;docid<kvp.second.size();docid++)
            {
                if (docid >= maxDoc) break;
                if(kvp.second.test(docid))
                {
                    data[docid] = kvp.first;
                    ++result;
                }
            }
        }
        
        return result;
    }

    void getValueBetween(const KeyType& key1, const KeyType& key2, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key1));
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            if( compare_(kvp.first,key2)>0 ) break;
            docs |= kvp.second;
        }
    }

    void getValueLess(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            if( compare_(kvp.first,key)>=0 ) break;
            docs |= kvp.second;
#ifdef BT_DEBUG
            std::cout<<"after this iterator : "<<kvp.first<<","<<docs<<std::endl;
#endif
        }
        
    }

    void getValueLessEqual(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            if( compare_(kvp.first,key)>0 ) break;
            docs |= kvp.second;
        }
    }

    void getValueGreat(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            if( compare_(kvp.first,key)==0 ) continue;
            docs |= kvp.second;
        }
    }

    void getValueGreatEqual(const KeyType& key, BitVector& docs)
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
        std::pair<KeyType, BitVector> kvp;
        while(term_enum->next(kvp))
        {
            docs |= kvp.second;
        }
    }

//     void getValueIn(const std::vector<KeyType>& keys, BitVector& docs)
//     {
//         for(std::size_t i=0;i<keys.size();i++)
//         {
//             BitVector bv;
//             getValue(keys[i], bv);
//             docs |= bv;
//         }
//     }
// 
//     void getValueNotIn(const std::vector<KeyType>& keys, BitVector& docs)
//     {
//         getValueIn(keys, docs);
//         docs.toggle();
//     }
// 
//     void getValueNotEqual(const KeyType& key, BitVector& docs)
//     {
//         getValue(key, docs);
//         docs.toggle();
//     }

//     void getValueStart(const KeyType& key, BitVector& docs)
//     {
//         if( !std::is_same( KeyType, izenelib::util::UString)::value)//not ustring
//         {
//             std::cout<<"call getValueStart in no ustring type"<<std::endl;
//             return;
//         }
//         std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
//         std::pair<KeyType, BitVector> kvp;
//         while(term_enum->next(kvp))
//         {
//             docs |= kvp.second;
//         }
//     }
// 
//     void getValueEnd(const KeyType& key, BitVector& docs)
//     {
//         if( !std::is_same( KeyType, izenelib::util::UString)::value)//not ustring
//         {
//             std::cout<<"call getValueEnd in no ustring type"<<std::endl;
//             return;
//         }
//         std::auto_ptr<BaseEnumType> term_enum(getEnum_());
//         std::pair<KeyType, BitVector> kvp;
//         while(term_enum->next(kvp))
//         {
//             if(kvp.first>key) break;
//             docs |= kvp.second;
//         }
//     }

//     void getValueSubString(const KeyType& key, BitVector& docs)
//     {
//         std::auto_ptr<BaseEnumType> term_enum(getEnum_());
//         std::pair<KeyType, BitVector> kvp;
//         while(term_enum->next(kvp))
//         {
//             if(IsSubString(kvp.first, key)
//             {
//                 docs |= kvp.second;
//             }
//         }
//     }

    void flush()
    {
        boost::lock_guard<boost::shared_mutex> lock(mutex_);
        cacheClear_();
        db_.flush();
    }

    


private:
    
    template<typename T>
    static int compare_(const T& t1, const T& t2)
    {
        return Compare::compare(t1, t2);
    }
    
    
    BaseEnumType* getEnum_(const KeyType& lowKey)
    {
//         EnumType1 term_enum1(cache_.getAM(), lowKey);
//         EnumType2 term_enum2(db_, lowKey);
        BaseEnumType* term_enum = new EnumType(cache_.getAM(), db_, lowKey, func_);
        return term_enum;
    }
    
    BaseEnumType* getEnum_()
    {
//         EnumType1 term_enum1(cache_.getAM());
//         EnumType2 term_enum2(db_);
        BaseEnumType* term_enum = new EnumType(cache_.getAM(), db_, func_);
        return term_enum;
    }
    
    void checkCache_()
    {
        if(cache_.is_full())
        {
            cacheClear_();
        }
    }
    
    void cacheClear_()
    {
#ifdef BT_DEBUG
        std::cout<<"!!!cacheClear_ "<<property_name_<<", key size: "<<cache_.key_size()<<std::endl;
#endif
        boost::function<void (const std::pair<KeyType, CacheValueType>&) > func=boost::bind( &ThisType::cacheIterator_, this, _1);
        cache_.iterate(func);
        cache_.clear();
    }
    
    void cacheIterator_(const std::pair<KeyType, CacheValueType>& kvp)
    {
        ValueType bitmap;
#ifdef BT_DEBUG
        std::cout<<"cacheIterator : "<<kvp.first<<","<<kvp.second<<std::endl;
#endif
        getValue_(kvp.first, kvp.second, bitmap);
#ifdef BT_DEBUG
        std::cout<<"cacheIterator update : "<<kvp.first<<","<<bitmap<<std::endl;
#endif
        db_.update(kvp.first, bitmap);
    }
    
    bool getDbValue_(const KeyType& key, ValueType& value)
    {
        return db_.get(key, value);
    }
    
    bool getCacheValue_(const KeyType& key, CacheValueType& value)
    {
        return cache_.get(key, value);
    }
    
    bool getValue_(const KeyType& key, ValueType& bitmap)
    {
        bool b_db = getDbValue_(key, bitmap);
        CacheValueType cache_value;
        bool b_cache = getCacheValue_(key, cache_value);
        if(!b_db && !b_cache) return false;
        if(b_cache)
        {
            setCacheValue_(bitmap, cache_value);
        }
        return true;
    }
    
    bool getValue_(const KeyType& key, BitVector& value)
    {
        ValueType compressed;
        if(!getValue_(key, compressed)) return false;
        decompress_(compressed, value);
        return true;
    }
    
    void getValue_(const KeyType& key, const CacheValueType& cacheValue, ValueType& value)
    {
        getDbValue_(key, value);
#ifdef BT_DEBUG
        std::cout<<"after getDbValue : "<<key<<","<<value<<std::endl;
#endif
        setCacheValue_(value, cacheValue);
#ifdef BT_DEBUG
        std::cout<<"after setCacheValue : "<<key<<","<<value<<std::endl;
#endif
    }
    
    /// the dbValue already got before
    void getValue2_(const KeyType& key, ValueType& dbValue)
    {
        CacheValueType cacheValue;
        if(getCacheValue_(key, cacheValue))
        {
            setCacheValue_(dbValue, cacheValue);
        }
    }
    
    static void decompress_(const ValueType& compressed, BitVector& value)
    {
        //TODO optimize?
        izenelib::am::EWAHBoolArrayBitIterator<uint32_t> it = compressed.bit_iterator();
        while(it.next())
        {
            value.set(it.getCurr() );
        }
    }
    
    static void compress_(const BitVector& value, ValueType& compressed)
    {
        value.compressed(compressed);
    }
    
    static void setCacheValue_(ValueType& bitmap, const CacheValueType& cacheValue)
    {
        bool need_decompress = false;
        if(cacheValue.delete_item.size()>0 || cacheValue.update_item.size()>0 )
        {
            need_decompress = true;
        }
#ifdef BT_DEBUG
        std::cout<<"need_decompress : "<<(int)need_decompress<<std::endl;
#endif        
        if(!need_decompress)
        {
            for(uint32_t i=0;i<cacheValue.insert_item.size();i++)
            {
#ifdef BT_DEBUG
                std::cout<<"bitmap : "<<bitmap<<" , add "<<cacheValue.insert_item[i]<<std::endl;
#endif   
                bitmap.set(cacheValue.insert_item[i]);
#ifdef BT_DEBUG
                std::cout<<"bitmap : "<<bitmap<<std::endl;
                izenelib::am::BoolArray<uint32_t> boolArray;
                bitmap.toBoolArray(boolArray);
                boolArray.printout(std::cout);
#endif   
            }
        }
        else
        {
            BitVector value;
            decompress_(bitmap, value);
            for(uint32_t i=0;i<cacheValue.insert_item.size();i++)
            {
                value.set(cacheValue.insert_item[i]);
            }
            for(uint32_t i=0;i<cacheValue.delete_item.size();i++)
            {
                value.clear(cacheValue.delete_item[i]);
            }
            for(uint32_t i=0;i<cacheValue.update_item.size();i++)
            {
                value.set(cacheValue.update_item[i]);
            }
            
            bitmap.reset();
            compress_(value, bitmap);
        }
    }
    
    static void combineValue_(const CacheValueType& cacheValue, const ValueType& cbitmap, BitVector& result)
    {
#ifdef BT_DEBUG
        std::cout<<"before decompress"<<std::endl;
        std::cout<<result<<std::endl;
        std::cout<<cbitmap<<std::endl;
#endif
        decompress_(cbitmap, result);
        
#ifdef BT_DEBUG
        std::cout<<"after decompress"<<std::endl;
        std::cout<<result<<std::endl;
        std::cout<<cacheValue<<std::endl;
#endif
        
        for(uint32_t i=0;i<cacheValue.insert_item.size();i++)
        {
            result.set(cacheValue.insert_item[i]);
        }
        for(uint32_t i=0;i<cacheValue.delete_item.size();i++)
        {
            result.clear(cacheValue.delete_item[i]);
        }
        for(uint32_t i=0;i<cacheValue.update_item.size();i++)
        {
            result.set(cacheValue.update_item[i]);
        }
#ifdef BT_DEBUG
        std::cout<<"after combine"<<std::endl;
        std::cout<<result<<std::endl;
#endif
    }
    
private:
    std::string path_;
    std::string property_name_;
    DbType db_;
    CacheType cache_;
    EnumCombineFunc func_;
    boost::shared_mutex mutex_;
};


}

NS_IZENELIB_IR_END

#endif /*BTreeIndex_H*/
