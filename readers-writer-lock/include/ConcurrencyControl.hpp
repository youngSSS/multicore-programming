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
struct lockHeader_t;
struct trxHeader_t;
struct lock_t;

struct lock_t {
	int trxId;
	int lockMode;
	int condition;

	lockHeader_t* sentinel;
	lock_t* prev = nullptr;
	lock_t* next = nullptr;

	boost::condition_variable cond;

	lock_t(int trxId, int mode, int c, lockHeader_t* s) : trxId(trxId), lockMode(mode), condition(c), sentinel(s) {}
};

struct trx_t {

};

struct lockHeader_t {
	lock_t* head;
	lock_t* tail;

	int64_t key;

	lockHeader_t(int64_t k) : head(nullptr), tail(nullptr), key(k) {}
};

struct trxHeader_t {
	lock_t* head;
	lock_t* tail;

	unordered_map<int, int64_t> undoLog;
	vector<int64_t> history;

	trxHeader_t() : head(nullptr), tail(nullptr) {}
};

class Transaction {
 private:
	Database* database;
	Lock* lockManager;


	int trxIdSeq;

	boost::mutex trxIdSeqMutex;

	int threadId;
	int executionLimit;
	int numExecution;

	void updateTrxTable(int trxId, lock_t* lockObj);
	void releaseAcquiredLocks(int trxId);

	// Transaction methods
	int trxBegin();
	void trxCommit(int trxId, vector<int> recordIdx);
	void trxRollback(int trxId);
	int64_t trxRead(int trxId, int rid, int* deadlockFlag);
	void trxWrite(int trxId, int rid, int64_t value, int* deadlockFlag);

 public:
	unordered_map<int, trxHeader_t*> trxTable;

	// Constructor
	Transaction(Database* db, int e) :
		database(db), lockManager(nullptr), executionLimit(e), trxIdSeq(0), threadId(-1), numExecution(0) {}

	// Start transaction routine
	int startTrx(vector<int> recordIdx, int tid);
	void setLockManager(Lock* l);
};

class Lock {
 private:
	Database* database;
	Transaction* trxManager;

	unordered_map<int, lockHeader_t*> lockTable;

	boost::mutex lockTableMutex;

	int analyzeLockTable(int trxId, int rid, int lockMode, lock_t* lockObj);
	int detectDeadlock(lock_t* lockObj);

 public:
	// Constructor
	Lock(Database* db) : database(db), trxManager(nullptr) {}

	// Lock methods
	lock_t* acquireRecordLock(int trxId, int rid, int lockMode);
	void releaseRecordLock(lock_t* lockObj);

	void acquireLockTableMutex();
	void releaseLockTableMutex();

	void setTrxManager(Transaction* t);
};

#endif //READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
