#ifndef READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
#define READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "Database.hpp"
#include "ConcurrencyControl.hpp"

using namespace std;

class TestRunner {
 private:
	int numThread;
	int numRecord;
	int numExecution;

	Database* database;
	Transaction* transaction;
	Lock* lock;

	boost::asio::io_service ioService;
	boost::asio::io_service::work work;
	boost::thread_group threadPool;

	void testRootine() {

	}

 public:
	TestRunner(int t, int r, int e) : numThread(t), numRecord(r), numExecution(e), work(ioService) {
		// Initialize
		database = new Database(r);
		transaction = new Transaction(database, lock);
		lock = new Lock(database, transaction);

		// Make thread pool
		for (int i = 0; i < numThread; i++)
			threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
	}

	~TestRunner() {
		delete database;
		delete transaction;
		delete lock;
	}

	void startReaderWriterTest() {

	}

};

#endif //READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
