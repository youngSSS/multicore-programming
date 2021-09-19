#include <iostream>
#include "Joiner.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	Joiner joiner;

	Utils::openLogFile(true);

	// Read join relations
	string line;
	while (getline(cin, line)) {
		// DEBUG :: Print Relations
		Utils::printLog("MAIN-RELATIONS", line + (line == "Done" ? "\n" : ""));

		if (line == "Done") break;
		joiner.addRelation(line.c_str());
	}

	// Preparation phase (not timed)
	// Build histograms, indexes,...

	QueryInfo i;
	while (getline(cin, line)) {
		// DEBUG :: Print Queries
		Utils::printLog(">> MAIN-QUERIES", line);
		Utils::printNewLine();

		if (line == "F") continue; // End of a batch
		i.parseQuery(line);
		string join_result = joiner.join(i);
		cout << join_result;
		Utils::printLog("MAIN-QUERIES_OUTPUT", join_result);
	}

	Utils::closeLogFile();

	return 0;
}
