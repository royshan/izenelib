///
///  @file : MessageClientFull.cpp
///  @date : 8/5/2008
///  @author : TuanQuang Nguyen
///
//
//
/**************** Include module header files *******************/
#include <net/message-framework/MessageClientFull.h>
#include <net/message-framework/MessageFrameworkConfiguration.h>
#include <net/message-framework/ServiceMessage.h>
#include <net/message-framework/PermissionOfServiceMessage.h>
#include <net/message-framework/MessageType.h>
#include <net/message-framework/ClientIdRequestMessage.h>

/**************** Include boost header files *******************/
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

/**************** Include std header files *******************/
#include <iostream>
#include <sstream>

using namespace std;

namespace messageframework {

/***************************************************************************
 Description: construct a MessageClientFull given the client name and
 information of controller. The MessageClientFull will connects to controller.
 Input:
 ioservice - main io service
 clientName - name of client
 controller - information of controller
 ***************************************************************************/
MessageClientFull::MessageClientFull(const std::string& clientName,
		const MessageFrameworkNode& controllerInfo) :
	messageDispatcher_(this, this, this), asyncConnector_(this, io_service_) {
	ownerManager_ = clientName;

	//  information of service that retrieves permission of service
	servicePermisionServer_ = controllerInfo;

	// timeout is 1 second
	timeOutMilliSecond_ = TIME_OUT_IN_MILISECOND;
	batchProcessedRequestNumber_ = 128;
	//batchProcessedRequestNumber_ = 1;
	//batchProcessedRequestNumber_ = 32;
	controllerNode_.nodeIP_ = getHostIp(io_service_, controllerInfo.nodeIP_);
	controllerNode_.nodePort_ = controllerInfo.nodePort_;
	controllerNode_.nodeName_ = controllerInfo.nodeName_;

	connectionToControllerEstablished_ = false;
	asyncConnector_.connect(controllerInfo.nodeIP_, controllerInfo.nodePort_);

	// create thread for I/O operations
	ioThread_ = new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_));

	// waiting until connection to controller is established
	boost::mutex::scoped_lock connectionToControllerEstablishedLock(
			connectionToControllerEstablishedMutex_);
	boost::system_time const timeout = boost::get_system_time()
			+ boost::posix_time::milliseconds(timeOutMilliSecond_);
	// there should not be any other thread waiting on connectionToControllerEvent_
	if (!connectionToControllerEstablished_
			&& !connectionToControllerEvent_.timed_wait(
					connectionToControllerEstablishedLock, timeout))
		throw MessageFrameworkException(SF1_MSGFRK_CONNECTION_TIMEOUT, __LINE__, __FILE__);
}

/***************************************************************************
 Description: destructor, destroy variables it if is neccessary
 ***************************************************************************/
MessageClientFull::~MessageClientFull() {
	asyncConnector_.shutdown();
	ioThread_->join();
	delete ioThread_;
}

/****************************************************************************
 Input:
 node - remote node
 Return:
 true - connection has already been established or built successfully.
 false - if the connection to remote node cannot be established in the given time.
 Description: This function check connection to remote node, if connection
 havn't been established, this function will try to prepare the connection.
 ****************************************************************************/
bool MessageClientFull::prepareConnection(const MessageFrameworkNode& node) {
	// connection to controller has not been established
	if (!connectionToControllerEstablished_)
		return false;

	if (!messageDispatcher_.isExist(node)) {
		// if connection to sever has not been established, connect to the server;
		boost::system_time const timeout = boost::get_system_time()
				+ boost::posix_time::milliseconds(timeOutMilliSecond_);
		boost::mutex::scoped_lock connectionToServerEstablishedLock(
				connectionToServerEstablishedMutex_);

		asyncConnector_.connect(node.nodeIP_, node.nodePort_);

		// maybe, the connection has been established before timed_wait
		while (!messageDispatcher_.isExist(node)) {
			if ( !connectedToServerEvent_.timed_wait(
					connectionToServerEstablishedLock, timeout)) {
				std::cout << "Cannot connect to " << node.nodeIP_;
				std::cout << ":" << node.nodePort_ << std::endl;
				return false;
			}
		}
	}

	return true;
}

/***************************************************************************
 Discription: This function generate request id
 ****************************************************************************/
const unsigned int MessageClientFull::generateRequestId() {
	unsigned int ret = 0;
	//int clientId;
	//if( getClientId(clientId) )
	{
		//ret = clientId;
		boost::mutex::scoped_lock lock(nxtSequentialNumberMutex_);
		if (nxtSequentialNumber_ == MF_FULL_MAX_SEQUENTIAL_NUMBER)
			nxtSequentialNumber_ = 0;
		//ret <<= MF_FULL_REQUEST_ID_CLIENT_ID_SHIFT;
		ret |= (nxtSequentialNumber_& MF_FULL_MASK_SEQUENTIAL_NUMBER);
		nxtSequentialNumber_ ++;
	}
	return ret+1;
}

/****************************************************************************
 Output:
 clientId - controller assigned ID
 Return:
 true - ID is got successfully
 false - MessageController is busy.
 Description: This function gets an ID from MessageController. This ID will be
 used combined with a sequential number to generate requestIds.
 ****************************************************************************/
/*	bool MessageClientFull::getClientId(int& clientId)
 {     
 clientId = nxtSequentialNumber_;
 nxtSequentialNumber_ ++;
 return clientId+1;
 
 if(clientId_ != 0)
 {   
 clientId = clientId_;
 return true;
 }

 try
 {
 if(!connectionToControllerEstablished_)
 return false;

 sendClientIdRequest();

 boost::mutex::scoped_lock clientIdLock(clientIdMutex_);
 boost::system_time const timeout = boost::get_system_time()
 + boost::posix_time::milliseconds(timeOutMilliSecond_);
 while(clientId_ == 0)
 {
 if(!clientIdEvent_.timed_wait(clientIdLock, timeout))
 {
 std::cout << "[Client:" << getName() ;
 std::cout << "] Timeout!!! Cannot get client id " << std::endl;
 return false;
 }
 }
 clientIdLock.unlock();

 if(clientId_ != 0)
 {
 clientId = clientId_;
 return true;
 }

 // it's strange to reach here
 throw MessageFrameworkException(SF1_MSGFRK_UNKNOWN_ERROR, __LINE__, __FILE__);
 }
 catch(MessageFrameworkException& e)
 {
 e.output(std::cerr);
 }
 catch(boost::system::error_code& e)
 {
 std::cerr << e << std::endl;
 }
 catch(std::exception& e)
 {
 std::cerr << "Exception: " << e.what() << std::endl;
 }
 return false;
 }*/

/******************************************************************************
 Input:
 serviceName - the service name
 Output:
 servicePermissionInfo - permission information of the service. If
 the service is available, it contains information of the server.
 Return:
 true - Available to request service
 false - the MessageController is busy
 Description: This function gets a permission of the given service from
 MessageController. If MessageController is busy, this function
 returns false immediately. Then, the MessageClientFull has to call this function again
 ******************************************************************************/
bool MessageClientFull::getPermissionOfService(const std::string& serviceName,
		ServicePermissionInfo& servicePermissionInfo) {
	try
	{
		std::map<std::string, ServicePermissionInfo>::const_iterator iter;

		// connection has not been established
		if(!connectionToControllerEstablished_)
		return false;

#ifdef _LOGGING_
		WriteToLog("log.log", "============= MessageClient::getPermissionOfService ===========");
#endif
		sendPermissionOfServiceRequest(serviceName);

		boost::system_time const timeout = boost::get_system_time()
		+ boost::posix_time::milliseconds(timeOutMilliSecond_);

		boost::mutex::scoped_lock acceptedPermissionLock(acceptedPermissionMutex_);

		while(acceptedPermissionList_.find(serviceName) == acceptedPermissionList_.end())
		{
			if(!newPermisionOfServiceEvent_.timed_wait(acceptedPermissionLock, timeout))
			{
				std::cout << "[Client:" << getName();
				std::cout << "] Timeout!!! Cannot receive permission of Service: " << serviceName << std::endl;

#ifdef _LOGGING_
				WriteToLog("log.log", "getPermissionOfService returns false (timeout)");
#endif
				return false;
			}
		}

		// search for the permission in the list
		iter = acceptedPermissionList_.find(serviceName);
		if(iter != acceptedPermissionList_.end())
		{
			servicePermissionInfo = iter->second;
			acceptedPermissionList_.erase(serviceName);

			if(servicePermissionInfo.getPermissionFlag() == UNKNOWN_PERMISSION_FLAG)
			{
				std::cout << "[Client:" << getName();
				std::cout << "] Service " << serviceName << " is not listed in Message Controller." << std::endl;

#ifdef _LOGGING_
				WriteToLog("log.log", "getPermissionOfService returns false(service is not available at MessageController)");
#endif
				// service is not available at MessageController
				return false;
			}

			acceptedPermissionLock.unlock();

			//				// make connection in advanced
			//				if(!messageDispatcher_.isExist(servicePermissionInfo.getServer()))
			//				{
			//					// if connection to sever has not been established, connect to the server;
			//					asyncConnector_.connect(servicePermissionInfo.getServer().nodeIP_,
			//						servicePermissionInfo.getServer().nodePort_);
			//				}

#ifdef _LOGGING_
			WriteToLog("log.log", "getPermissionOfService returns true(newly received)");
#endif
			return true;
		}

		// it's strange to reach here
		throw MessageFrameworkException(SF1_MSGFRK_UNKNOWN_ERROR, __LINE__, __FILE__);
	}
	catch(MessageFrameworkException& e)
	{
		e.output(std::cerr);
	}
	catch(boost::system::error_code& e)
	{
		std::cerr << e << std::endl;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return false;
}

/******************************************************************************
 Input:
 servicePermissionInfo - it contains information of service name and the server
 serviceRequestInfo - information about request service. It contains the
 service name and its parameter values.
 Description: This function is called when we want to request result of service.
 The MessageClientFull will sends the request to either MessageController or
 MessageServer.
 ******************************************************************************/
bool MessageClientFull::putServiceRequest(
		const ServicePermissionInfo& servicePermissionInfo,
		ServiceRequestInfoPtr& serviceRequestInfo) {
	if ( !prepareConnection(servicePermissionInfo.getServer()) )
		return false;

	// Following is modified by Wei Cao, 2009-02-17
	// 1. create a semaphore for each request that need a reply.
	// 2. insert pair <requestid, semaphore into the hash map semaphoreTable_.
	// 3. then send request to MessageServer
	//
	// So when a reply arrives, it can lookup the semaphore associated with the request
	// by look up hash table semaphoreTable_, then release the semaphore to wake up any
	// thread waiting on it (the thread who calls getResultService).
	//
	// The reason I don't adopt boost::interprocess::interprocess_semaphore is the boost
	// semaphore is too costly, a test shows 5000 pairs of post() and wait() operations
	// on a boost semaphore costs nearly 10 seconds, so I write a light-weight version
	// instead, it costs only 0.3 seconds with the same test.

	std::string serviceName = serviceRequestInfo->getServiceName();
	unsigned int requestId = generateRequestId();

	//if(requestId == 0)
	if (0) {
		std::cout << "[Client:" << getName() ;
		std::cout << "]  generate a request id: "<<requestId << std::endl;
		//return false;
	}

	serviceRequestInfo->setRequestId(requestId);
	// minor id ranges from 1, a minor id = 0 indicates there is only one
	// request corresponding to the request id.
	serviceRequestInfo->setMinorId(0);
	//serviceRequestInfo.setServiceResultFlag(
	//servicePermissionInfo.getServiceResultFlag());

	/*std::vector<boost::shared_ptr<VariantType> > requestList;
	 boost::shared_ptr<ServiceRequestInfo> request(
	 new ServiceRequestInfo(serviceRequestInfo) );
	 boost::shared_ptr<VariantType> tmp( new VariantType() );
	 tmp->putCustomData(request);
	 requestList.push_back(tmp);*/

	if (servicePermissionInfo.getServiceResultFlag() != SERVICE_WITHOUT_RESULT) {
		// prepare semaphore
		{
			boost::shared_ptr<semaphore_type> semaphore = boost::shared_ptr<semaphore_type>(new semaphore_type(0));
				boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
				semaphoreTable_[requestId] = semaphore;
			}
			// prepare result list
			{
				boost::mutex::scoped_lock resultTableLock(serviceResultMutex_);
				serviceResultTable_[requestId].resize(1);
				serviceResultCountTable_[requestId] = 1;
			}
			// insert request id to uncompleted request set
			{
				boost::mutex::scoped_lock uncompletedRequestLock(uncompletedRequestMutex_);
				uncompletedRequestSet_.insert(requestId);
			}
		}

     //cout<<"Single!!! after requestID"<<endl;
	// serviceRequestInfo->display();
     messageDispatcher_.sendDataToLowerLayer1(SERVICE_REQUEST_MSG, serviceRequestInfo, servicePermissionInfo.getServer());
     
	/*	sendServiceRequest(requestId, serviceName, requestList,
			servicePermissionInfo.getServer());*/
		return true;
	}

	/******************************************************************************
	Input:
    servicePermissionInfo - it contains information of service name and the server
	serviceRequestInfos - a set of information about request services, each contains
		the service name and its parameter values.
	Description: This function puts a set of requests of the same manager to the
		MessageClientFull. The MessageClientFull will sends the request to either
		MessageController or MessageServerFull.
	******************************************************************************/
	bool MessageClientFull::putServiceRequest(
				const ServicePermissionInfo& servicePermissionInfo,
				std::vector<ServiceRequestInfoPtr>& serviceRequestInfos)
	{
    /*for(unsigned int i=0; i<serviceRequestInfos.size(); i++)
    {      
           cout<<"put service REquest "<<i<<endl;  
     	     putServiceRequest(servicePermissionInfo, serviceRequestInfos[i]);
    } 

    return true;*/
    
	
		if( !prepareConnection(servicePermissionInfo.getServer()) )
			return false;

		std::string serviceName = servicePermissionInfo.getServiceName();
		int totalRequestNumber = serviceRequestInfos.size();
		int batchProcessedRequestNumber = getBatchProcessedRequestNumber();
		assert(batchProcessedRequestNumber < 256);

		//int serviceMessageNumber = ( (totalRequestNumber-1)/batchProcessedRequestNumber) + 1;
		for( int i=0; i<totalRequestNumber; i+=batchProcessedRequestNumber )
		{
			unsigned int requestId = generateRequestId();
			/*if(requestId == 0)
			{
				std::cout << "[Client:" << getName() ;
				std::cout << "] Cannot generate a request id" << std::endl;
				return false;
			}*/

			int minorRequestNumber = (totalRequestNumber - i) > batchProcessedRequestNumber ?
				batchProcessedRequestNumber : (totalRequestNumber - i);
			//std::vector<boost::shared_ptr<VariantType> > requestList;
			//requestList.reserve(minorRequestNumber);
			ServiceRequestInfoPtr batchedServiceRequest(new ServiceRequestInfo);
			batchedServiceRequest->setRequestId(requestId);
        batchedServiceRequest->setServiceName(serviceName);
        batchedServiceRequest->setMinorId(minorRequestNumber);       
			for( int j=0; j<minorRequestNumber; j++ )
			{
			   serviceRequestInfos[i+j]->setRequestId(requestId);
           serviceRequestInfos[i+j]->setMinorId(j+1);
         //  serviceRequestInfos[i+j]->setServiceResultFlag(
			//		servicePermissionInfo.getServiceResultFlag());
           
          // serviceRequestInfo.setServiceResultFlag(servicePermissionInfo.getServiceResultFlag());
			   assert(serviceRequestInfos[i+j]->getBufferNum() == 1);          
           batchedServiceRequest->pushBuffer(serviceRequestInfos[i+j]->getBuffer(0));
                  
            
				/*unsigned int minorId = j+1;
				ServiceRequestInfo& serviceRequestInfo = serviceRequestInfos[i+j];
				serviceRequestInfo.setRequestId(requestId);
				// minor id ranges from 1, a minor id = 0 indicates there is only one
				// request corresponding to the request id.
				serviceRequestInfo.setMinorId(minorId);
				serviceRequestInfo.setServiceResultFlag(
					servicePermissionInfo.getServiceResultFlag());

				// copy request from user space
				boost::shared_ptr<ServiceRequestInfo> request(
					new ServiceRequestInfo(serviceRequestInfo) );
				boost::shared_ptr<VariantType> tmp( new VariantType() );
				tmp->putCustomData(request);
				requestList.push_back(tmp);*/
			}

			// 1. create a semaphore for each request that need a reply.
			// 2. insert pair <requestid, semaphore into the hash map semaphoreTable_.
			// 3. then send request to MessageServer
			if(servicePermissionInfo.getServiceResultFlag() != SERVICE_WITHOUT_RESULT)
			{
				// prepare semaphore
				{
					boost::shared_ptr<semaphore_type> semaphore =
						boost::shared_ptr<semaphore_type>(new semaphore_type(0));
					boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
					semaphoreTable_[requestId] = semaphore;
				}
				// prepare result list
				{
					boost::mutex::scoped_lock resultTableLock(serviceResultMutex_);
					serviceResultTable_[requestId].resize(minorRequestNumber);
					serviceResultCountTable_[requestId] = minorRequestNumber;
				}
				// insert request id to uncompleted request set
				{
					boost::mutex::scoped_lock uncompletedRequestLock(uncompletedRequestMutex_);
					uncompletedRequestSet_.insert(requestId);
				}
			}
		//	cout<<"batch!!! after requestID"<<endl;
	    //  batchedServiceRequest->display();
	      
			sendServiceRequest(batchedServiceRequest,
				servicePermissionInfo.getServer());
		}

		return true;
	}

	/******************************************************************************
	Input:
		 serviceRequestInfos - a set of service request informations
	Output:
		 serviceResults - a set of service results
	Retur:
		 true - Result is ready.
		 false - result is not ready.
	Description: This function gets a set of results of the service
	that have been requested. The service is requested through function
	putServiceRequest(..) When the result is not ready, it returns false immediately.
	******************************************************************************/
	bool MessageClientFull::getResultOfService(
				const std::vector<ServiceRequestInfoPtr> & serviceRequestInfos,
				std::vector<ServiceResultPtr> & serviceResults)
	{
		if(serviceRequestInfos.size() == 0)
			return false;
		// connection has not been established
		if(!connectionToControllerEstablished_)
			return false;

		serviceResults.resize(serviceRequestInfos.size());

		bool ret = true;
		unsigned int i = 0;
		while( i < serviceRequestInfos.size() )
		{		
			unsigned int requestId = serviceRequestInfos[i]->getRequestId();
			std::string serviceName = serviceRequestInfos[i]->getServiceName();
			//ServiceResultFlag serviceResultFlag = serviceRequestInfos[i].getServiceResultFlag();

  			unsigned int range = 0;
			while( ( i + range + 1) < serviceRequestInfos.size() )
			{
				if( serviceRequestInfos[i+range+1]->getRequestId() != requestId )
					break;
				range++;
			}

			//if( serviceResultFlag == messageframework::SERVICE_WITHOUT_RESULT )
			//	goto get_service_result_fail;

			// cout<<"dbg getResultOfService range = "<<range<<endl;

			if( range == 0 )
			{
				if( false == getResultOfService( serviceRequestInfos[i], serviceResults[i] ) )
					goto get_service_result_fail;
				goto get_service_result_succ;
			}

			// Modified by Wei Cao, 2009-2-19
			// 1. get the semaphore associated with the request by looking up hash table
			//    semaphoreTable_.
			// 2. timed wait on the semaphore until timeout or reply returned and semphore
			//    is released.
			// 3. delete the semaphore from semaphoreTable_.
			// 4. check wheterh request id in completedRequestSet_..
			// 5. get reply by looking up std::map serviceResultTable_.

			// semaphore may be deleted in previous invokcation
			// so, if semaphore doesn't exist, just pass it
			{
				boost::shared_ptr<semaphore_type> semaphore;
				{
					boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
					std::map<unsigned int, boost::shared_ptr<semaphore_type> >::const_iterator it =
						semaphoreTable_.find(requestId);
					if( it != semaphoreTable_.end() )
						semaphore = it->second;
				}
				if(semaphore != NULL)
				{
					boost::system_time const timeout = boost::get_system_time()
						+ boost::posix_time::milliseconds(timeOutMilliSecond_);
					if( false == semaphore->timed_wait(timeout) )
					{
						std::cout << "[Client1:" << getName() ;
						std::cout << "] Reqeust id 0x" << hex << requestId << dec << " timeout" << std::endl;
						goto get_service_result_fail;
					}
					// if multi threads waiting on requests which owns the same request id but
					// different minor id, the thread who obtains the semaphore first has the
					// responsiblity to wake up other threads.
					// I think this operation should not have side effect, after all, semaphore
					// would never be reused.
					semaphore->post();
					boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
					semaphoreTable_.erase(requestId);
					semaphore.reset();
				}
			}

			{
				boost::mutex::scoped_lock serviceResultLock(serviceResultMutex_);

				// check whether request id in completed request set
				// this check must be done in the same lock scope with request id deletion bellow
				{
					boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
					std::set<unsigned int>::iterator it = completedRequestSet_.find(requestId);
					if( it == completedRequestSet_.end() )
					{
						std::cout << "dbg: [Client:" << getName() << "] invalid request " << requestId << std::endl;
						goto get_service_result_fail;
					}
				}

				for(unsigned int j = i; j <= i + range; j++ )
				{
					unsigned int minorId = serviceRequestInfos[j]->getMinorId();
					if( serviceResultTable_[requestId][minorId -1] == NULL ) {
						std::cout << "invalid service result " << requestId << "," << minorId << std::endl;
						goto get_service_result_fail;
					}
					serviceResults[j] = serviceResultTable_[requestId][minorId -1];
					serviceResultTable_[requestId][minorId-1].reset();
					serviceResultCountTable_[requestId] --;
				}
				if(serviceResultCountTable_[requestId] == 0)
				{
					serviceResultTable_.erase(requestId);
					serviceResultCountTable_.erase(requestId);
					// delete request id from completed request set if all results have been got away
					{
						boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
						completedRequestSet_.erase(requestId);
					}
				}
				goto get_service_result_succ;
			}

get_service_result_fail:
			ret = false;
get_service_result_succ:
			i += (range+1);
			continue;
		}

		return ret;
		//return false;
	}

	/******************************************************************************
	Input:
		 serviceRequestInfo - it contains information of service name and the server
	Output:
		 result - information about result. I contains the result of the service.
	Retur:
		 true - Result is ready.
		 false - result is not ready.
	Description: This function gets a result of the service that have been requested.
	The service is requested through function putServiceRequest(..)
	When the result is not ready, it returns false immediately.
	******************************************************************************/
	bool MessageClientFull::getResultOfService(
				const ServiceRequestInfoPtr& serviceRequestInfo,
				ServiceResultPtr& serviceResult)
	{
		// connection has not been established
		if(!connectionToControllerEstablished_)
			return false;

		//the service provides no return value  @by MyungHyun - 2009-01-28
		
		/*if( serviceRequestInfo.getServiceResultFlag() ==
			messageframework::SERVICE_WITHOUT_RESULT)
			return false;*/

		unsigned int requestId = serviceRequestInfo->getRequestId();
		unsigned int minorId = serviceRequestInfo->getMinorId();

		// Modified by Wei Cao, 2009-2-19
		// 1. get the semaphore associated with the request by looking up hash table
		//    semaphoreTable_.
		// 2. timed wait on the semaphore until timeout or reply returned and semphore
		//    is released.
		// 3. delete the semaphore from semaphoreTable_.
		// 4. check wheterh request id in completedRequestSet_..
		// 5. get reply by looking up std::map serviceResultTable_.

		// semaphore may be deleted in previous invokcation
		// so, if semaphore doesn't exist, just pass it
		boost::shared_ptr<semaphore_type> semaphore;
		{
			boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
			std::map<unsigned int, boost::shared_ptr<semaphore_type> >::const_iterator it =
				semaphoreTable_.find(requestId);
			if( it != semaphoreTable_.end() )
				semaphore = it->second;
		}
		if(semaphore != NULL)
		{
			boost::system_time const timeout = boost::get_system_time()
				+ boost::posix_time::milliseconds(timeOutMilliSecond_);
			if( false == semaphore->timed_wait(timeout) )
			{
				std::cout << "[Client:" << getName() ;
				std::cout << "] Reqeust id 0x" << hex << requestId << dec << " timeout" << std::endl;
				return false;
			}
			// if multi threads waiting on requests which owns the same request id but
			// different minor id, the thread who obtains the semaphore first has the
			// responsiblity to wake up other threads.
			// I think this operation should not have side effect, after all, semaphore
			// would never be reused.
			semaphore->post();
			boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
			semaphoreTable_.erase(requestId);
			semaphore.reset();
		}

		// minor id equals to 0 indicats that, there is only one request corresponding to
		// the request id, so implement following steps in a optimized way.
		if(minorId == 0)
		{
			// check whether request id in completed request set
			{
				boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
				std::set<unsigned int>::iterator it = completedRequestSet_.find(requestId);
				if( it == completedRequestSet_.end() )
				{
					std::cout << "[Client:" << getName() << "] invalid request " << requestId << std::endl;
					return false;
				}
				completedRequestSet_.erase(requestId);
			}
			boost::mutex::scoped_lock serviceResultLock(serviceResultMutex_);
			serviceResult = serviceResultTable_[requestId][0];
			serviceResultTable_.erase(requestId);
			serviceResultCountTable_.erase(requestId);
			return true;
		}

		// Following code is invoked only in such a situation:
		// user code sends a set of requests using batch-request interface but
		// collects the result one by one.
		// I think I should satisfy this kind of requirement, in a safe but
		// not so efficiently way, above all this kind of usage should not be
		// encouraged.

		// get a result from result list and decrease count by 1
		{
			boost::mutex::scoped_lock serviceResultLock(serviceResultMutex_);

			// check whether request id in completed request set
			// this check must be done in the same lock scope with request id deletion bellow
			{
				boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
				std::set<unsigned int>::iterator it = completedRequestSet_.find(requestId);
				if( it == completedRequestSet_.end() )
				{
					std::cout << "[Client:" << getName() << "] invalid request " << requestId << std::endl;
					return false;
				}
			}

			if(serviceResultTable_[requestId][minorId-1] == NULL )
				return false;
			serviceResult = serviceResultTable_[requestId][minorId -1];
			serviceResultTable_[requestId][minorId-1].reset();

			serviceResultCountTable_[requestId] --;
			if(serviceResultCountTable_[requestId] == 0)
			{
				serviceResultTable_.erase(requestId);
				serviceResultCountTable_.erase(requestId);

				// delete request id from completed request set if all results have been got away
				{
					boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
					completedRequestSet_.erase(requestId);
				}
			}
		}

		return true;
	}


	/**
	 * @brief This function sends a request to get client id
 	 */
 	 
	/*void MessageClientFull::sendClientIdRequest()
	{
		ClientIdRequestMessage message;
		messageDispatcher_.sendDataToLowerLayer( CLIENT_ID_REQUEST_MSG, message, controllerNode_);
	}*/


	/******************************************************************************
	Description: This function saves clientId to clientId_
	Input:
		clientId - controller assigned client id
	*******************************************************************************/
	/*void MessageClientFull::receiveClientIdResult(const int& clientId)
	{
		{
			boost::mutex::scoped_lock clientIdLock(clientIdMutex_);
			clientId_ = clientId;
		}
		clientIdEvent_.notify_all();
	}*/


	/**
	 * @brief This function sends a request to get Permission of a service
 	 * @param serviceName the service name
 	 */
	void MessageClientFull::sendPermissionOfServiceRequest(
					const std::string& serviceName)
	{
		PermissionRequestMessage message(serviceName);
		messageDispatcher_.sendDataToLowerLayer( PERMISSION_OF_SERVICE_REQUEST_MSG, message, controllerNode_);
	}

	/******************************************************************************
 	 Description: This function push PermissionOfService to the internal list
 	 Input:
		servicePermissionInfo- the information of the PermissionOfService
	 ******************************************************************************/
	void MessageClientFull::receivePermissionOfServiceResult(
							const ServicePermissionInfo& servicePermissionInfo)
	{
#ifdef SF1_DEBUG
		std::cout << "[Client:" << getName() ;
		std::cout << "] Receive permission of " << servicePermissionInfo.getServiceName() << std::endl;
#endif

		boost::mutex::scoped_lock acceptedPermissionLock(acceptedPermissionMutex_);
		acceptedPermissionList_[servicePermissionInfo.getServiceName()] = servicePermissionInfo;
		acceptedPermissionLock.unlock();
		newPermisionOfServiceEvent_.notify_all();
	}


	/**
	 * @brief This function sends request to the ServiceResultServer
	 * @param
	 * requestId - the id of service request
	 * @param
	 * serviceName - the service name
	 * @param
	 * data - the data of the service request
	 * @param
	 * server - the server that will receive data
	 */
	void MessageClientFull::sendServiceRequest(const ServiceRequestInfoPtr& requestInfo,
				const MessageFrameworkNode& server)
	{
		/*ServiceMessage message;

		for(size_t i = 0; i < data.size(); i++)
			message.appendData(data[i]);
		message.setRequestId(requestId);
		message.setServiceName(serviceName);*/

		// now send the message to server
		messageDispatcher_.sendDataToLowerLayer1(SERVICE_REQUEST_MSG, requestInfo, server);
	}

	/*********************************************************************************
	Description: This function updates result of a requested service.
	It put the result in the result table. Later, the result is retrieved through
	getResultOfService(function)
	Input:
		 requestId - the request Id
		 serviceResult - the result of the requested service
	*********************************************************************************/
	void MessageClientFull::receiveResultOfService(const ServiceResultPtr& result)
	{
#ifdef SF1_DEBUG
		std::cout << "[Client:" << getName() << "] Receive result of request " << result->getRequestId() << std::endl;
      result->display();
#endif
//cout<<"!!!!! wps"<<endl;
		//check whether request id in uncompleted request set
		//then remove request id from uncompleted request set
		unsigned int requestId = result->getRequestId();
		{
			boost::mutex::scoped_lock uncompletedRequestLock(uncompletedRequestMutex_);
			std::set<unsigned int>::iterator it = uncompletedRequestSet_.find(requestId);
			if( it == uncompletedRequestSet_.end() )
			{
				std::cout << "[Client:" << getName() << "] invalid request " << requestId << std::endl;
				throw MessageFrameworkException(SF1_MSGFRK_LOGIC_ERROR, __LINE__, __FILE__);
			}
			uncompletedRequestSet_.erase(requestId);
		}
      	{
      		boost::mutex::scoped_lock serviceResultLock(serviceResultMutex_);
           serviceResultTable_[requestId][0] = result;
			}
     

		// insert results into result table
		{
			boost::mutex::scoped_lock serviceResultLock(serviceResultMutex_);
			int expectedResultCount = serviceResultCountTable_[requestId];
	
			int resultCount = 1;
			if( result->getMinorId() != 0)
				 resultCount= result->getBufferNum();
			if( expectedResultCount != resultCount )
			{
				std::cout << "[Client:" << getName() << "] processing request " << requestId
					<< " expect " << expectedResultCount << " results "
					<< " but receive " << resultCount << " results " << std::endl;
				throw MessageFrameworkException(SF1_MSGFRK_LOGIC_ERROR, __LINE__, __FILE__);
			}
        unsigned int minorId = result->getMinorId();
        //assert(minorId == resultCount);
       // cout<<minorId<<" vs "<<resultCount <<endl;
       // result->display();
        
        if( minorId == 0)
			{
					serviceResultTable_[requestId][0] = result;
			} 
        else{
        	
			 for( int i = 0; i < resultCount; i++ )
			 {
				ServiceResultPtr serviceResult(new ServiceResult);
				//data[i]->getCustomData(serviceResult);
				minorId = i+1;
				serviceResult->setServiceName(result->getServiceName());
				serviceResult->setRequestId(result->getRequestId());
				//serviceResult->setMinorId(result->getMinorId());	
				serviceResult->setMinorId(i+1);	
				serviceResult->pushBuffer(result->getBuffer(i));
				//cout<<"DBG: decompose result: ";
				//serviceResult->display();
				serviceResultTable_[requestId][minorId-1] = serviceResult;			
			 
			 }
        	}
		}

		//result is ready, insert request id into completed request table
		{
			boost::mutex::scoped_lock completedRequestLock(completedRequestMutex_);
			completedRequestSet_.insert(requestId);
		}

		// When a reply arrives, find the semaphore associated with the request, then
		// release the semaphore to wake up any threads waiting on it ( most probably
		// the thread who calls getServiceResult )
		boost::shared_ptr<semaphore_type> semaphore;
		{
			boost::mutex::scoped_lock semaphoreLock(semaphoreMutex_);
			std::map<unsigned int, boost::shared_ptr<semaphore_type> >::const_iterator it =
				semaphoreTable_.find(requestId);
			if( it != semaphoreTable_.end() )
				semaphore = it->second;
		}
		if(semaphore != NULL)
			semaphore->post();
	}

	/**
	 * @brief This function create a new AsyncStream that is based on tcp::socket
	 */
	AsyncStream* MessageClientFull::createAsyncStream(boost::shared_ptr<tcp::socket> sock)
	{
		// tcp::endpoint endpoint = sock->local_endpoint();
		tcp::endpoint endpoint = sock->remote_endpoint();
		std::string logMsg;
		logMsg = "Accept new connection, Remote IP = ";
		logMsg += endpoint.address().to_string();
		logMsg += ", port = ";
		std::ostringstream oss(ostringstream::out);
		oss << endpoint.port();
		logMsg += oss.str();
        if(!connectionToControllerEstablished_)
			logMsg += ", controller connection";
		else
			logMsg += ", server connection";

		std::cout << logMsg << std::endl;

#ifdef _LOGGING_
		WriteToLog("log.log", logMsg.c_str());
#endif
		if(!connectionToControllerEstablished_)
		{
			// this must be a connection to controller
			AsyncStream* pNewSocket;
			{
				boost::mutex::scoped_lock connectionToControllerEstablishedLock(
					connectionToControllerEstablishedMutex_);
				pNewSocket = new AsyncStream(&messageDispatcher_, sock);
				connectionToControllerEstablished_ = true;
			}
			connectionToControllerEvent_.notify_all();
			return pNewSocket;

		} else {
			AsyncStream* pNewSocket;
			{
				boost::mutex::scoped_lock connectionToServerEstablishedLock(
					connectionToServerEstablishedMutex_);
				pNewSocket = new AsyncStream(&messageDispatcher_, sock);
			}
			// the connection is made with server
			connectedToServerEvent_.notify_all();
			return pNewSocket;
		}

		throw MessageFrameworkException(SF1_MSGFRK_CONNECTION_TIMEOUT, __LINE__, __FILE__);
	}	

}// end of messageframework
