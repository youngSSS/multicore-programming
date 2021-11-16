#include "ConcurrencyControl.hpp"

vector<int> Lock::getTrxWaitingList(lock_t* lockObj) {
	vector<int> trxWaitingList;

	// Case: lockObj is working
	if (lockObj->condition == WORKING) return trxWaitingList;

	// Case: lockObj is waiting
	else {
		if (lockObj->lockMode == S_MODE) {
			while (lockObj != nullptr) {
				// Case: Waiting for working locks
				if (lockObj->condition == WORKING) {
					while (lockObj != nullptr) {
						trxWaitingList.push_back(lockObj->tSentinel->trxId);
						lockObj = lockObj->lPrev;
					}

					break;
				}

				// Case: Waiting for X mode waiting lock
				if (lockObj->lockMode == X_MODE) {
					trxWaitingList.push_back(lockObj->tSentinel->trxId);
					break;
				}

				lockObj = lockObj->lPrev;
			}
		}
		else {

		}
	}

}

/* Return values
 * 0: No deadlock
 * 1: Deadlock
 */
int Lock::detectDeadlock(lock_t* lockObj) {
	set<int> waitForSet;

	vector<int> trxWaitingList = getTrxWaitingList(lockObj);



	return 0;
}

/* Return values
 * 0: Impossible cases
 * 1: Acquire with linking
 * 2: Acquire without linking
 * 3: Upgrade to X_MODE & Acquire without linking
 * 4: Detect deadlock (deadlock or wait)
 * 5: Deadlock
 */
int Lock::analyzeLockTable(int trxId, int rid, int lockMode, lock_t* lockObj) {
	// Nothing in lock table list
	if (lockTable.find(rid) == lockTable.end()) {
		lockTable[rid] = new lockTable_t(rid);
		return 1;
	}

	// Find a transaction with trxId in lock table
	int trxExistFlag = 0;
	lockObj = lockTable[rid]->head;
	while (lockObj != nullptr) {
		if (lockObj->tSentinel->trxId == trxId) {
			trxExistFlag = 1;
			break;
		}
		lockObj = lockObj->lNext;
	}

	if (trxExistFlag) {
		int ownLockMode = lockObj->lockMode;
		int ownLockCondition = lockObj->condition;

		// Case: Transaction has S mode WORKING lock
		if (ownLockMode == S_MODE && ownLockCondition == WORKING) {
			if (lockMode == S_MODE) return 2;
			if (lockMode == X_MODE) {
				// Case: Working alone
				if (lockObj->lSentinel->head == lockObj && (lockObj->lNext == nullptr || lockObj->lNext->condition == WAITING)) return 3;

				// Case: Working together without waiting lock
				if (lockObj->lSentinel->tail->condition == WORKING) return 4;
				// Case: Working together with waiting lock
				else return 5;
			}
		}

		// Case: Transaction has X mode WORKING lock
		if (ownLockMode == X_MODE && ownLockCondition == WORKING) return 2;
	}

	else {
		lockObj = lockTable[rid]->tail;
		int tailLockMode = lockObj->lockMode;
		int tailCondition = lockObj->condition;

		// Case: Tail is S mode WORKING lock
		if (tailLockMode == S_MODE && tailCondition == WORKING && lockMode == S_MODE) return 1;
		if (tailLockMode == S_MODE && tailCondition == WORKING && lockMode == X_MODE) return 4;

		// Case: Tail is X mode WORKING lock
		if (tailLockMode == X_MODE && tailCondition == WORKING) return 4;

		// Case: Tail is S mode WAITING lock
		if (tailLockMode == S_MODE && tailCondition == WAITING) return 4;

		// Case: Tail is X mode WAITING lock
		if (tailLockMode == X_MODE && tailCondition == WAITING) return 4;
	}

	return 0;
}

lock_t* Lock::acquireRecordLock(int trxId, int rid, int lockMode) {
	lock_t* lockObj = nullptr;
	int caseNum = analyzeLockTable(trxId, rid, lockMode, lockObj);

	/* caseNum
	 * 0: Impossible cases
	 * 1: Acquire with linking
	 * 2: Acquire without linking
	 * 3: Upgrade to X_MODE & Acquire without linking
	 * 4: Detect deadlock (deadlock or wait)
	 * 5: Deadlock
	 */

	// Impossible
	if (caseNum == 0) return nullptr;

	// Acquire with linking
	if (caseNum == 1) {
		auto* newLockObj = new lock_t(lockMode, WAITING, lockTable[rid]);
		if (lockObj == nullptr) newLockObj->condition = WORKING;

		// Append new lock object to lock table
		if (lockTable[rid]->head == nullptr) {
			lockTable[rid]->head = newLockObj;
			lockTable[rid]->tail = newLockObj;
		}
		else {
			lockTable[rid]->tail->lNext = newLockObj;
			newLockObj->lPrev = lockTable[rid]->tail;
			lockTable[rid]->tail = newLockObj;
		}

		// Append new lock object to transaction table
		trxManager->updateTrxTable(trxId, newLockObj);

		return newLockObj;
	}

	// Acquire without linking
	if (caseNum == 2) return lockObj;

	// Upgrade to X_MODE & Acquire without linking
	if (caseNum == 3) {
		lockObj->lockMode = X_MODE;
		return lockObj;
	}

	// Detect deadlock (deadlock or wait)
	if (caseNum == 4) {
		// Wait
		auto* newLockObj = new lock_t(lockMode, WAITING, lockTable[rid]);

		// Append new lock object to lock table
		lockTable[rid]->tail->lNext = newLockObj;
		newLockObj->lPrev = lockTable[rid]->tail;
		lockTable[rid]->tail = newLockObj;

		// Append new lock object to transaction table
		trxManager->updateTrxTable(trxId, newLockObj);

		// Detect deadlock
		int isDeadlock = detectDeadlock(newLockObj);

		// Case: Deadlock
		if (isDeadlock) return DEADLOCK;

		return newLockObj;
	}

	// Deadlock
	if (caseNum == 5) return DEADLOCK;
}

void Lock::releaseRecordLock(lock_t* lockObj) {

}

void Lock::acquireLockTableMutex() {
	lockTableMutex.lock();
}

void Lock::releaseLockTableMutex() {
	lockTableMutex.unlock();
}

void Lock::setTrxManager(Transaction* t) {
	trxManager = t;
}