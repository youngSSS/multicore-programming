# Project Structure
This project is consist of four concepts.

1. Two Phase Locking Runner
2. Database
3. Lock Manager
4. Transaction Manager

## Two Phase Locking Runner

**include/TwoPhaseLockingRunner.hpp**

This makes threads and start two phase locking routine.

## Database (include/Database.hpp)

**include/Database.hpp**

This is the database of project.

Each record is a 64 bit signed integer (initial value: 100).

## Lock Manager

**src/Lock.cpp**

**include/ConcurrencyControl.hpp**

Lock Manager manages acquire and release of record lock and protect lock table with the global mutex.

## Transaction Manager

**src/Transaction.cpp**

**include/ConcurrencyControl.hpp**

Transaction Manager manages the operations of transaction.

Transaction Manager's main role is listed in below.
1. Release the record lock, when transaction finished it work.
2. Undo the result of each operation, when rollback happens.
3. Traverse the wait-for graph and detect deadlock. If there's a cycle, it's deadlock.

# Lock Acquire

# Lock Release

# Deadlock Detection