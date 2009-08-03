// how to see all log info.


// GLOG_logtostderr=1 ./t_log 
// default log files 

//To use glog, just include this 
#include <util/izene_log.h>

#include <iomanip>
#include <string>

using namespace std;
using namespace izenelib::util;

//severity levels (in increasing order of severity):
//INFO, WARNING, ERROR, and FATAL. Use this way:
//
//   LOG(INFO), LOG(ERROR),...
//
//Logging a FATAL message terminates the program (after the message is logged).
//Note that messages of a given severity are logged not only in the logfile for 
//that severity, but also in all logfiles of lower severity. E.g., a message of
//severity FATAL will be logged to the logfiles of severity FATAL, ERROR, WARNING, and INFO.

//The DFATAL severity logs a FATAL error in debug mode (i.e., there is no NDEBUG macro defined), 
//but avoids halting the program in production by automatically reducing the severity to ERROR. 

void TestLogging() {
	cout<<"\n===================TestLogging======================"<<endl;

	//glog at INFO level
	LOG(INFO) << string("foo ") << "bar " << 10 << ' ' << 3.4;
	for (int i = 0; i < 10; ++i) {
		int COUNTER = i;
		PLOG_EVERY_N(ERROR, 2) << "Plog every 2, iteration " << COUNTER;

		LOG_EVERY_N(ERROR, 3) << "Log every 3, iteration " << COUNTER << endl;
		LOG_EVERY_N(ERROR, 4) << "Log every 4, iteration " << COUNTER << endl;

		LOG_IF_EVERY_N(WARNING, true, 5) << "Log if every 5, iteration "
				<< COUNTER;
		LOG_IF_EVERY_N(WARNING, false, 3) << "Log if every 3, iteration "
				<< COUNTER;
		LOG_IF_EVERY_N(INFO, true, 1) << "Log if every 1, iteration "
				<< COUNTER;
		LOG_IF_EVERY_N(ERROR, (i < 3), 2)
				<< "Log if less than 3 every 2, iteration " << COUNTER;
	}
	LOG_IF(WARNING, true) << "log_if this";
	LOG_IF(WARNING, false) << "don't log_if this";

	char s[] = "array";
	LOG(INFO) << s;
	const char const_s[] = "const array";
	LOG(INFO) << const_s;
	int j = 1000;
	LOG(ERROR) << string("foo") << ' '<< j << ' ' << setw(10) << j << " "
			<< setw(1) << hex << j;

	//LogMessage("foo", LogMessage::kNoLogPrefix, INFO).stream() << "no prefix";

}

//for vebose log
void TestLogging_verbose() {

	cout<<"\n===================TestLogging_verbose======================"
			<<endl;

	VLOG(1) << "I'm printed when you run the program with --v=1 or higher";
	VLOG(2) << "I'm printed when you run the program with --v=2 or higher";

}

//for debug mode log
//if #define NDEBUG, then no output.
void TestLogging_debug() {

	cout<<"\n===================TestLogging_debug======================"<<endl;

	//glog at INFO level
	DLOG(INFO) << string("foo ") << "bar " << 10 << ' ' << 3.4;
	for (int i = 0; i < 10; ++i) {
		int COUNTER = i;

		DLOG_EVERY_N(ERROR, 3) << "DLOG every 3, iteration " << COUNTER << endl;
		DLOG_EVERY_N(ERROR, 4) << "DLOG every 4, iteration " << COUNTER << endl;

		DLOG_IF_EVERY_N(WARNING, true, 5) << "DLOG if every 5, iteration "
				<< COUNTER;
		DLOG_IF_EVERY_N(WARNING, false, 3) << "DLOG if every 3, iteration "
				<< COUNTER;
		DLOG_IF_EVERY_N(INFO, true, 1) << "DLOG if every 1, iteration "
				<< COUNTER;
		DLOG_IF_EVERY_N(ERROR, (i < 3), 2)
				<< "DLOG if less than 3 every 2, iteration " << COUNTER;
	}
	DLOG_IF(WARNING, true) << "DLOG_if this";
	DLOG_IF(WARNING, false) << "don't DLOG_if this";

	char s[] = "array";
	DLOG(INFO) << s;
	const char const_s[] = "const array";
	DLOG(INFO) << const_s;
	int j = 1000;
	DLOG(ERROR) << string("foo") << ' '<< j << ' ' << setw(10) << j << " "
			<< setw(1) << hex << j;

}


void TestLogging_memory() {
	cout<<"\n===================TestLogging_memory======================"<<endl;
	LOG(ERROR) << getMemInfo();
	char *p = new char[100*1024*1024];
	LOG(ERROR) << getMemInfo();
	delete p;
	LOG(ERROR) << getMemInfo();	
}


int main(int argc, char* argv[]) {
	// Initialize Google's logging library.
	google::InitGoogleLogging(argv[0]);
	TestLogging();
	TestLogging_verbose();
	TestLogging_debug();
	TestLogging_memory();
}

