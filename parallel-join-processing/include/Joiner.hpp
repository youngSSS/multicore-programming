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

using namespace std;

//---------------------------------------------------------------------------
class Joiner {

	/// Add scan to query
	std::unique_ptr<Operator> addScan(std::set<unsigned>& usedRelations, SelectInfo& info, QueryInfo& query);

	boost::asio::io_service ioService;
	boost::thread_group threadPool;
	boost::asio::io_service::work work;

	condition_variable cond;
	mutex _mutex;

	vector<string> joinResults;

	int NUM_THREADS = 10;

 public:
	Joiner() : work(ioService) {
		for (int i = 0; i < NUM_THREADS; i++) {
			threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
		}
	}

	// Thread Index (= Number of threads)
	int threadIndex = -1;

	/// The relations that might be joined
	std::vector<Relation> relations;
	/// Add relation
	void addRelation(const char* fileName);
	/// Get relation
	Relation& getRelation(unsigned id);
	/// Joins a given set of relations
	void join(QueryInfo& i, int threadIdx);

	void startJoinThread(string line);

	std::string getJoinResults();
};
//---------------------------------------------------------------------------
