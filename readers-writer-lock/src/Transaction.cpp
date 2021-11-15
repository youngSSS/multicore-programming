#include "ConcurrencyControl.hpp"

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

void Transaction::releaseAcquiredLocks(int trxId) {
	lock_t* lockObj = trxTable[trxId]->head;

	while (lockObj != nullptr) {
		lockManager->releaseRecordLock(lockObj);
		lockObj = lockObj->next;
	}
}

int Transaction::trxBegin() {
	int id;

	trxIdSeqMutex.lock();
	id = trxIdSeq++;
	trxTable[id] = new trxHeader_t();
	trxIdSeqMutex.unlock();

	return id;
}

void Transaction::trxCommit(int trxId, vector<int> recordIdx) {
	lockManager->acquireLockTableMutex();

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	// Increase the global execution order by 1
	int commitId = ++numExecution;

	// Case: Commit count exceeds execution limit
	if (commitId > executionLimit) {
		// Undo phase
		for (auto undoItem : trxTable[trxId]->undoLog)
			database->updateRecord(undoItem.first, undoItem.second);

		delete trxTable[trxId];

		lockManager->releaseLockTableMutex();

		return;
	}

	// Append commit log
	ofstream fout("thread" + to_string(threadId) + ".txt", ios::app);
	fout << commitId << " ";
	for (auto idx : recordIdx)
		fout << idx << " ";
	for (auto h : trxTable[trxId]->history)
		fout << h << " ";
	fout.close();

	delete trxTable[trxId];

	lockManager->releaseLockTableMutex();
}

void Transaction::trxRollback(int trxId) {
	lockManager->acquireLockTableMutex();

	// Undo phase
	for (auto undoItem : trxTable[trxId]->undoLog)
		database->updateRecord(undoItem.first, undoItem.second);

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	delete trxTable[trxId];

	lockManager->releaseLockTableMutex();
}

int64_t Transaction::trxRead(int trxId, int rid, int* deadlockFlag) {
	lockManager->acquireLockTableMutex();

	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, S_MODE);
	if (lockObj == DEADLOCK) {
		*deadlockFlag = 1;
		return 0;
	}
	else {
		// Update a transaction operation list
		updateTrxTable(trxId, lockObj);

		// Logging operation result for commit log
		trxTable[trxId]->history.push_back(lockObj->sentinel->key);
	}

	lockManager->releaseLockTableMutex();

	return database->readRecord(rid);
}

void Transaction::trxWrite(int trxId, int rid, int64_t value, int* deadlockFlag) {
	lockManager->acquireLockTableMutex();

	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, X_MODE);

	if (lockObj == DEADLOCK) {
		*deadlockFlag = 1;
		return;
	}
	else {
		// Update a transaction operation list
		updateTrxTable(trxId, lockObj);

		// Logging oldest value
		if (trxTable[trxId]->undoLog.find(rid) == trxTable[trxId]->undoLog.end())
			trxTable[trxId]->undoLog[rid] = lockObj->sentinel->key;

		// Update lock object's key value
		lockObj->sentinel->key += value;

		// Logging operation result for commit log
		trxTable[trxId]->history.push_back(lockObj->sentinel->key);
	}

	lockManager->releaseLockTableMutex();

	database->updateRecord(rid, lockObj->sentinel->key);
}

int Transaction::startTrx(vector<int> recordIdx, int tid) {
	threadId = tid;
	int* deadlockFlag = new int;

	while (true) {
		int trxId = trxBegin();
		int readValue;
		*deadlockFlag = 0;

		readValue = trxRead(trxId, recordIdx[0], deadlockFlag);
		if (*deadlockFlag == 1) {
			trxRollback(trxId);
			continue;
		}

		trxWrite(trxId, recordIdx[1], readValue + 1, deadlockFlag);
		if (*deadlockFlag == 1) {
			trxRollback(trxId);
			continue;
		}

		trxWrite(trxId, recordIdx[2], -readValue, deadlockFlag);
		if (*deadlockFlag == 1) {
			trxRollback(trxId);
			continue;
		}

		trxCommit(trxId, recordIdx);
		break;
	}

	delete deadlockFlag;
}

void Transaction::setLockManager(Lock* l) {
	lockManager = l;
}