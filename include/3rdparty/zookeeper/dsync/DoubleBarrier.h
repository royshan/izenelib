/**
 * @file DoubleBarrier.h
 * @author Zhongxia Li
 * @date Sep 8, 2011
 * @brief 
 */
#ifndef DOUBLE_BARRIER_H_
#define DOUBLE_BARRIER_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperWatcher.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace zookeeper {

class SyncPrimitive : public ZooKeeperEventHandler
{
public:
    SyncPrimitive(const std::string& host);

    virtual ~SyncPrimitive();

    std::string root_;

    static std::auto_ptr<ZooKeeper> zk_;
    static boost::mutex mutex_;
    static boost::condition_variable condition_;
};

class Barrier : public SyncPrimitive
{
public:
    Barrier(const std::string& host, const std::string& zpath, size_t size, const std::string& name);

    virtual ~Barrier();

    bool enter();

    bool leave();

    virtual void process(ZooKeeperEvent& zkEvent);

private:
    std::string name_;
    size_t size_;
};

}

#endif /* DOUBLE_BARRIER_H_ */
