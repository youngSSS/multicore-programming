This project is consists of four concepts.

1. Two-Phase Locking Runner
2. Database
3. Lock Manager
4. Transaction Manager

<br/>

# Two Phase Locking Runner

Code: `include/TwoPhaseLockingRunner.hpp`

This makes threads and starts a two-phase locking routine.

<br/>

# Database (include/Database.hpp)

Code: `include/Database.hpp`

This is the database of the project.

Each record is a 64 bit signed integer (initial value: 100).

<br/>

# Lock Manager

Code: `src/Lock.cpp`, `include/ConcurrencyControl.hpp`

Lock Manager manages acquirement and release of record lock and protects lock table with the global mutex.

<br/>

# Transaction Manager

Code: `src/Transaction.cpp`, `include/ConcurrencyControl.hpp`

Transaction Manager manages the operations of transactions.

Transaction Manager's main role is listed below.
1. Release the record lock, when the transaction is finished its work.
2. Undo the result of each operation, when rollback happens.
3. Traverse the wait-for graph and detect deadlock. If there's a cycle, it's deadlock.

<br/>

# Lock Acquire

Acquire the record lock.

There are many cases. Specification about cases is below.

## 1. Nothing in the lock table

![Untitled](uploads/068e9c3ce759b22681a092c941656431/Untitled.png)

1. S Lock → `Acquire the record lock`
2. X Lock → `Acquire the record lock`

## 2. The lock table has my transaction

### 2-1. My transaction has S mode WORKING lock

1. S Lock → `Acquire the record lock`
    
    ![1](uploads/565aff49b0e9a71222ac64ce3dd9df4c/1.png)
    
2. X Lock
    1. My transaction's S mode WORKING lock is working alone → `Upgrade the record lock to X mode and acquire the record lock`
        
        ![2](uploads/7d173040eedac1e3d5ccc8ab15bfe7e8/2.png)
        
        ![3](uploads/741928f759076dfe93729e4ced27ed99/3.png)
        
    2. My transaction's S mode WORKING lock is working together with other transactions
        1. There's WAITING lock → `Deadlock`
        2. No WAITING lock → `If there's no deadlock, wait to get a record lock`
            
            ![4](uploads/9de83a0a5895e61915ba42cac6e398f0/4.png)
            

### 2-2. My transaction has X mode WORKING lock

![5](uploads/fe0a23aadc2ff10b90f2aaa557ea91bf/5.png)

1. S Lock → `Acquire the record lock`
2. X Lock → `Acquire the record lock`

### 2-3. My transaction has a WAITING lock

![6](uploads/35933263b63042a15e07358d0f157936/6.png)

Impossible case.

## 3. The lock table does not have my transaction

### 3-1. The tail of the lock table is S mode WORKING lock

1. S Lock → `Acquire the record lock`
    
    ![7](uploads/702dd4969730591713f5ecf2f92b8bce/7.png)
    
2. X Lock → `If there's no deadlock, wait to get a record lock`
    
    ![8](uploads/9dad05c79799f6aad565b1f9c0a49614/8.png)
    

### 3-2. The tail of the lock table is X mode WORKING lock

![9](uploads/3573365ee028872a30b4bbd2956a3f62/9.png)

1. S Lock → `If there's no deadlock, wait to get a record lock`
2. X Lock → `If there's no deadlock, wait to get a record lock`

### 3-3. The tail of the lock table is S mode WAITING lock

![10](uploads/3890dc216eac73e14c8d4ca1c49ae1ef/10.png)

1. S Lock → `If there's no deadlock, wait to get a record lock`
2. X Lock → `If there's no deadlock, wait to get a record lock`

### 3-4. The tail of the lock table is X mode WAITING lock

![11](uploads/b24d8964ece23434570e5b852dffe2e8/11.png)

1. S Lock → `If there's no deadlock, wait to get a record lock`
2. X Lock → `If there's no deadlock, wait to get a record lock`


# Lock Release

Release the record lock acquired by the transaction and send a signal to an appropriate lock object.

There are many cases. Specification about cases is below.

## 1. The lock table has only one lock

![12](uploads/07b5cd6d819292563215034cd56d2db0/12.png)

## 2. There's another WORKING lock in the lock table

![13](uploads/7669ba58fcb56675af62d278fc916f79/13.png)

## 3. There's only one WORKING lock and others are WAITING lock

![14](uploads/b52453d2fe5e8a7c3aed5d605b6cf101/14.png)
![15](uploads/58d374e09b31695693749433aed11518/15.png)

## 4. Release target is WAITING lock (This happens when the transaction is aborted)

![16](uploads/58d97835898bcc22e361d7f09108db2b/16.png)

## 5. The transaction which has S mode lock is waiting to get X mode lock

![17](uploads/4cfca03d05c7d74a6638dbfb84d543cd/17.png)

In this picture, TRX1 already has an S lock. But TRX1 is waiting for TRX2 to get an X lock.

In this case, TRX2 should send a signal to TRX1 to make a situation like fig 3.

# Deadlock Detection

1. Make a wait-for graph using a lock table.
2. Traverse a wait-for graph. If there's a cycle, it's a deadlock.