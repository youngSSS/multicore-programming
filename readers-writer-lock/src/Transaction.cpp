#include "ConcurrencyControl.hpp"

// This method already has the global mutex
void Transaction::releaseAcquiredLocks(int trxId) {
	lock_t* lockObj = trxTable[trxId]->head;
	lock_t* nextLockObj;

	while (lockObj != nullptr) {
		nextLockObj = lockObj->tNext;
		lockManager->releaseRecordLock(lockObj);
		lockObj = nextLockObj;
	}
}

int Transaction::trxBegin() {
	// +++++++++ Acquire the global mutex +++++++++
	lockManager->acquireLockTableMutex();
	// ++++++++++++++++++++++++++++++++++++++++++++

	int id = trxIdSeq++;
	trxTable[id] = new trxTable_t(id);

	// --------- Release the global mutex ---------
	lockManager->releaseLockTableMutex();
	// --------------------------------------------

	return id;
}

int Transaction::trxCommit(int trxId, vector<int> recordIdx) {
	// +++++++++ Acquire the global mutex +++++++++
	lockManager->acquireLockTableMutex();
	// ++++++++++++++++++++++++++++++++++++++++++++

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	// Increase the global execution order by 1
	int commitId = ++numExecution;

	// Case: Commit count exceeds a execution limit
	if (commitId > executionLimit) {
		// Undo phase
		for (auto undoItem : trxTable[trxId]->undoLog)
			database->updateRecord(undoItem.first, undoItem.second);

		// Delete a transaction from trxTable
		delete trxTable[trxId];
		trxTable.erase(trxId);
	}
	else {
		// Append results to the commit log
		auto iter = find(threadId.begin(), threadId.end(), boost::this_thread::get_id());
		ofstream fout("thread" + to_string(iter - threadId.begin()) + ".txt", ios::app);
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
	}

	// --------- Release the global mutex ---------
	lockManager->releaseLockTableMutex();
	// --------------------------------------------

	if (commitId > executionLimit) return 1;
	else return 0;
}

// This method already has the global mutex
void Transaction::trxRollback(int trxId) {
	// Undo phase
	for (auto undoItem : trxTable[trxId]->undoLog)
		database->updateRecord(undoItem.first, undoItem.second);

	// Two-phase locking, release acquired locks
	releaseAcquiredLocks(trxId);

	// Delete a transaction from trxTable
	delete trxTable[trxId];
	trxTable.erase(trxId);
}

int Transaction::trxRead(int trxId, int rid, int64_t* readValue) {
	// +++++++++ Acquire the global mutex +++++++++
	lockManager->acquireLockTableMutex();
	// ++++++++++++++++++++++++++++++++++++++++++++

	// Acquire the record lock
	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, S_MODE);

	// If deadlock occurred, rollback the transaction
	// Otherwise, save the result for commit log
	if (lockObj == DEADLOCK) trxRollback(trxId);
	else trxTable[trxId]->history.push_back(lockObj->lSentinel->key);

	// --------- Release the global mutex ---------
	lockManager->releaseLockTableMutex();
	// --------------------------------------------

	if (lockObj == DEADLOCK) return 1;

	*readValue = database->readRecord(rid);
	return 0;
}

int Transaction::trxWrite(int trxId, int rid, int64_t value) {
	// +++++++++ Acquire the global mutex +++++++++
	lockManager->acquireLockTableMutex();
	// ++++++++++++++++++++++++++++++++++++++++++++

	// Acquire the record lock
	lock_t* lockObj = lockManager->acquireRecordLock(trxId, rid, X_MODE);
	int64_t updatedValue;

	// If deadlock occurred, rollback the transaction
	if (lockObj == DEADLOCK) trxRollback(trxId);
	else {
		// Save the oldest value for undo
		if (trxTable[trxId]->undoLog.find(rid) == trxTable[trxId]->undoLog.end())
			trxTable[trxId]->undoLog[rid] = database->readRecord(rid);

		// Save updated lock object value for commit log
		updatedValue = database->readRecord(rid) + value;
		trxTable[trxId]->history.push_back(updatedValue);
	}

	// --------- Release the global mutex ---------
	lockManager->releaseLockTableMutex();
	// --------------------------------------------

	if (lockObj == DEADLOCK) return 1;

	database->updateRecord(rid, updatedValue);
	return 0;
}

int Transaction::startTrx(vector<int> recordIdx, int tid) {
	// Set thread id for log file
	threadId[tid] = boost::this_thread::get_id();

	while (true) {
		int trxId = trxBegin();
		auto* readValue = new int64_t;

		// If deadlock occurred, skip a rest operations and restart transaction from beginning
		// Rollback is already operated in trxRead or trxWrite when deadlock is detected
		if (trxRead(trxId, recordIdx[0], readValue) == 1) continue;
		if (trxWrite(trxId, recordIdx[1], *readValue + 1) == 1) continue;
		if (trxWrite(trxId, recordIdx[2], -*readValue) == 1) continue;

		// Success all operations, commit a transaction
		delete readValue;
		return trxCommit(trxId, recordIdx);
	}
}

void Transaction::setLockManager(Lock* l) {
	lockManager = l;
}

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

/* Return values
 * 0: No deadlock
 * 1: Deadlock
 */
int Transaction::detectDeadlock(lock_t* lockObj, set<int>& waitForSet, int targetTrxId) {
	// If lockObj is working lock, it's not waiting anything
	if (lockObj->condition == WORKING) return 0;

	// Case: Special case, transaction waits its own but it's not deadlock
	if (lockObj->lockMode == X_MODE) {
		lock_t* prevLockObj = lockObj->lPrev;

		if (prevLockObj->lockMode == S_MODE && prevLockObj->condition == WORKING) {
			int ownFlag = 0;
			while (prevLockObj != nullptr && ownFlag == 0) {
				if (prevLockObj->tSentinel->trxId == targetTrxId)
					ownFlag = 1;
				prevLockObj = prevLockObj->lPrev;
			}

			if (ownFlag) return 0;
		}
	}

	// Case: lockObj is waiting lock
	if (lockObj->lockMode == S_MODE) {
		lockObj = lockObj->lPrev;

		if (lockObj->lockMode == S_MODE) {
			while (lockObj != nullptr && lockObj->lockMode == S_MODE)
				lockObj = lockObj->lPrev;
		}

		waitForSet.insert(lockObj->tSentinel->trxId);

		// Deadlock
		if (waitForSet.find(targetTrxId) != waitForSet.end()) return 1;

		lock_t* trxLockObj = trxTable[lockObj->tSentinel->trxId]->head;
		while (trxLockObj != nullptr) {
			// Deadlock
			if (detectDeadlock(trxLockObj, waitForSet, targetTrxId)) return 1;
			trxLockObj = trxLockObj->tNext;
		}
	}
	else if (lockObj->lockMode == X_MODE) {
		lockObj = lockObj->lPrev;

		if (lockObj->lockMode == S_MODE) {
			while (lockObj != nullptr && lockObj->lockMode == S_MODE) {
				waitForSet.insert(lockObj->tSentinel->trxId);

				if (waitForSet.find(targetTrxId) != waitForSet.end()) return 1;

				lock_t* trxLockObj = trxTable[lockObj->tSentinel->trxId]->head;
				while (trxLockObj != nullptr) {
					// Deadlock
					if (detectDeadlock(trxLockObj, waitForSet, targetTrxId)) return 1;
					trxLockObj = trxLockObj->tNext;
				}

				lockObj = lockObj->lPrev;
			}
		}
		else if (lockObj->lockMode == X_MODE) {
			waitForSet.insert(lockObj->tSentinel->trxId);

			// Deadlock
			if (waitForSet.find(targetTrxId) != waitForSet.end()) return 1;

			lock_t* trxLockObj = trxTable[lockObj->tSentinel->trxId]->head;
			while (trxLockObj != nullptr) {
				// Deadlock
				if (detectDeadlock(trxLockObj, waitForSet, targetTrxId)) return 1;
				trxLockObj = trxLockObj->tNext;
			}
		}
	}

	return 0;
}