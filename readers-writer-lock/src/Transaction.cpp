#include "ConcurrencyControl.hpp"

void Transaction::handleDeadlock() {

}

void Transaction::updateTrxTable(int trxId, lock_t* lockObj) {
	if (trxTable[trxId]->head == nullptr) {
		trxTable[trxId]->head = lockObj;
		trxTable[trxId]->tail = lockObj;
	}
	else {
		trxTable[trxId]->tail->next = lockObj;
		lockObj->prev = trxTable[trxId]->tail;
		trxTable[trxId]->tail = lockObj;
	}
}

int Transaction::trxBegin() {
	int id;

	trxIdSeqMutex.lock();
	id = trxIdSeq++;
	trxIdSeqMutex.unlock();

	trxTable[id] = new trxHeader_t();

	return id;
}

int Transaction::trxCommit(int trxId) {
	// Two-phase locking, release acquired locks

}

int Transaction::trxRollback(int trxId) {
	// Two-phase locking, release acquired locks
}

int64_t Transaction::trxRead(int trxId, int rid) {
	Lock::acquireLockTableMutex();

	lock_t* lockObj = lockManager->acquireRecordMutex(trxId, rid, S_MODE);
	if (lockObj == DEADLOCK) handleDeadlock();
	else updateTrxTable(trxId, lockObj);

	Lock::releaseLockTableMutex();

	return database->readRecord(rid);
}

void Transaction::trxWrite(int trxId, int rid, int64_t value) {
	Lock::acquireLockTableMutex();

	lock_t* lockObj = lockManager->acquireRecordMutex(trxId, rid, X_MODE);
	if (lockObj == DEADLOCK) handleDeadlock();
	else updateTrxTable(trxId, lockObj);

	Lock::releaseLockTableMutex();

	database->updateRecord(rid, value);
}