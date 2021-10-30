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
        
        vector<int> scan();

    public:
        // Constructor
        WaitFreeSnapshot(int numThread) : snapValues(vector<SnapValue>(numThread, SnapValue(0, 0))) {}

        // Update value 
        void update(int updateValue, int threadId);
};