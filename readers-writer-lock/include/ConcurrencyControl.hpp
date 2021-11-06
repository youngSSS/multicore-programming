#ifndef READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
#define READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_

#include "Database.hpp"

using namespace std;

class Transaction;
class Lock;

class Transaction {
 private:
	Database* database;
	Lock* lock;

 public:
	Transaction(Database* db, Lock* l) : database(db), lock(l) {}

};

class Lock {
 private:
	Database* database;
	Transaction* transaction;

 public:
	Lock(Database* db, Transaction* t) : database(db), transaction(t) {}
};

#endif //READERS_WRITER_LOCK_INCLUDE_CONCURRENCYCONTROL_HPP_
