#include "Joiner.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <set>
#include <sstream>
#include <vector>
#include "Parser.hpp"

#include <thread>

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
void Joiner::join(QueryInfo& query, int threadIdx)
// Executes a join query
{
	// cerr << query.dumpText() << endl;
	set<unsigned> usedRelations;

	// We always start with the first join predicate and append the other joins to it (--> left-deep join trees)
	// You might want to choose a smarter join ordering ...
	auto& firstJoin = query.predicates[0];
	auto left = addScan(usedRelations, firstJoin.left, query);
	auto right = addScan(usedRelations, firstJoin.right, query);
	unique_ptr<Operator> root = make_unique<Join>(move(left), move(right), firstJoin);

//	Utils::printLog("JOINER-JOIN-LOOP-P_INFO", "0th pInfo");
//	Utils::printLog("JOINER-JOIN-LOOP-LEFT", left + "." + leftInfo.colId);
//	Utils::printLog("JOINER-JOIN-LOOP-RIGHT", rightInfo.binding + "." + rightInfo.colId);
//	Utils::printNewLine();

	// Predicate 연산만 수행한다. (1 <= i < predicate.size()에 대해서 join을 수행하는 이유)
	// Filter는 Predicate 연산 수행시, Filter 조건에 맞는 Relation과 Column이 있는 경우에만 수행된다.
	// Predicate 연산 수행 전 Filter를 먼저 적용하여 탐색 범위를 줄이고 Predicate 연산을 수핸한다.
	for (unsigned i = 1; i < query.predicates.size(); ++i) {
		auto& pInfo = query.predicates[i];
		auto& leftInfo = pInfo.left;
		auto& rightInfo = pInfo.right;
		unique_ptr<Operator> left, right;

		switch (analyzeInputOfJoin(usedRelations, leftInfo, rightInfo)) {
		case QueryGraphProvides::Left:
			left = move(root);
			right = addScan(usedRelations, rightInfo, query);
			root = make_unique<Join>(move(left), move(right), pInfo);
			break;
		case QueryGraphProvides::Right:
			left = addScan(usedRelations, leftInfo, query);
			right = move(root);
			root = make_unique<Join>(move(left), move(right), pInfo);
			break;
		case QueryGraphProvides::Both:
			// All relations of this join are already used somewhere else in the query.
			// Thus, we have either a cycle in our join graph or more than one join predicate per join.
			root = make_unique<SelfJoin>(move(root), pInfo);
			break;
		case QueryGraphProvides::None:
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

	joinResults[threadIdx] = out.str();

	// Decrease Thread Index
	unique_lock<mutex> lk(_mutex);
	cond.wait(lk);
	threadIndex -= 1;
	_mutex.unlock();
}
//---------------------------------------------------------------------------
void Joiner::startJoinThread(string line) {
	QueryInfo queryInfo;
	queryInfo.parseQuery(line);

	threadIndex += 1;
	joinResults.emplace_back();

	// Boost bind
	// https://www.boost.org/doc/libs/1_75_0/libs/bind/doc/html/bind.html
	ioService.post(boost::bind(&Joiner::join, this, queryInfo, threadIndex));
}

string Joiner::getJoinResults() {
	// Wait until all thread finish work
	while (threadIndex != -1)
		cond.notify_one();

	string retVal;
	for (string& result : joinResults) retVal += result;

	// Cleanup
	joinResults.clear();

	return retVal;
}