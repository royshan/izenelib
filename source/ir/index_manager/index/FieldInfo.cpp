#include <ir/index_manager/index/FieldInfo.h>

using namespace std;
using namespace boost;

using namespace izenelib::ir::indexmanager;

FieldsInfo::FieldsInfo()
        :ppFieldsInfo(NULL)
        ,nNumFieldInfo(0)
        ,fdInfosIterator(0)
{
}
FieldsInfo::FieldsInfo(const FieldsInfo& src)
        :fdInfosIterator(0)
{
    nNumFieldInfo = src.nNumFieldInfo;
    ppFieldsInfo = new FieldInfo*[nNumFieldInfo];
    for (int32_t i = 0;i<nNumFieldInfo;i++)
    {
        ppFieldsInfo[i] = new FieldInfo(*(src.ppFieldsInfo[i]));
        fdInfosByName.insert(make_pair(ppFieldsInfo[i]->name.c_str(),ppFieldsInfo[i]));
    }
}

FieldsInfo::~FieldsInfo()
{
    clear();
}

void FieldsInfo::setSchema(const IndexerCollectionMeta& collectionMeta)
{
    clear();

    nNumFieldInfo = 0;
    std::set<IndexerPropertyConfig> schema = collectionMeta.getDocumentSchema();

    for(std::set<IndexerPropertyConfig>::const_iterator it = schema.begin(); it != schema.end(); it++ )
	if (it->getPropertyId() != BAD_PROPERTY_ID)
           nNumFieldInfo++;

    ppFieldsInfo = new FieldInfo*[nNumFieldInfo];
    memset(ppFieldsInfo,0,nNumFieldInfo*sizeof(FieldInfo*));

    int32_t n = 0;
    for(std::set<IndexerPropertyConfig>::const_iterator it = schema.begin(); it != schema.end(); it++ )
    {
	if (it->getPropertyId() != BAD_PROPERTY_ID)
	{
	    ppFieldsInfo[n] = new FieldInfo(it->getPropertyId(),
                                                           it->getName().c_str(),
                                                           it->isForward(),
                                                           it->isIndex());
           ppFieldsInfo[n]->setColID(colId);
           fdInfosByName.insert(make_pair(ppFieldsInfo[n]->getName(),ppFieldsInfo[n]));
           n++;
	}
    }
}

void FieldsInfo::addField(FieldInfo* pFieldInfo)
{
    FieldInfo* pInfo = getField(pFieldInfo->getName());
    if (!pInfo)
    {
        pInfo = new FieldInfo(*pFieldInfo);
        FieldInfo** ppFieldInfos = new FieldInfo*[nNumFieldInfo+1];
        for (int32_t i = 0;i<nNumFieldInfo;i++)
        {
            ppFieldInfos[i] = ppFieldsInfo[i];
        }
        ppFieldInfos[nNumFieldInfo] = pInfo;

        delete[] ppFieldsInfo;
        ppFieldsInfo = ppFieldInfos;
        nNumFieldInfo++;

        fdInfosByName.insert(pair<string,FieldInfo*>(pInfo->getName(),pInfo));
    }
}

void FieldsInfo::read(IndexInput* pIndexInput)
{
    try
    {
        clear();
        int32_t count = pIndexInput->readInt(); ///<FieldsCount(Int32)>

        if (count <= 0)
        {
            SF1V5_LOG(level::warn) << "FieldsInfo::read():field count <=0." << SF1V5_ENDL;
            return ;
        }
        nNumFieldInfo = count;
        ppFieldsInfo = new FieldInfo*[count];
        memset(ppFieldsInfo,0,count*sizeof(FieldInfo*));

        string str;
        FieldInfo* pInfo = NULL;
        for (int32_t i = 0;i<count;i++)
        {
            pInfo = new FieldInfo();
            pInfo->setID(i);
            pInfo->setColID(colId);
            pIndexInput->readString(str);	///<FieldName(String)>
            pInfo->setName(str.c_str());
            pInfo->setFieldFlag(pIndexInput->readByte()); //IsIndexed(Bool) and IsForward(Bool)
            if (pInfo->isIndexed()&&pInfo->isForward())
            {
                pInfo->setDistinctNumTerms(pIndexInput->readLong());
                pInfo->setIndexOffset(pIndexInput->readLong());
                pInfo->vocLength = pIndexInput->readLong();
                pInfo->dfiLength = pIndexInput->readLong();
                pInfo->ptiLength = pIndexInput->readLong();

                cout<<"FieldInfo:"<<"indexoffset "<<pInfo->getIndexOffset()<<" distinctnumterms "<<pInfo->distinctNumTerms()<<" voclength "<<pInfo->vocLength<<" dfilength "<<pInfo->dfiLength<<" ptilength "<<pInfo->ptiLength<<endl;
            }

            ppFieldsInfo[i] = pInfo;
            fdInfosByName.insert(pair<string,FieldInfo*>(pInfo->getName(),pInfo));
        }

    }
    catch (const IndexManagerException& e)
    {
        SF1V5_RETHROW(e);
    }
    catch (const bad_alloc& )
    {
        SF1V5_THROW(ERROR_OUTOFMEM,"FieldsInfo.read():alloc memory failed.");
    }
    catch (...)
    {
        SF1V5_THROW(ERROR_UNKNOWN,"FieldsInfo::read failed.");
    }
}

void FieldsInfo::write(IndexOutput* pIndexOutput)
{
    try
    {
        pIndexOutput->writeInt(nNumFieldInfo);///<FieldsCount(Int32)>
        FieldInfo* pInfo;
        for (int32_t i = 0;i<nNumFieldInfo;i++)
        {
            pInfo = ppFieldsInfo[i];

            pIndexOutput->writeString(pInfo->getName());	///<FieldName(String)>
            pIndexOutput->writeByte(pInfo->getFieldFlag());		///<IsIndexed(Bool) and IsForward(Bool)>(Byte)
            if (pInfo->isIndexed()&&pInfo->isForward())
            {
                pIndexOutput->writeLong(pInfo->distinctNumTerms());
                pIndexOutput->writeLong(pInfo->getIndexOffset());
                pIndexOutput->writeLong(pInfo->vocLength);
                pIndexOutput->writeLong(pInfo->dfiLength);
                pIndexOutput->writeLong(pInfo->ptiLength);
            }

        }
    }
    catch (const IndexManagerException& e)
    {
        SF1V5_RETHROW(e);
    }
    catch (const bad_alloc& )
    {
        SF1V5_THROW(ERROR_OUTOFMEM,"FieldsInfo.write():alloc memory failed.");
    }
    catch (...)
    {
        SF1V5_THROW(ERROR_UNKNOWN,"FieldsInfo.write() failed.");
    }
}
void FieldsInfo::clear()
{
    for (int32_t i = 0;i<nNumFieldInfo;i++)
    {
        if (ppFieldsInfo[i])
            delete ppFieldsInfo[i];
        ppFieldsInfo[i] = NULL;
    }
    if (ppFieldsInfo)
    {
        delete[] ppFieldsInfo;
        ppFieldsInfo = NULL;
    }
    nNumFieldInfo = 0;

    fdInfosByName.clear();
}

void FieldsInfo::reset()
{
    for (int32_t i = 0;i<nNumFieldInfo;i++)
    {
        ppFieldsInfo[i]->reset();
    }
}

void FieldsInfo::setFieldOffset(fieldid_t fid,fileoffset_t offset)
{
    ppFieldsInfo[fid]->setIndexOffset(offset);
}
fileoffset_t FieldsInfo::getFieldOffset(fieldid_t fid)
{
    return ppFieldsInfo[fid]->getIndexOffset();
}

void FieldsInfo::setDistinctNumTerms(fieldid_t fid,uint64_t distterms)
{
    ppFieldsInfo[fid]->setDistinctNumTerms(distterms);
}

uint64_t FieldsInfo::distinctNumTerms(fieldid_t fid)
{
    return ppFieldsInfo[fid]->distinctNumTerms();
}

fieldid_t FieldsInfo::getFieldID(const char* fname)
{
    map<string,FieldInfo*>::iterator iter = fdInfosByName.find(fname);
    if (iter != fdInfosByName.end())
    {
        return iter->second->getID();
    }
    return -1;
}
FieldInfo* FieldsInfo::getField(const char* field)
{
    string tmp(field);
    map<string,FieldInfo*>::iterator iter = fdInfosByName.find(tmp);
    if (iter != fdInfosByName.end())
    {
        return iter->second;
    }
    return NULL;
}

