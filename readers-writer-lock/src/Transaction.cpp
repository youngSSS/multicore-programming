#include "ConcurrencyControl.hpp"

void Transaction::updateTrxTable(int trxId, lock_t* lockObj) {
	lockObj->tSentinel = trxTable[trxId];

	if (trxTable[trxId]->head == nullptr) {
		trxTable[trxId]->head = lockObj;
		trxTable[trxId]->tail = lockObj;
	}
	else {
		trxTable[trxId]->tail->tNext = lockObj;
		lockObj->tPrev = trxTable[trxId]->tail;
		trxTable[trxId]->tail = lockObj;
	}
}

void Transaction::releaseAcquiredLocks(int trxId) {
	lock_t* lockObj = trxTable[trxId]->head;
	lock_t* nextLockObj;

	while (lockObj != nullptr) {
		nextLockObj = lockObj->lNext;
		lockManager->releaseRecordLock(lockObj);
		lockObj = nextLockObj;
	}
}

int Transaction::trxBegin() {
	// Acquire the global mutex
	lockManager->acquireLockTableMutex();

	int id = trxIdSeq++;
	trxTable[id] = new trxTable_t(id);

	// Release the global mutex
	lockManager->releaseLockTableMutex();

	return id;
}

int Transaction::trxCommit(int trxId, vector<int> recordIdx) {
	// Acquire the global mutex
	lockManager->acquireLockTableMutex();

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	// Increase the global execution order by 1
	int commitId = ++numExecution;

	// Case: Commit count exceeds a execution limit
	if (commitId > executionLimit) {
		// Undo phase
		for (auto undoItem : trxTable[trxId]->undoLog)
			database->updateRecord(undoItem.first, undoItem.second);

		delete trxTable[trxId];

		lockManager->releaseLockTableMutex();

		return 1;
	}

	// Append results to the commit log
	ofstream fout("thread" + to_string(threadId) + ".txt", ios::app);
	fout << commitId << " ";
	for (auto idx : recordIdx)
		fout << idx << " ";
	for (auto h : trxTable[trxId]->history)
		fout << h << " ";
	fout << "\n\n";
	fout.close();

	// Delete a transaction from trxTable
	delete trxTable[trxId];
	trxTable.erase(trxId);

	// Release the global mutex
	lockManager->releaseLockTableMutex();

	return 0;
}

void Transaction::trxRollback(int trxId) {
	// Acquire the global mutex
	lockManager->acquireLockTableMutex();

	// Undo phase
	for (auto undoItem : trxTable[trxId]->undoLog)
		database->updateRecord(undoItem.first, undoItem.second);

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	// Delete a transaction from trxTable
	delete trxTable[trxId];
	trxTable.erase(trxId);

	// Release the global mutex
	lockManager->releaseLockTableMutex();
}

int64_t Transaction::trxRead(int trxId, int rid, int* deadlockFlag) {
	// Acquire the global mutex
	lockManager->acquireLockTableMutex();

	// Acquire the record lock
	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, S_MODE);

	// Case: Deadlock
	if (lockObj == DEADLOCK) {
		*deadlockFlag = 1;
		// Release the global mutex
		lockManager->releaseLockTableMutex();
		return 0;
	}
	// Case: Success to acquire the record lock
	else {
		// Save result for commit log
		trxTable[trxId]->history.push_back(lockObj->lSentinel->key);
	}

	// Release the global mutex
	lockManager->releaseLockTableMutex();

	return database->readRecord(rid);
}

void Transaction::trxWrite(int trxId, int rid, int64_t value, int* deadlockFlag) {
	// Acquire the global mutex
	lockManager->acquireLockTableMutex();

	// Acquire the record lock
	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, X_MODE);

	// Case: Deadlock
	if (lockObj == DEADLOCK) {
		*deadlockFlag = 1;
		// Release the global mutex
		lockManager->releaseLockTableMutex();
		return;
	}
	// Case: Success to acquire the record lock
	else {
		// Save oldest value for undo
		if (trxTable[trxId]->undoLog.find(rid) == trxTable[trxId]->undoLog.end())
			trxTable[trxId]->undoLog[rid] = lockObj->lSentinel->key;

		// Update lock object's key value
		lockObj->lSentinel->key += value;

		// Save result for commit log
		trxTable[trxId]->history.push_back(lockObj->lSentinel->key);
	}

	// Release the global mutex
	lockManager->releaseLockTableMutex();

	database->updateRecord(rid, lockObj->lSentinel->key);
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

		delete deadlockFlag;
		return trxCommit(trxId, recordIdx);
	}
}

void Transaction::setLockManager(Lock* l) {
	lockManager = l;
}