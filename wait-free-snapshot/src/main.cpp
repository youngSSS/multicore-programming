#include <iostream>
#include "WaitFreeSnapshot.hpp"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "************** Execution Format ERROR **************\n"
             << "- The execution format should be like the last line.\n"
             << "- N is the number of worker threads.\n"
             << "\n./run N\n" << endl;
        return 0;
    }

    if (isdigit(argv[1][0]) == 0) {
        cerr << "************** Execution Format ERROR **************\n"
             << "- The execution format should be like the last line.\n"
             << "- N should be the \"NUMBER\" of worker threads.\n"
             << "\n./run N\n" << endl;
        return 0;
    }

    int numThread = atoi(argv[1]);
    
    return 0;
}