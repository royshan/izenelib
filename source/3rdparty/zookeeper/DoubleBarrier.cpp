#include <3rdparty/zookeeper/DoubleBarrier.h>

#include <unistd.h>

using namespace zookeeper;


std::auto_ptr<ZooKeeper> SyncPrimitive::zk_;
boost::mutex SyncPrimitive::mutex_;
boost::condition_variable SyncPrimitive::condition_;

SyncPrimitive::SyncPrimitive(const std::string& host)
{
    if (!zk_.get())
    {
        zk_.reset(new ZooKeeper(host, 2000));
        zk_->registerEventHandler(this);
    }
}

SyncPrimitive::~SyncPrimitive()
{
}


DoubleBarrier::DoubleBarrier(const std::string& host, const std::string& root, size_t size, const std::string& name)
: SyncPrimitive(host), size_(size)
{
    if (zk_.get())
    {
        if (!zk_->isZNodeExists(root))
        {
           zk_->createZNode(root, "barrier");
        }
    }

    root_ = root;

    //char name[32];
    //gethostname(name, sizeof name);
    name_ = root+"/"+name;
}

DoubleBarrier::~DoubleBarrier()
{
}

bool DoubleBarrier::enter()
{
    zk_->createZNode(name_, "");

    while(true)
    {
        boost::unique_lock<boost::mutex> ulock(mutex_);
        std::vector<std::string> childrenList;
        zk_->getZNodeChildren(root_, childrenList);

        if (childrenList.size() < size_) {
            condition_.wait(ulock);
        }
        else {
            return true;
        }
    }

    return true;
}

bool DoubleBarrier::leave()
{
    zk_->deleteZNode(name_);

    while (true)
    {
        boost::unique_lock<boost::mutex> ulock(mutex_);
        std::vector<std::string> childrenList;
        zk_->getZNodeChildren(root_, childrenList);

        if (childrenList.size() > 0) {
            condition_.wait(ulock);
        }
        else {
            return true;
        }
    }

    return false;
}

void DoubleBarrier::process(ZooKeeperEvent& zkEvent)
{
    std::cout << "notified!"<<std::endl;
    condition_.notify_all();
}


