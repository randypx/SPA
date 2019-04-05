#pragma once
#include "Hasher.h"
#include "PKB.h"

class PKB;

class RunTimeDesignExtractor {

public:
	unordered_set<int> extractNextStar(int stmt);
	unordered_set<int> extractPreviousStar(int stmt);
	bool extractNextStarPair(int start, int end);
	unordered_set<pair<int, int>, intPairhash> getNextStarPairs();
	void DFSRecursiveNext(int procedure, unordered_set<int> &visitedStmts, unordered_set<int>& stmtsAfter);
	void DFSRecursivePrevious(int procedure, unordered_set<int> &visitedStmts, unordered_set<int> &stmtsBefore);

	bool isAffect(int stmt, int stmt1);

	bool DFSRecursiveCheckAffectsPair(int start, int target, int current, std::unordered_set<int>& cfgPath, bool isStart);

	bool isStatementAffectingAnother(int stmt);

	bool DFSRecursiveCheckAffecting(int start, int current, std::unordered_set<int>& cfgPath, bool isStart);

	bool isStatementAffectedByAnother(int stmt);

	bool DFSRecursiveCheckAffectedBy(int end, int current, std::unordered_set<int>& cfgPath, bool isStart);

	bool hasAffectsRelation();

	vector<int> getStatementsAffectedByAnother(int stm);

	vector<int> getStatementsAffectingAnother(int stm);

	void DFSRecursiveGetAffectingList(int start, int current, std::unordered_set<int>& cfgPath, bool isStart, std::vector<int>& affectedList);

	bool isAffectPossible(int stmt, int stmt1);

	bool isLastModifiedBroken(int current, int start);

	bool contains(std::unordered_set<std::string>& modifiedInStmt, std::unordered_set<std::string>& usedInStmt1);

private:
	PKB* pkb;
};