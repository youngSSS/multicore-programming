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
	int numTread;
	int numRecord;
	int numExecution;

	boost::asio::io_service ioService;
	boost::asio::io_service::work work;
	boost::thread_group threadPool;

 public:
	TestRunner(int numThread) : wfs(new WaitFreeSnapshot(numThread)), numThread(numThread), work(ioService) {
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
