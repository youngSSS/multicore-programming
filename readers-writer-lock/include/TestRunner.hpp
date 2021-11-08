#ifndef READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
#define READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_

#include <iostream>
#include <vector>
#include <random>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

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

	vector<int> getRandomRecordIdx() {
		vector<int> recordIdx;

		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<int> dis(0, numRecord - 1);

		while (recordIdx.size() < 3) {
			int idx = dis(gen);
			if (find(recordIdx.begin(), recordIdx.end(), idx) == recordIdx.end())
				recordIdx.push_back(idx);
		}

		return recordIdx;
	}

	void readerWriterTestRoutine() {
		vector<int> recordIdx = getRandomRecordIdx();
		int i = recordIdx[0];
		int j = recordIdx[1];
		int k = recordIdx[2];

		int trxId = transaction->trxBegin();
		int readValue = transaction->trxRead(trxId, i);
		transaction->trxWrite(trxId, j, readValue + 1);
		transaction->trxWrite(trxId, k, -readValue);
		transaction->trxCommit(trxId);
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

		for (int tid = 0; tid < numThread; tid++)
			ioService.post(boost::bind(&TestRunner::readerWriterTestRoutine, this));

//		ioService.stop();
		threadPool.join_all();
	}

};

#endif //READERS_WRITER_LOCK_INCLUDE_TESTRUNNER_HPP_
