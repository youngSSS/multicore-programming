#include "Joiner.hpp"
#include <iostream>
#include <string>
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
//	for (auto col : relations[relations.size() - 1].columns)
//		Utils::printLog("JOINER-ADD_RELATION-COLUMN", to_string(*col));
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

	// Predicate를 수행하기 전에 현재 Column에 대해 적용할 수 있는 필터를 먼저 적용
	vector<FilterInfo> filters;
	for (auto& f : query.filters) {
		if (f.filterColumn.binding == info.binding) {
			filters.emplace_back(f);
		}
	}

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
	/* --------------- DEBUG :: Print Information of Query --------------- */
	// Relation Id
	string relationIdList;
	for (auto q : query.relationIds) relationIdList += to_string(q) + ", ";
	Utils::printLog("JOINER-RELATION_ID_LIST",
		relationIdList.substr(0, relationIdList.size() - 2) + "\n");

	// Predicate
	for (PredicateInfo predicateInfo : query.predicates) {
		SelectInfo leftInfo = predicateInfo.left;
		string leftRelationId = to_string(leftInfo.binding);
		string leftColumnId = to_string(leftInfo.colId);

		SelectInfo rightInfo = predicateInfo.right;
		string rightRelationId = to_string(rightInfo.binding);
		string rightColumnId = to_string(rightInfo.colId);

		Utils::printLog("JOINER-QUERY-PREDICATE-[LEFT, RIGHT]",
			"[" + leftRelationId + "." + leftColumnId + ", " + rightRelationId + "." + rightColumnId + "]" +
				" (r_id = " + leftRelationId + ", col_id = " + leftColumnId + " and "
																			  "r_id = " + rightRelationId
				+ ", col_id = " + rightColumnId + ")");
	}
	Utils::printNewLine();

	// Filter
	for (FilterInfo filterInfo : query.filters) {
		SelectInfo filterColumn = filterInfo.filterColumn;
		uint64_t constant = filterInfo.constant;
		FilterInfo::Comparison comparison = filterInfo.comparison;

		string comp;
		if (comparison == 62) comp = ">";
		else if (comparison == 60) comp = "<";
		else comp = "=";

		Utils::printLog("JOINER-QUERY-FILTER",
			to_string(filterColumn.binding) + "." + to_string(filterColumn.colId) + " " + comp + " "
				+ to_string(constant) +
				" (r_id = " + to_string(filterColumn.binding) + ", col_id = " + to_string(filterColumn.colId) +
				", comparison = " + comp + ", constant = " + to_string(constant) + ")");
	}
	Utils::printNewLine();

	// Select
	string selectList;
	for (SelectInfo selectInfo : query.selections) {
		string relationId = to_string(selectInfo.binding);
		string columnId = to_string(selectInfo.colId);

		selectList += relationId + "." + columnId + ", ";
	}
	Utils::printLog("JOINER-QUERY-SELECT", selectList.substr(0, selectList.size() - 2));
	Utils::printNewLine();
	/* ------------------------------------------------------------------- */

	// cerr << query.dumpText() << endl;
	set<unsigned> usedRelations;

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

	// Predicate 연산만 수행한다. (1 <= i < predicate.size()에 대해서 join을 수행하는 이유)
	// Filter는 Predicate 연산 수행시, Filter 조건에 맞는 Relation과 Column이 있는 경우에만 수행된다.
	// Predicate 연산 수행 전 Filter를 먼저 적용하여 탐색 범위를 줄이고 Predicate 연산을 수핸한다.
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
