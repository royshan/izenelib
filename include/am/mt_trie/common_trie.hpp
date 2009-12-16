/**
 * @author Wei Cao
 * @date 2009-12-15
 */

#ifndef _COMMON_TRIE_H_
#define _COMMON_TRIE_H_

#include <iostream>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>

#include <am/concept/DataType.h>
#include <hdb/HugeDB.h>

#include "traits.h"

NS_IZENELIB_AM_BEGIN

/**
 * Maintain all edges' information in a Trie data structure in the form of
 * database table.
 * As all kinds of graph in computer science, Trie's graph representation also
 * contains two types of basic elements, node and edge.
 * Be different from other graphs, Each edge in Trie has a character propery.
 * So Given a node, the edges from the root node to the given node form a path,
 * and all the characters on the path form a string. That is to say, each node in
 * Trie can represent a string.
 * Node may contains pointers to numerous child nodes, A parent node P has pointer
 * to a child node C means, the string represented by P is the prefix of the string
 * represented by C. For example, node "ca" is the parent of node "cat".
 * Node can be an leaf node means, the string it represents is a user input string.
 * What is more, a node is a leaf node DOES NOT indicate it does not have children.
 * For example, user insert "cat" into trie, so node "cat" is a leaf node, however,
 * if user insert "catch" too, then leaf node "cat" could become the parent of
 * node "catc".
 */

/**
 * @brief Base class for all EdgeTable derivations.
 */
template <typename CharType,
          typename NodeIDType,
          typename LockType>
class CommonEdgeTable {
public:
    typedef std::pair<NodeIDType, CharType> EdgeTableKeyType;
    typedef NodeIDType EdgeTableValueType;

public:
    CommonEdgeTable(const std::string & name) : name_(name) {}
    virtual ~CommonEdgeTable() {}

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void flush() = 0;
    virtual void optimize() = 0;

    virtual void insertValue(const EdgeTableKeyType&,
        const EdgeTableValueType &) = 0;

    virtual bool getValue(const EdgeTableKeyType&,
        EdgeTableValueType &) = 0;

    virtual bool getValueBetween( std::vector<DataType<EdgeTableKeyType,
        EdgeTableValueType> > &, const EdgeTableKeyType&,
        const EdgeTableKeyType&) = 0;

protected:
    std::string name_;
};

/**
 * @brief Base class for all DataTable derivations.
 */
template <typename NodeIDType,
          typename UserDataType,
          typename LockType>
class CommonDataTable {
public:
    typedef NodeIDType DataTableKeyType;
    typedef UserDataType DataTableValueType;

public:
    CommonDataTable(const std::string & name) : name_(name) {}
    virtual ~CommonDataTable() {}

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void flush() = 0;
    virtual void optimize() = 0;
    virtual unsigned int  numItems() = 0;

    virtual void insertValue(const DataTableKeyType&,
        const DataTableValueType &) = 0;

    virtual bool getValue(const DataTableKeyType&,
        DataTableValueType &) = 0;

protected:
    std::string name_;
};

/**
 * @brief Base class for all DBTrie.
 */
template <typename CharType,
          typename UserDataType,
          typename NodeIDType,
          typename EdgeTableType,
          typename DataTableType>
class CommonTrie_
{

    enum FindRegexpParameterType {
        EnumerateKey = 0,
        EnumerateValue = 1,
        EnumerateBoth = 2
    };

    /**
     * Definitions of Edge Table
     */
    typedef typename EdgeTableType::EdgeTableKeyType EdgeTableKeyType;
    typedef typename EdgeTableType::EdgeTableValueType EdgeTableValueType;
    typedef DataType<EdgeTableKeyType, EdgeTableValueType> EdgeTableRecordType;

    /**
     * Definitions of Data Table
     */
    typedef typename DataTableType::DataTableKeyType DataTableKeyType;
    typedef typename DataTableType::DataTableValueType DataTableValueType;
    typedef DataType<DataTableKeyType, DataTableValueType> DataTableRecordType;

    typedef CommonTrie_<CharType, UserDataType, NodeIDType, EdgeTableType, DataTableType> ThisType;

public:

    CommonTrie_(const std::string& name)
    :   name_(name), isOpen_(false),
        configPath_(name_ + ".config.xml"),
        edgeTable_(name_ + ".edgetable"),
        dataTable_(name_ + ".datatable")
    {
        if( !load() ) {
            baseNID_ = 0;
            nextNID_ = NodeIDTraits<NodeIDType>::MinValue;
        }
    }

    virtual ~CommonTrie_()
    {
        flush();
    }

    void open()
    {
        if(!isOpen_) {
            edgeTable_.open();
            dataTable_.open();
            isOpen_ = true;
        }
    }

    void flush()
    {
        if(isOpen_) {
            sync();

            edgeTable_.flush();
            dataTable_.flush();
        }
    }

    void close()
    {
        if(isOpen_) {
            flush();
            edgeTable_.close();
            dataTable_.close();
            isOpen_ = false;
        }
    }

    /**
     * @brief Optimize read performance, e.g. get() and findPrefix()
     */
    void optimize()
    {
        if(isOpen_) {
            edgeTable_.optimize();
            dataTable_.optimize();
        }
    }

    /**
     * @brief Insert key value pairs into Trie.
     */
    void insert(const std::vector<CharType>& key, const UserDataType& data)
    {
        if( key.size() == 0) return;

        NodeIDType nid = NodeIDTraits<NodeIDType>::RootValue;
        for( size_t i=0; i< key.size(); i++ )
        {
            NodeIDType tmp = NodeIDType();
            addEdge(key[i], nid, tmp);
            nid = tmp;
        }
        putData(nid, data);
    }

    /**
     * @brief Get value for a given key.
     * @return false if key doesn't exist
     *         true otherwise
     */
    bool get(const std::vector<CharType>& key, UserDataType& data)
    {
        if(key.size() == 0) return false;

        NodeIDType nid = NodeIDTraits<NodeIDType>::RootValue;
        for( size_t i=0; i<key.size(); i++ )
        {
            NodeIDType tmp = NodeIDType();
            if( !getEdge(key[i], nid, tmp) )
                return false;
            nid = tmp;
        }
        return getData(nid,data);
    }

    /**
     * @brief Get a list of values whose keys match the wildcard query. Only "*" and
     * "?" are supported currently, legal input looks like "ea?th", "her*", or "*ear?h".
     * @return true at leaset one result found.
     *         false nothing found.
     */
    bool findRegExp(const std::vector<CharType>& regexp,
        std::vector<std::vector<CharType> >& keyList,
        const int maximumResultNumber)
    {
        std::vector<CharType> prefix;
        keyList.clear();
        std::vector<UserDataType > valueList;

        findRegExp_<EnumerateKey>(regexp, 0, prefix,
            NodeIDTraits<NodeIDType>::RootValue,
            keyList, valueList, maximumResultNumber);
        return keyList.size() ? true : false;
    }

    /**
     * @brief Get a list of values whose keys match the wildcard query. Only "*" and
     * "?" are supported currently, legal input looks like "ea?th", "her*", or "*ear?h".
     * @return true at leaset one result found.
     *         false nothing found.
     */
    bool findRegExp(const std::vector<CharType>& regexp,
        std::vector<std::vector<UserDataType> >& valueList,
        const int maximumResultNumber)
    {
        std::vector<CharType> prefix;
        std::vector<std::vector<CharType> > keyList;
        valueList.clear();

        findRegExp_<EnumerateValue>(regexp, 0, prefix,
            NodeIDTraits<NodeIDType>::RootValue,
            keyList, valueList, maximumResultNumber);
        return valueList.size() ? true : false;
    }

    /**
     * @brief Get a list of values whose keys match the wildcard query. Only "*" and
     * "?" are supported currently, legal input looks like "ea?th", "her*", or "*ear?h".
     * @return true at leaset one result found.
     *         false nothing found.
     */
    bool findRegExp(const std::vector<CharType>& regexp,
        std::vector<std::vector<CharType> >& keyList,
        std::vector<std::vector<UserDataType> >& valueList,
        const int maximumResultNumber)
    {
        std::vector<CharType> prefix;
        keyList.clear();
        valueList.clear();

        findRegExp_<EnumerateBoth>(regexp, 0, prefix,
            NodeIDTraits<NodeIDType>::RootValue,
            keyList, valueList, maximumResultNumber);
        return keyList.size() ? true : false;
    }

    /**
     * @return number of key value pairs inserted.
     */
    unsigned int num_items()
    {
        return dataTable_.numItems();
    }

    /**
     * @brief Set base NodeID.
     */
    void setBaseNID(const NodeIDType base)
    {
        nextNID_ = (nextNID_ - baseNID_) + base;
        baseNID_ = base;

        if(nextNID_ > NodeIDTraits<NodeIDType>::MaxValue)
            throw std::runtime_error("NID rebase out of range");
    }

    /**
     * @brief Get next avaiable NodeID.
     */
    NodeIDType getNextNID() {
        return nextNID_;
    }

    EdgeTableType& getEdgeTable() {
        return edgeTable_;
    }

    DataTableType& getDataTable() {
        return dataTable_;
    }

protected:

    /**
     * Given the NID of parent node and the edge's character property,
     * generate the child node's NID.
     * @param   ch          edge's character property
     *          parentNID   NID of parent node
     *          childNID    NID of child node
     * @return  true    if child node is generated successfully
     *          false   if key exists already
     */
    bool addEdge(const CharType ch, const NodeIDType parentNID, NodeIDType& childNID)
    {
        EdgeTableKeyType etKey(parentNID, ch);

        // if NID is found, just return
        if(edgeTable_.getValue(etKey, childNID))
            return false;

        childNID = nextNID_;
        ++nextNID_;
        if(nextNID_ > NodeIDTraits<NodeIDType>::MaxValue)
            throw std::runtime_error("NID allocation out of range");

        edgeTable_.insertValue(etKey, childNID);
        return true;
    }

    /**
     * Given the NID of parent node and the edge's character property,
     * find the child node's NID.
     * @param   ch          edge's character property
     *          parentNID   NID of parent node
     *          childNID    NID of child node
     * @return  true    if successfully
     *          false   if child does not exist yet
     */
    bool getEdge(const CharType& ch, const NodeIDType& parentNID, NodeIDType& childNID)
    {
        EdgeTableKeyType etKey(parentNID, ch);
        NodeIDType value = NodeIDType();

        // Return false if edge doesn't exist
        if(!edgeTable_.getValue(etKey, value))
            return false;

        childNID = value;
        return true;
    }

    /**
     * Store key's nid and userdata into DataTable.
     * @return true     successfully
     *         false    key exists already
     */
    void putData( const NodeIDType& nid, const UserDataType& userData)
    {
        dataTable_.insertValue(nid,userData);
    }

    /**
     * Retrieve userdata stored in DataTable by given key.
     * @return true     successfully
     *         false    given key does not exist
     */
    bool getData( const NodeIDType& nid, UserDataType& userData)
    {
        return dataTable_.getValue(nid, userData);
    }

    template<FindRegexpParameterType EnumerateType>
    void findRegExp_(const std::vector<CharType>& regexp, const size_t& startPos,
        std::vector<CharType>& prefix, const NodeIDType& nid,
        std::vector<std::vector<CharType> >& keyList,
        std::vector<UserDataType >& valueList,
        int maximumResultNumber)
    {
        if(keyList.size() >= (size_t)maximumResultNumber)
            return;

        if(startPos == regexp.size())
        {
            UserDataType tmp = UserDataType();
            if(getData(nid, tmp)) {
                if(EnumerateType == EnumerateKey || EnumerateType == EnumerateBoth) {
                    keyList.push_back(prefix);
                }
                if(EnumerateType == EnumerateValue || EnumerateType == EnumerateBoth) {
                    valueList.push_back(tmp);
                }
            }
            return;
        }

        // std::cout << regexp[startPos] << "," << nid << std::endl;

        switch( regexp[startPos] )
        {
            case 42:    //"*"
            {
                EdgeTableKeyType minKey(nid, NumericTraits<CharType>::MinValue);
                EdgeTableKeyType maxKey(nid, NumericTraits<CharType>::MaxValue);

                std::vector<EdgeTableRecordType> result;
                edgeTable_.getValueBetween(result, minKey, maxKey);

                for(size_t i = 0; i <result.size(); i++ ) {
                    prefix.push_back(result[i].key.second);
                    findRegExp_<EnumerateType>(regexp, startPos, prefix, result[i].value,
                        keyList, valueList, maximumResultNumber);
                    prefix.pop_back();
                }

                findRegExp_<EnumerateType>(regexp, startPos+1, prefix, nid,
                    keyList, valueList, maximumResultNumber);
                break;
            }
            case 63:    //"?"
            {
                EdgeTableKeyType minKey(nid, NumericTraits<CharType>::MinValue);
                EdgeTableKeyType maxKey(nid, NumericTraits<CharType>::MaxValue);

                std::vector<EdgeTableRecordType> result;
                edgeTable_.getValueBetween(result, minKey, maxKey);

                for(size_t i = 0; i <result.size(); i++ ) {
                    prefix.push_back(result[i].key.second);
                    findRegExp_<EnumerateType>(regexp, startPos+1, prefix, result[i].value,
                        keyList, valueList, maximumResultNumber);
                    prefix.pop_back();
                }
                break;
            }
            default:
            {
                EdgeTableKeyType key(nid, regexp[startPos]);
                NodeIDType nxtNode = NodeIDType();
                if( edgeTable_.getValue(key, nxtNode) ) {
                    prefix.push_back(regexp[startPos]);
                    findRegExp_<EnumerateType>(regexp, startPos+1, prefix, nxtNode,
                        keyList, valueList, maximumResultNumber);
                    prefix.pop_back();
                }
                break;
            }
        }
    }

protected:

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & boost::serialization::make_nvp("BaseNodeID", baseNID_);
        ar & boost::serialization::make_nvp("NextNodeID", nextNID_);
    }


    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar & boost::serialization::make_nvp("BaseNodeID", baseNID_);
        ar & boost::serialization::make_nvp("NextNodeID", nextNID_);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    bool load()
    {
	    ifstream ifs(configPath_.c_str());
        if( ifs ) {
            try {
                boost::archive::xml_iarchive xml(ifs);
                xml >> boost::serialization::make_nvp("PartitionTrie", *this);
            } catch (...) {
                throw std::runtime_error("PartitionTrie config file corrputed");
            }
            ifs.close();
            return true;
        }
        return false;
    }

    void sync()
    {
            ofstream ofs(configPath_.c_str());
            boost::archive::xml_oarchive xml(ofs);
            xml << boost::serialization::make_nvp("PartitionTrie", *this);
            ofs.flush();
    }

private:

    std::string name_;

    bool isOpen_;

    std::string configPath_;

    EdgeTableType edgeTable_;

    DataTableType dataTable_;

    NodeIDType baseNID_;

    NodeIDType nextNID_;
};

template <typename StringType,
          typename UserDataType,
          typename NodeIDType,
          typename EdgeTableType,
          typename DataTableType>
class CommonTrie
{
    typedef typename StringType::value_type CharType;
    typedef CommonTrie<StringType, UserDataType, NodeIDType,
        EdgeTableType, DataTableType> ThisType;
    typedef CommonTrie_<CharType, UserDataType, NodeIDType,
        EdgeTableType, DataTableType> CommonTrieType;

public:

    CommonTrie(const std::string name)
    :   trie_(name) {}

    virtual ~CommonTrie(){}

    void open(){ trie_.open(); }
    void flush(){ trie_.flush(); }
    void close(){ trie_.close(); }
    void optimize(){ trie_.optimize(); }

    void insert(const StringType& key, const UserDataType& data = UserDataType())
    {
        CharType* chArray = (CharType*)key.c_str();
        size_t chCount = key.length();
        std::vector<CharType> chVector(chArray, chArray+chCount);
        trie_.insert(chVector, data);
    }

    bool get(const StringType& key, const UserDataType& data = UserDataType())
    {
        CharType* chArray = (CharType*)key.c_str();
        size_t chCount = key.length();
        std::vector<CharType> chVector(chArray, chArray+chCount);
        return trie_.get(chVector, data);
    }

    bool findRegExp(const StringType& regexp,
        std::vector<StringType>& keyList,
        int maximumResultNumber = 100)
    {
        CharType* chArray = (CharType*)regexp.c_str();
        size_t chCount = regexp.length();
        std::vector<CharType> chVector(chArray, chArray+chCount);

        std::vector< std::vector<CharType> > resultList;
        if( trie_.findRegExp(chVector, resultList, maximumResultNumber) ) {
            for(size_t i = 0; i< resultList.size(); i++ )
            {
                StringType tmp(resultList[i].begin(), resultList[i].end() );
                keyList.push_back(tmp);
            }
            return true;
        }
        return false;
    }

    bool findRegExp(const StringType& regexp,
        std::vector<UserDataType>& valueList,
        int maximumResultNumber = 100)
    {
        CharType* chArray = (CharType*)regexp.c_str();
        size_t chCount = regexp.length();
        std::vector<CharType> chVector(chArray, chArray+chCount);

        return trie_.findRegExp(chVector, valueList, maximumResultNumber);
    }

    bool findRegExp(const StringType& regexp,
        std::vector<StringType>& keyList,
        std::vector<UserDataType>& valueList,
        int maximumResultNumber = 100)
    {
        CharType* chArray = (CharType*)regexp.c_str();
        size_t chCount = regexp.length();
        std::vector<CharType> chVector(chArray, chArray+chCount);

        std::vector< std::vector<CharType> > resultList;
        if( trie_.findRegExp(chVector, resultList, valueList,
            maximumResultNumber) )
        {
            for(size_t i = 0; i< resultList.size(); i++ )
            {
                StringType tmp(resultList[i].begin(), resultList[i].end() );
                keyList.push_back(tmp);
            }
            return true;
        }
        return false;
    }

    void setBaseNID(const NodeIDType base) {
        trie_.setBaseNID(base);
    }

    NodeIDType getNextNID() {
        return trie_.getNextNID();
    }

    EdgeTableType& getEdgeTable() {
        return trie_.getEdgeTable();
    }

    DataTableType& getDataTable() {
        return trie_.getDataTable();
    }

    unsigned int num_items() {
        return trie_.num_items();
    }

private:

    CommonTrieType trie_;
};

NS_IZENELIB_AM_END

#endif
