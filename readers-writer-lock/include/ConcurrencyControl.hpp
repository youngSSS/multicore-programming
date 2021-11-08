#ifndef READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
#define READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_

#include "Database.hpp"

#include <boost/thread.hpp>

#define S_MODE 0
#define X_MODE 1

#define DEADLOCK 1

using namespace std;

class Transaction;
class Lock;

struct lock_t {
	int trxId;
	int lockMode;
	int isWaiting;

	lock_t* prev;
	lock_t* next;

	boost::condition_variable cond;
};

struct trx_t {
	int trxId;
	lock_t* prev;
	lock_t* next;
};

class Transaction {
 private:
	Database* database;
	Lock* lockManager;
	int trxIdSeq;

	boost::mutex trxIdSeqMutex;

	void handleDeadlock();

 public:
	// Constructor
	Transaction(Database* db, Lock* l) : database(db), lockManager(l), trxIdSeq(0) {}

	// Transaction methods
	int trxBegin();
	int trxCommit(int trxId);
	int trxRollback(int trxId);
	int64_t trxRead(int trxId, int rid);
	void trxWrite(int trxId, int rid, int64_t value);
};

class Lock {
 private:
	Database* database;
	Transaction* trxManager;

	static boost::mutex lockTableMutex;

 public:
	// Constructor
	Lock(Database* db, Transaction* t) : database(db), trxManager(t) {}

	// Lock methods
	int acquireRecordMutex(int trxId, int rid, int lockMode);
	void releaseRecordMutex(int trxId, int rid, int lockMode);

	static void acquireLockTableMutex();
	static void releaseLockTableMutex();
};

#endif //READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
