#include "ConcurrencyControl.hpp"

#include <iostream>

void Lock::printLockTable() {
	cout << "########## Lock Table ##########" << endl;
	for (auto l : lockTable) {
		lock_t* lockObj = l.second->head;
		cout << "rid(" << l.first << ") - ";

		while (lockObj != nullptr) {
			if (lockObj->tSentinel->trxId == 0) {

			}

			cout << "[ " << "trxId(" << lockObj->tSentinel->trxId << "), " << (lockObj->lockMode == S_MODE ? "S" : "X")
				<< ", " << (lockObj->condition == WORKING ? "W" : "Z") << " ], ";
			lockObj = lockObj->lNext;
		}
		cout << endl;
	}
	cout << endl;
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
		if (lockTable[rid] == nullptr) {

		}
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

		// Append a new lock object to transaction table
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
		set<int> waitForSet;
		int isDeadlock = trxManager->detectDeadlock(newLockObj, waitForSet, trxId);

		// Case: Deadlock
		if (isDeadlock) return DEADLOCK;

		// Wait
		newLockObj->cond.wait(lockTableLockers[trxManager->getThreadId(boost::this_thread::get_id())]);

		return newLockObj;
	}

	// Deadlock
	if (caseNum == 5) return DEADLOCK;
}

void Lock::releaseRecordLock(lock_t* lockObj) {
	int rid = lockObj->lSentinel->key;
	lock_t* prevLockObj, *nextLockObj;

	// Case: Special case, lock release by abort
	if (lockObj->condition == WAITING) {
		prevLockObj = lockObj->lPrev;
		nextLockObj = lockObj->lNext;

		if (nextLockObj == nullptr) {
			lockTable[rid]->tail = lockObj->lPrev;
			prevLockObj->lNext = nullptr;
		}

		else if (lockObj->lockMode == X_MODE &&
			prevLockObj->condition == WORKING && prevLockObj->lockMode == S_MODE &&
			nextLockObj->condition == WAITING && nextLockObj->lockMode == S_MODE)
		{
			// Delete lockObj on lock table list
			prevLockObj->lNext = nextLockObj;
			nextLockObj->lPrev = prevLockObj;

			// Send signal to workable locks
			while (nextLockObj != nullptr && nextLockObj->lockMode == S_MODE) {
				nextLockObj->condition = WORKING;
				nextLockObj->cond.notify_one();
				nextLockObj = nextLockObj->lNext;
			}
		}

		delete lockObj;
		return;
	}

	// Case: Alone in the lock table list
	if (lockTable[rid]->head == lockObj && lockTable[rid]->tail == lockObj) {
		// Delete the entry from lock table
		delete lockTable[rid];
		lockTable.erase(rid);
	}
	else {
		// Next lock object of lockObj
		nextLockObj = lockObj->lNext;
		prevLockObj = lockObj->lPrev;

		// Case: Not alone in the lock table list, and working together
		if ((prevLockObj != nullptr && prevLockObj->condition == WORKING) || (nextLockObj != nullptr && nextLockObj->condition == WORKING)) {
			if (prevLockObj != nullptr && nextLockObj != nullptr) {
				prevLockObj->lNext = nextLockObj;
				nextLockObj->lPrev = prevLockObj;
			}
			else if (prevLockObj != nullptr) {
				lockTable[rid]->tail = prevLockObj;
				prevLockObj->lNext = nullptr;
			}
			else if (nextLockObj != nullptr) {
				lockTable[rid]->head = nextLockObj;
				nextLockObj->lPrev = nullptr;
			}

			prevLockObj = lockTable[rid]->head;
			nextLockObj = lockTable[rid]->head->lNext;

			// Case: Special case, the transaction which already has s mode lock is waiting to get X mode lock
			if (nextLockObj != nullptr && nextLockObj->condition == WAITING && prevLockObj->tSentinel->trxId == nextLockObj->tSentinel->trxId) {
				lockTable[rid]->head = nextLockObj;
				nextLockObj->lPrev = nullptr;
				nextLockObj->condition = WORKING;

				nextLockObj->cond.notify_one();

				delete prevLockObj;
			}
		}

		// Case: Not alone in the lock table list, but working alone
		else if (nextLockObj->condition == WAITING) {
			lockTable[rid]->head = nextLockObj; // Change the head of lock table to nextLockObj
			nextLockObj->lPrev = nullptr; 		// Delete lockObj pointer in nextLockObj

			// Next lock is the X mode waiting lock
			if (nextLockObj->lockMode == X_MODE) {
				nextLockObj->condition = WORKING;
				nextLockObj->cond.notify_one();
			}
			// Next lock is the S mode waiting lock
			else if (nextLockObj->lockMode == S_MODE) {
				while (nextLockObj != nullptr && nextLockObj->lockMode == S_MODE) {
					nextLockObj->condition = WORKING;
					nextLockObj->cond.notify_one();
					nextLockObj = nextLockObj->lNext;
				}
			}
		}
	}

	// Delete the lock object
	delete lockObj;
}

void Lock::acquireLockTableMutex() {
	lockTableLockers[trxManager->getThreadId(boost::this_thread::get_id())].lock();
}

void Lock::releaseLockTableMutex() {
	lockTableLockers[trxManager->getThreadId(boost::this_thread::get_id())].unlock();
}

void Lock::setTrxManager(Transaction* t) {
	trxManager = t;
}

void Lock::getWaitForGraph(unordered_map< int, vector<int> >& nodes) {
	// Traverse lockTable
	for (auto lockList : lockTable) {
		lock_t* baseLockObj = lockList.second->tail;

		while (baseLockObj != nullptr) {
			lock_t* lockObj = baseLockObj;
			int trxId = lockObj->tSentinel->trxId;

			// Append a relation to wait-for graph
			if (lockObj->condition == WAITING) {
				if (lockObj->lockMode == S_MODE) {
					lockObj = lockObj->lPrev;

					if (lockObj->lockMode == S_MODE) {
						while (lockObj->lockMode == S_MODE)
							lockObj = lockObj->lPrev;
					}

					nodes[trxId].push_back(lockObj->tSentinel->trxId);
				}
				else if (lockObj->lockMode == X_MODE) {
					lockObj = lockObj->lPrev;

					if (lockObj->lockMode == S_MODE) {
						while (lockObj != nullptr && lockObj->lockMode == S_MODE) {
							nodes[trxId].push_back(lockObj->tSentinel->trxId);
							lockObj = lockObj->lPrev;
						}
					}
					else if (lockObj->lockMode == X_MODE) {
						nodes[trxId].push_back(lockObj->tSentinel->trxId);
					}
				}
			}

			baseLockObj = baseLockObj->lPrev;
		}
	}
}