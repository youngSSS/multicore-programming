#include "Joiner.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <set>
#include <sstream>
#include <vector>
#include "Parser.hpp"
#include "Utils.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
void Joiner::addRelation(const char* fileName)
// Loads a relation from disk
{
	relations.emplace_back(fileName);
}
//---------------------------------------------------------------------------
Relation& Joiner::getRelation(unsigned relationId)
// Loads a relation from disk
{
	if (relationId >= relations.size()) {
		cerr << "Relation with id: " << relationId << " does not exist" << endl;
		throw;
	}
	return relations[relationId];
}
//---------------------------------------------------------------------------
unique_ptr<Operator> Joiner::addScan(set<unsigned>& usedRelations, SelectInfo& info, QueryInfo& query)
// Add scan to query
{
	usedRelations.emplace(info.binding);
	vector<FilterInfo> filters;
	for (auto& f : query.filters) {
		if (f.filterColumn.binding == info.binding) {
			filters.emplace_back(f);
		}
	}

	Utils::printLog("JOINER-ADDSCAN-BINDING", to_string(info.binding));
	for (auto& f : filters)
		Utils::printLog("JOINER-ADDSCAN-FILTER", to_string(f.constant));
	for (auto& f : query.filters) {
		Utils::printLog("JOINER-ADDSCAN-QUERY_FILTER", to_string(f.filterColumn.binding));
	}
	Utils::printLog("", "\n");

	return !filters.empty() ? make_unique<FilterScan>(getRelation(info.relId), filters) : make_unique<Scan>(getRelation(
		info.relId), info.binding);
}
//---------------------------------------------------------------------------
enum QueryGraphProvides {
	Left, Right, Both, None
};
//---------------------------------------------------------------------------
static QueryGraphProvides analyzeInputOfJoin(set<unsigned>& usedRelations, SelectInfo& leftInfo, SelectInfo& rightInfo)
// Analyzes inputs of join
{
	bool usedLeft = usedRelations.count(leftInfo.binding);
	bool usedRight = usedRelations.count(rightInfo.binding);

	if (usedLeft ^ usedRight)
		return usedLeft ? QueryGraphProvides::Left : QueryGraphProvides::Right;
	if (usedLeft && usedRight)
		return QueryGraphProvides::Both;
	return QueryGraphProvides::None;
}
//---------------------------------------------------------------------------
string Joiner::join(QueryInfo& query)
// Executes a join query
{
	//cerr << query.dumpText() << endl;
	set<unsigned> usedRelations;

	for (auto q : query.relationIds)
		Utils::printLog("JOINER-RELATION_ID", to_string(q));
//	for (auto q : query.predicates)
//		Utils::printLog("JOINER-RELATION_ID", q);
//	for (auto q : query.filters)
//		Utils::printLog("JOINER-RELATION_ID", q);
//	for (auto q : query.selections)
//		Utils::printLog("JOINER-RELATION_ID", q);
	Utils::printLog("", "\n");

	// We always start with the first join predicate and append the other joins to it (--> left-deep join trees)
	// You might want to choose a smarter join ordering ...
	auto& firstJoin = query.predicates[0];
	auto left = addScan(usedRelations, firstJoin.left, query);
	auto right = addScan(usedRelations, firstJoin.right, query);
	unique_ptr<Operator> root = make_unique<Join>(move(left), move(right), firstJoin);

	Utils::printLog("JOINER-JOIN-LEFT_REL_ID", to_string(firstJoin.left.relId));
	Utils::printLog("JOINER-JOIN-LEFT_BINDING", to_string(firstJoin.left.binding));
	Utils::printLog("JOINER-JOIN-LEFT_COL_ID", to_string(firstJoin.left.colId));
	Utils::printLog("JOINER-JOIN-RIGHT_REL_ID", to_string(firstJoin.right.relId));
	Utils::printLog("JOINER-JOIN-RIGHT_BINDING", to_string(firstJoin.right.binding));
	Utils::printLog("JOINER-JOIN-RIGHT_COL_ID", to_string(firstJoin.right.colId));
	Utils::printLog("", "\n");

	for (unsigned i = 1; i < query.predicates.size(); ++i) {
		auto& pInfo = query.predicates[i];
		auto& leftInfo = pInfo.left;
		auto& rightInfo = pInfo.right;
		unique_ptr<Operator> left, right;

		Utils::printLog("JOINER-JOIN-LEFT_REL_ID", to_string(pInfo.left.relId));
		Utils::printLog("JOINER-JOIN-LEFT_BINDING", to_string(pInfo.left.binding));
		Utils::printLog("JOINER-JOIN-LEFT_COL_ID", to_string(pInfo.left.colId));
		Utils::printLog("JOINER-JOIN-RIGHT_REL_ID", to_string(pInfo.right.relId));
		Utils::printLog("JOINER-JOIN-RIGHT_BINDING", to_string(pInfo.right.binding));
		Utils::printLog("JOINER-JOIN-RIGHT_COL_ID", to_string(pInfo.right.colId));
		Utils::printLog("", "\n");

		switch (analyzeInputOfJoin(usedRelations, leftInfo, rightInfo)) {
		case QueryGraphProvides::Left:
			Utils::printLog("JOINER-JOIN-QUERY_GRAPH_PROVIDES", "LEFT");

			left = move(root);
			right = addScan(usedRelations, rightInfo, query);
			root = make_unique<Join>(move(left), move(right), pInfo);
			break;
		case QueryGraphProvides::Right:
			Utils::printLog("JOINER-JOIN-QUERY_GRAPH_PROVIDES", "RIGHT");

			left = addScan(usedRelations, leftInfo, query);
			right = move(root);
			root = make_unique<Join>(move(left), move(right), pInfo);
			break;
		case QueryGraphProvides::Both:
			Utils::printLog("JOINER-JOIN-QUERY_GRAPH_PROVIDES", "BOTH");

			// All relations of this join are already used somewhere else in the query.
			// Thus, we have either a cycle in our join graph or more than one join predicate per join.
			root = make_unique<SelfJoin>(move(root), pInfo);
			break;
		case QueryGraphProvides::None:
			Utils::printLog("JOINER-JOIN-QUERY_GRAPH_PROVIDES", "NONE");

			// Process this predicate later when we can connect it to the other joins
			// We never have cross products
			query.predicates.push_back(pInfo);
			break;
		};
	}

	Checksum checkSum(move(root), query.selections);
	checkSum.run();

	stringstream out;
	auto& results = checkSum.checkSums;
	for (unsigned i = 0; i < results.size(); ++i) {
		out << (checkSum.resultSize == 0 ? "NULL" : to_string(results[i]));
		if (i < results.size() - 1)
			out << " ";
	}
	out << "\n";
	return out.str();
}
//---------------------------------------------------------------------------
