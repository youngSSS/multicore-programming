#pragma once
#include <fstream>
#include <string>
#include "Relation.hpp"
//---------------------------------------------------------------------------
class Utils {
 public:
	/// Create a dummy relation
	static Relation createRelation(uint64_t size, uint64_t numColumns);

	/// Store a relation in all formats
	static void storeRelation(std::ofstream& out, Relation& r, unsigned i);
};
//---------------------------------------------------------------------------
