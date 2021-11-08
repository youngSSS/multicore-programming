#ifndef READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_
#define READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_

#include <vector>

using namespace std;

class Database {
 private:
	vector<int64_t> database;

 public:
	explicit Database(int numRecord) {
		database.resize(numRecord, 100);
	}

	int64_t readRecord(int rid) {
		return database[rid];
	}

	void updateRecord(int rid, int64_t value) {
		database[rid] = value;
	}
};

#endif //READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_
