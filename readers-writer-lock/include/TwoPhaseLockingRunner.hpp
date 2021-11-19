#ifndef READERS_WRITER_LOCK_INCLUDE_TWOPHASELOCKINGRUNNER_HPP_
#define READERS_WRITER_LOCK_INCLUDE_TWOPHASELOCKINGRUNNER_HPP_

#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include <pthread.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "Database.hpp"
#include "ConcurrencyControl.hpp"

using namespace std;

class TwoPhaseLockingRunner {
 private:
	// User inputs
	int numThread;
	int numRecord;
	int numExecution;

	// Two phase locking classes
	Database* database;
	Transaction* transaction;
	Lock* lock;

	// Boost
	boost::asio::io_service ioService;
	boost::asio::io_service::work work;
	boost::thread_group threadPool;

	pthread_mutex_t mainMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t mainCond = PTHREAD_COND_INITIALIZER;

	// Thread routine
	void threadFunc(int tid) {
		if (tid == numThread) pthread_cond_signal(&mainCond);

		vector<int> recordIdx;

		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<int> dis(0, numRecord - 1);

		int result = 0;
		while (result == 0) {
			while (recordIdx.size() < 3) {
				int idx = dis(gen);
				if (find(recordIdx.begin(), recordIdx.end(), idx) == recordIdx.end())
					recordIdx.push_back(idx);
			}

			result = transaction->startTrx(recordIdx, tid);
			recordIdx.clear();
		}
	}

 public:
	// Constructor
	TwoPhaseLockingRunner(int t, int r, int e) : numThread(t), numRecord(r), numExecution(e), work(ioService) {
		// Initialize
		database = new Database(r);
		transaction = new Transaction(database, numExecution, numThread);
		lock = new Lock(database, numThread);

		transaction->setLockManager(lock);
		lock->setTrxManager(transaction);

		// Make thread pool
		for (int i = 0; i < numThread; i++)
			threadPool.create_thread([ObjectPtr = &ioService] { ObjectPtr->run(); });

		// Initialize the result files
		for (int i = 1; i <= numThread; i++) {
			ofstream fout("thread" + to_string(i) + ".txt");
			fout.close();
		}
	}

	// Destructor
	~TwoPhaseLockingRunner() {
		delete database;
		delete transaction;
		delete lock;
	}

	// Two phase locking running method
	void startReadersWriterTest() {
		// Bind thread function to threads
		for (int tid = 1; tid <= numThread; tid++)
			ioService.post([this, tid] { threadFunc(tid); });

		pthread_mutex_lock(&mainMutex);
		pthread_cond_wait(&mainCond, &mainMutex);
		pthread_mutex_unlock(&mainMutex);

		// Wait until the termination of threads
		ioService.stop();
		threadPool.join_all();
	}

};

#endif //READERS_WRITER_LOCK_INCLUDE_TWOPHASELOCKINGRUNNER_HPP_