#include "TwoPhaseLockingRunner.hpp"

using namespace std;

int isArgvInvalid(int value) {
	if (isdigit(value) == 0) {
		cerr << "************** Execution Format ERROR **************\n"
			 << "- The execution format should be like the last line.\n"
			 << "- N is the number of worker threads.\n"
			 << "- R is the number of records.\n"
			 << "- E is the last global execution order which threads will be used to decide termination.\n"
			 << "- N, R, E must be number.\n"
			 << "\n./run N R E\n" << endl;
		return 1;
	}

	return 0;
}

int main(int argc, char** argv) {
	// Check arguments
	if (isArgvInvalid(argv[1][0])) return 1;
	if (isArgvInvalid(argv[2][0])) return 1;
	if (isArgvInvalid(argv[3][0])) return 1;

	int numTread = atoi(argv[1]);
	int numRecord = atoi(argv[2]);
	int numExecution = atoi(argv[3]);

	auto* twoPhaseLockingRunner = new TwoPhaseLockingRunner(numTread, numRecord, numExecution);
	twoPhaseLockingRunner->startReadersWriterTest();
	delete twoPhaseLockingRunner;

	return 0;
}