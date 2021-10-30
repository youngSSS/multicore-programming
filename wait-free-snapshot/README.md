# Wait-free Snapshot

## Logic

### Snapshot

Snapshot is needed to keep a track of linearizable point.

### At most three

We cannot find linearizable schedule.

<br/>

## TestRunner.hpp

### CLASS - TestRunner

In class `TestRunner`, method `startUpdateTest()` updates each thread's local value to random value for 1 minute.

<br/>

## WaitFreeSnapshot.hpp

### CLASS - SnapValue

The variable `label` is the unique number to distinguish updates. Without `label`, we cannot recognize whether an update happened or not when the updated value is the same as the old one.

The variable `value` is the local value of the thread.

The variable `snapshot` is the latest valid snapshot of the system.

### CLASS - WaitFreeSnapshot

The variable `snapValues` is the status of the system. It has the whole status of each thread (`snapValue`).

Method `scan()` scans the status of the system and returns the latest valid snapshot of the system.

Method `update(updateValue, threadId)` updates the local value of the thread. It should take a snapshot before updating.

<br/>

## Result

- Run with 1 thread -> 18,205,259
- Run with 2 thread -> 27,664,085
- Run with 4 thread -> 37,446,084
- Run with 8 thread -> 30,980,002
- Run with 16 thread -> 19,102,499
- Run with 32 thread -> 8,652,252
