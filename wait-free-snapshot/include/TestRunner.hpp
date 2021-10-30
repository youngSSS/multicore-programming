#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "WaitFreeSnapshot.hpp"

#define EXECUTION_TIME 10

using namespace std;

class TestRunner {
    private:
        WaitFreeSnapshot* wfs;
        int numThread;
        vector<int> updateCounter;

        boost::asio::io_service ioService;
        boost::asio::io_service::work work;
        boost::thread_group threadPool;

        void updateTest(WaitFreeSnapshot* wfs, int tid, int* exitFlag) {
            while (!*exitFlag) {
                srand((unsigned int)time(nullptr));
                int randomValue = rand() % 10000;

                wfs->update(randomValue, tid);

                updateCounter[tid] += 1;
            }
        }

    public:
        TestRunner(int numThread) : wfs(new WaitFreeSnapshot(numThread)), numThread(numThread), work(ioService) {
            for (int i = 0; i < numThread; i++) 
                threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
        }

        ~TestRunner() {
            delete wfs;
        }

        // Update each thread's local value
        void startUpdateTest() {
            WaitFreeSnapshot* wfs = new WaitFreeSnapshot(numThread);
            int* exitFlag = new int(0);
            updateCounter.resize(numThread, 0);

            for (int tid = 0; tid < numThread; tid++) 
                ioService.post(boost::bind(&TestRunner::updateTest, this, wfs, tid, exitFlag));

            // Each thread works for 1 minute
            sleep(EXECUTION_TIME);
            *exitFlag = 1;
            delete exitFlag;

            sleep(1);

            int numUpdate = 0;
            for (int tid = 0; tid < numThread; tid++)
                numUpdate += updateCounter[tid];
            
            printf("The number of total updates: %d\n", numUpdate);
        }
};