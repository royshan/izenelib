#include <net/distribute/DataTransfer.h>

#include <fstream>
#include <string.h>
#include <signal.h>
#include <boost/filesystem.hpp>

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace net{
namespace distribute{

DataTransfer::DataTransfer(const std::string& hostname, unsigned int port, buf_size_t bufSize)
: serverAddr_(hostname, port), bufSize_(bufSize)
{
    buf_ = new char[bufSize];
    assert(buf_);
    memset(buf_, 0, bufSize);

    socketIO_.Connect(hostname, port);
}

DataTransfer::~DataTransfer()
{
    socketIO_.Close();

    delete[] buf_;
}

int
DataTransfer::syncSend(const std::string& src, const std::string& curDirName, bool isRecursively)
{
    if (!bfs::exists(src))
    {
        std::cout<<"Not exists: "<<src<<std::endl;
        return -1;
    }

    // src is a file
    if (!bfs::is_directory(src))
    {
        bfs::path path(src);
        std::string curFileDir = curDirName.empty() ? path.parent_path().filename() : curDirName;
        return syncSendFile(src, curFileDir);
    }

    // src is a directory
    bfs::path path(processPath(src));
    //std::cout<<path.string()<<std::endl;
    std::string curFileDir = curDirName.empty() ? path.filename() : curDirName; // rename dir to dirName
    //std::cout<<"[DataTransfer] dir: "<<curFileDir<<std::endl;

    int ret = 0;
    int sent = 0;
    bfs::directory_iterator iterEnd;
    for (bfs::directory_iterator iter(path); iter != iterEnd; iter++)
    {
        if (bfs::is_regular_file(iter->path()))
        {
            //std::cout<<iter->path().filename()<<std::endl;
            sent = syncSendFile(iter->path().string(), curFileDir);
            ret += sent;
        }
        else if (isRecursively && bfs::is_directory(iter->path()))
        {
            std::string subDir = iter->path().string();
            sent = syncSendDirRecur(subDir, curFileDir);
            ret += sent;
        }
    }

    return ret;
}

int
DataTransfer::copy(const std::string& src, const std::string& dest, bool isRecursively)
{
    //bfs::copy_file();
    return 0;
}

/// private ////////////////////////////////////////////////////////////////////
int
DataTransfer::syncSendDirRecur(const std::string& curDir, const std::string& parentDir)
{
    bfs::path path(curDir);
    std::string curFileDir = parentDir + "/" + path.filename();
    //std::cout<<"[DataTransfer] dir: "<<curFileDir<<std::endl; //xxx

    int ret = 0;
    int sent = 0;
    bfs::directory_iterator iterEnd;
    for (bfs::directory_iterator iter(curDir); iter != iterEnd; iter++)
    {
        if (bfs::is_regular_file(iter->path()))
        {
            //std::cout<<iter->path().filename()<<std::endl;//xxx
            sent = syncSendFile(iter->path().string(), curFileDir);
            ret += sent;
        }
        else if (bfs::is_directory(iter->path()))
        {
            std::string subDir = iter->path().string();
            sent = syncSendDirRecur(subDir, curFileDir);
            ret += sent;
        }
    }

    return ret;
}

int
DataTransfer::syncSendFile(const std::string& fileName, const std::string& curDir)
{
    if (bfs::is_directory(fileName))
    {
        std::cout<<"Is a directory: "<<fileName<<std::endl;
        return -1;
    }

    std::ifstream ifs;
    ifs.open(fileName.c_str());
    if (!ifs.is_open())
    {
        std::cout<<"Failed to open: "<<fileName<<std::endl;
        return -1;
    }

    LOG(INFO)<<"Transferring "<<fileName<<" to remote dir "<<curDir;

    if (!socketIO_.isGood())
    {
        LOG(ERROR)<<"socket error";
        return -1;
    }

    // don't terminate when server broken, xxx check error
    signal(SIGPIPE, SIG_IGN);

    struct timeval timeout = {180,0};

    /// send head
    SendFileReqMsg msg;
    bfs::path path(fileName);
    msg.setFileType(SendFileReqMsg::FTYPE_SCD);
    msg.setFileName(curDir+"/"+path.filename());
    msg.setFileSize(bfs::file_size(fileName));

    std::string msg_head = msg.toString();
    int nsend = socketIO_.syncSend(msg_head.c_str(), msg_head.size());
    LOG(INFO)<<"Sent header size "<<nsend<<" - "<<msg_head;

    if (nsend < msg_head.size())
    {
        LOG(ERROR)<<"Failed to send file header info";
        return -1;
    }

    /// check if ready to receive
    int nrecv;
    ResponseMsg resMsg;
    nrecv = socketIO_.syncRecv(buf_, bufSize_, timeout);
    resMsg.loadMsg(std::string(buf_, nrecv));
    //LOG(INFO)<<"Ready? "<<std::string(buf_, nrecv);
    if (nrecv >0 && resMsg.getStatus() != "success")
    {
        LOG(ERROR)<<"Receiver not ready";
        return -1;
    }
    LOG(INFO)<<"Ready to send file data";

    /// send data
    std::streamsize readLen, sendLen, totalLen = 0;
    ifs.read(buf_, bufSize_);
    while ((readLen = ifs.gcount()) > 0)
    {
        if ((sendLen = socketIO_.syncSend(buf_, readLen)) < readLen)
        {
            ifs.close();
            LOG(ERROR)<<"Failed to send file data";
            return -1;
        }
        //std::cout<<"read "<<readLen<<", sent "<<sendLen<<std::endl;
        totalLen += sendLen;
        ifs.read(buf_, bufSize_);
    }
    ifs.close();
    LOG(INFO)<<"Sent data size "<<totalLen;

    /// check receive status
    nrecv = socketIO_.syncRecv(buf_, bufSize_, timeout);
    if (nrecv > 0)
    {
        resMsg.loadMsg(std::string(buf_, nrecv));
        //LOG(INFO)<<"receiver status? "<<std::string(buf_, nrecv);
        // error info?
        unsigned int nrecved = resMsg.getReceivedSize();

        if (nrecved < totalLen)
        {
            // xxx, retry?
            LOG(ERROR)<<"Receiver received incompleted data, received"
                      <<nrecved<<", total"<<totalLen;
            return -1;
        }

        // transfer succeeded
        LOG(INFO)<<"Transfer succeed!";
        return 0;
    }
    else
    {
        LOG(ERROR)<<"Failed to get Receiver status";
        return -1;
    }
}

//int
//DataTransfer::syncSend(const char* buf, buf_size_t bufLen)
//{
//    return socketIO_.syncSend(buf, bufLen);
//}

std::string DataTransfer::processPath(const std::string& path)
{
    std::string ret = path;
    size_t n = ret.size();

    // remove tailing '/'
    if (n > 0 && ret[n-1] == '/')
    {
        ret.erase(n-1, n);
    }

    return ret;
}

}} // namespace

