#include "ConcurrencyControl.hpp"

void Transaction::handleDeadlock() {

}

int Transaction::trxBegin() {
	int id;

	trxIdSeqMutex.lock();
	id = trxIdSeq++;
	trxIdSeqMutex.unlock();

	return id;
}

int Transaction::trxCommit(int trxId) {

}

int Transaction::trxRollback(int trxId) {

}

int64_t Transaction::trxRead(int trxId, int rid) {
	Lock::acquireLockTableMutex();

	int result = lockManager->acquireRecordMutex(trxId, rid, S_MODE);
	if (result == DEADLOCK) handleDeadlock();

	Lock::releaseLockTableMutex();

	return database->readRecord(rid);
}

void Transaction::trxWrite(int trxId, int rid, int64_t value) {
	Lock::acquireLockTableMutex();

	int result = lockManager->acquireRecordMutex(trxId, rid, X_MODE);
	if (result == DEADLOCK) handleDeadlock();

	Lock::releaseLockTableMutex();

	database->updateRecord(rid, value);
}