# Wait-free Snapshot

## Logic

<br/>

## TestRunner.hpp

### CLASS - TestRunner

In `TestRunner` class, `startUpdateTest()` function updates each thread's local value for 1 minute.

<br/>

## WaitFreeSnapshot.hpp

### CLASS - SnapValue

Class `SnapValue` has label, value, and snapshot.

Variable `label` is the unique number for update.

Variable `value` is the local value of thread.

Variable `snapshot` is the latest valid snapshot of thread.

### CLASS - WaitFreeSnapshot

Variable `snapValues` is the status of whole threads.

Method `scan` scans `snapValues`.

Method `update` updates the local value of thread. It should take snapshot before updates the local value.

<br/>

## Result

- Run with 1 thread -> 18,205,259
- Run with 2 thread -> 27,664,085
- Run with 4 thread -> 37,446,084
- Run with 8 thread -> 30,980,002
- Run with 16 thread -> 19,102,499
- Run with 32 thread -> 8,652,252
