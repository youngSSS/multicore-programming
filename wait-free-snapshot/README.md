# Wait-free Snapshot

This is the WIKI of `Wait-free Snapshot`.

<br/>

# Logic

## Snapshot

The snapshot is needed to keep a track of the linearizable point.

## At most three collect

### Why we cannot finish in two collect

When we fail to pick a snapshot at once, we pick the other updater's snapshot as ours.

In this case, if we finish on the second try, we cannot know whether the other updater's snapshot had been made after our invocation or not. If we pick the snapshot which had been made before our invocation, we cannot find a linearizable schedule.

Therefore by picking the snapshot on the third try, we can guarantee that snapshot had been made after our invocation.

<br/>

# Source Code

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

# Result

- Case 1: Run with 1 thread -> 18,205,259
- Case 2: Run with 2 thread -> 27,664,085
- Case 3: Run with 4 thread -> 37,446,084
- Case 4: Run with 8 thread -> 30,980,002
- Case 5: Run with 16 thread -> 19,102,499
- Case 6: Run with 32 thread -> 8,652,252

## Analysis

From case 1 to case 3, the number of updates has been increased. Because many threads work together and there are only a few conflicts.

But from case 3 to case 6, the number of updates has been decreased. It is expected to increase because more threads work together, but at the same time, the number of conflicts has been increased too, as a result, the number of updates has been decreased.
