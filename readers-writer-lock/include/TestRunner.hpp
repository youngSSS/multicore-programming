#ifndef READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
#define READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace std;

class TestRunner {
 private:
	int numThread;
	int numRecord;
	int numExecution;



	boost::asio::io_service ioService;
	boost::asio::io_service::work work;
	boost::thread_group threadPool;

 public:
	TestRunner(int t, int r, int e) : wfs(new WaitFreeSnapshot(numThread)), numThread(t), numRecord(r), numExecution(e), work(ioService) {
		for (int i = 0; i < numThread; i++)
			threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
	}

	~TestRunner() {
		delete wfs;
	}

	void startReaderWriterTest() {

	}

};

#endif //READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
