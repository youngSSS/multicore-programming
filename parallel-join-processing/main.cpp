#include <iostream>
#include "Joiner.hpp"
#include "Parser.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	Joiner joiner;

	// Read join relations
	string line;
	while (getline(cin, line)) {
		if (line == "Done") break;
		joiner.addRelation(line.c_str());
	}

	// Preparation phase (not timed)
	// Build histograms, indexes,...

	QueryInfo queryInfo;
	while (getline(cin, line)) {
		if (line == "F") {
			// Print the result of join set
			cout << joiner.getJoinResults();
			continue;
		};

		// Start join thread with single join request
		joiner.startJoinThread(line);
	}

	return 0;
}
