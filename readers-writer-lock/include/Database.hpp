#ifndef READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_
#define READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_

#include <vector>

using namespace std;

class Database {
 private:
	vector<int64_t> database;

 public:
	Database(int numRecord) {
		database.resize(numRecord, 100);
	}

};

#endif //READERS_WRITER_LOCK_INCLUDE_DATABASE_HPP_
