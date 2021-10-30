#include <iostream>
#include <vector>

using namespace std;

class SnapValue {
    public:
        int label;
        int value;
        vector<int> snapshot;

        // Constructor
        SnapValue(int l, int v) : label(l), value(v) {}
        SnapValue(int l, int v, vector<int> s) : label(l), value(v), snapshot(s) {}
};

class WaitFreeSnapshot {
    private:
        vector<SnapValue> snapValues;

        vector<int> scan() {
            vector<SnapValue> oldCopy, newCopy;
            vector<int> moved(snapValues.size(), 0);
            vector<int> scanResult(snapValues.size());

            oldCopy = snapValues;

            while (true) {
                newCopy = snapValues;
                
                int secondFlag = 0;
                for (int i = 0; i < snapValues.size(); i++) {
                    // oldLabel != newLabel means update happened
                    if (oldCopy[i].label != newCopy[i].label) {
                        if (moved[i]) { // Second try
                            return newCopy[i].snapshot;
                        }
                        else { // First try
                            moved[i] = 1;
                            oldCopy = newCopy;
                            secondFlag = 1;
                            break;
                        }
                    }
                }

                if (secondFlag) continue;

                for (int i = 0; i < snapValues.size(); i++)
                    scanResult[i] = newCopy[i].value;

                return scanResult;
            }
        }

    public:
        // Constructor
        WaitFreeSnapshot(int numThread) : snapValues(vector<SnapValue>(numThread, SnapValue(0, 0))) {}

        // Update value 
        void update(int updateValue, int threadId) {
            vector<int> snapshot = scan();

            SnapValue oldSnapValue = snapValues[threadId];
            SnapValue newSnapValue(oldSnapValue.label + 1, updateValue, snapshot);

            snapValues[threadId] = newSnapValue;
        }
};