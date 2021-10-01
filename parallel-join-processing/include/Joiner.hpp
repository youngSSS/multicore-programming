#pragma once
#include <vector>
#include <cstdint>
#include <set>
#include "Operators.hpp"
#include "Relation.hpp"
#include "Parser.hpp"

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <condition_variable>

using namespace std;

//---------------------------------------------------------------------------
class Joiner {
	/// Add scan to query
	std::unique_ptr<Operator> addScan(std::set<unsigned>& usedRelations, SelectInfo& info, QueryInfo& query);

	// Boost
	// 1. How to create thread pool
	// -> https://coderedirect.com/questions/116779/how-to-create-a-thread-pool-using-boost-in-c
	boost::asio::io_service ioService;
	boost::thread_group threadPool;
	boost::asio::io_service::work work;

	// Condition variable, Mutex
	condition_variable cond;
	mutex _mutex;

	// Results of join process
	vector<string> joinResults;

	// Number of threads
	int NUM_THREADS = 100;

 public:
	// Constructor, Create thread pool
	Joiner() : work(ioService) {
		for (int i = 0; i < NUM_THREADS; i++) {
			threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
		}
	}

	// Thread index (= Number of threads)
	int threadIndex = -1;
	// Start join processes concurrently
	void startJoinThread(string line);
	// Get join results from threads
	std::string getJoinResults();

	/// The relations that might be joined
	std::vector<Relation> relations;
	/// Add relation
	void addRelation(const char* fileName);
	/// Get relation
	Relation& getRelation(unsigned id);
	/// Joins a given set of relations
	void join(QueryInfo& i, int threadIdx);
};
//---------------------------------------------------------------------------
