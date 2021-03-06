#ifndef READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
#define READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_

#include "Database.hpp"

#include <unordered_map>
#include <boost/thread.hpp>
#include <fstream>

#define S_MODE 0
#define X_MODE 1

#define WAITING 0
#define WORKING 1

#define DEADLOCK nullptr

using namespace std;

class Transaction;
class Lock;
struct lockTable_t;
struct lock_t;
struct trxTable_t;

struct lockTable_t {
	int64_t key;

	lock_t* head;
	lock_t* tail;

	explicit lockTable_t(int64_t k) : head(nullptr), tail(nullptr), key(k) {}
};

struct trxTable_t {
	int trxId;

	lock_t* head;
	lock_t* tail;

	unordered_map<int, int64_t> undoLog;
	vector<int64_t> history;

	explicit trxTable_t(int trxId) : trxId(trxId), head(nullptr), tail(nullptr) {}
};

struct lock_t {
	int lockMode;
	int condition;

	lockTable_t* lSentinel;
	lock_t* lPrev = nullptr;
	lock_t* lNext = nullptr;

	trxTable_t* tSentinel;
	lock_t* tPrev = nullptr;
	lock_t* tNext = nullptr;

	boost::condition_variable cond;

	lock_t(int mode, int c, lockTable_t* ls) :
		lockMode(mode), condition(c), lSentinel(ls), tSentinel(nullptr) {}

};

class Transaction {
 private:
	Database* database;
	Lock* lockManager;

	unordered_map<int, trxTable_t*> trxTable;
	int trxIdSeq;

	int executionLimit;
	int numExecution;

	// Private methods
	int  trxBegin();
	int  trxCommit(int trxId, vector<int> recordIdx);
	void trxRollback(int trxId);
	int  trxRead(int trxId, int rid, int64_t* readValue); // Success(0), Deadlock(1)
	int  trxWrite(int trxId, int rid, int64_t value); 	  // Success(0), Deadlock(1)

	void releaseAcquiredLocks(int trxId);

 public:
	vector<boost::thread::id> threadId;

	int getThreadId(boost::thread::id realId) {
		auto iter = find(threadId.begin(), threadId.end(), realId);
		return iter - threadId.begin();
	}

	// Constructor
	Transaction(Database* db, int e, int t) :
		database(db), lockManager(nullptr), executionLimit(e), trxIdSeq(0), numExecution(0) {
		threadId.resize(t + 1);
	}

	// Public methods
	int  startTrx(vector<int> recordIdx, int tid);
	void setLockManager(Lock* l);
	void updateTrxTable(int trxId, lock_t* lockObj);
	int  detectDeadlock(lock_t* lockObj, set<int>& waitForSet, int targetTrxId);

};

class Lock {
 private:
	Database* database;
	Transaction* trxManager;

	unordered_map<int, lockTable_t*> lockTable;
	boost::mutex lockTableMutex;
	unordered_map<int, boost::unique_lock<boost::mutex> > lockTableLockers;

	// Private methods
	void printLockTable();
	int  analyzeLockTable(int trxId, int rid, int lockMode, lock_t* lockObj);

 public:
	// Constructor
	Lock(Database* db, int numThread) : database(db), trxManager(nullptr) {
		for (int i = 1; i <= numThread; i++)
			lockTableLockers[i] = boost::unique_lock<boost::mutex>(lockTableMutex, boost::defer_lock);
	}

	// Public methods
	lock_t* acquireRecordLock(int trxId, int rid, int lockMode);
	void 	releaseRecordLock(lock_t* lockObj);
	void 	acquireLockTableMutex();
	void 	releaseLockTableMutex();
	void 	setTrxManager(Transaction* t);
	void	getWaitForGraph(unordered_map< int, vector<int> >& nodes);
};

#endif //READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
