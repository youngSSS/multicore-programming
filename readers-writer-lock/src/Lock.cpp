#include "ConcurrencyControl.hpp"

lock_t* Lock::acquireRecordMutex(int trxId, int rid, int lockMode) {

}

void Lock::releaseRecordMutex(int trxId, int rid, int lockMode) {

}

void Lock::acquireLockTableMutex() {
	lockTableMutex.lock();
}

void Lock::releaseLockTableMutex() {
	lockTableMutex.unlock();
}