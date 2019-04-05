#include"RunTimeDesignExtractor.h"

unordered_set<int> RunTimeDesignExtractor::extractNextStar(int stmt) {
	unordered_set<int>visitedStmts;
	unordered_set<int>stmtsAfter;
	DFSRecursiveNext(stmt, visitedStmts, stmtsAfter);
	return stmtsAfter;
}

unordered_set<int> RunTimeDesignExtractor::extractPreviousStar(int stmt) {
	unordered_set<int>visitedStmts;
	unordered_set<int>stmtsBefore;
	DFSRecursivePrevious(stmt, visitedStmts,stmtsBefore);
	return stmtsBefore;
}

bool RunTimeDesignExtractor::extractNextStarPair(int start, int end) {
	unordered_set<int>visitedStmts;
	unordered_set<int>stmtsAfter;
	DFSRecursiveNext(start, visitedStmts,stmtsAfter);
	if (stmtsAfter.find(end) == stmtsAfter.end()) {
		return false;
	}
	return true;
}

unordered_set<pair<int, int>, intPairhash> RunTimeDesignExtractor::getNextStarPairs() {
	int stmtNum = pkb->getTotalStmNo();
	unordered_set<pair<int, int>, intPairhash> allNextStarPairs;
	//for all Statement in statemnet list generate nextStar
	for (int i = 1; i <= stmtNum; i++) {
		unordered_set<int>visitedStmts;
		unordered_set<int>stmtsAfter;
		DFSRecursiveNext(i, visitedStmts,stmtsAfter);
		for (int statments : stmtsAfter) {
			allNextStarPairs.emplace(i, statments);
		}
	}
	return allNextStarPairs;
}

void RunTimeDesignExtractor::DFSRecursiveNext(int statments, unordered_set<int> &visitedStmts, unordered_set<int> &stmtsAfter) {
	//Marks statment as visited
	visitedStmts.insert(statments);
	//Get neighbouring statments.
	unordered_set<int> adjacentStmts = pkb->getNext(statments);

	//For each neighbouring statments
	for (int stmts : adjacentStmts) {
		
		std::unordered_set<int>::const_iterator visited = stmtsAfter.find(stmts);
		if (visited == stmtsAfter.end()) {
			stmtsAfter.insert(stmts);
		}
		//Check if neighbouring statments has been visited
		std::unordered_set<int>::const_iterator exist = visitedStmts.find(stmts);

		//If neighbouring statments has not been visited, visit it.
		if (exist == visitedStmts.end()) {
			stmtsAfter.insert(stmts);
			DFSRecursiveNext(stmts, visitedStmts,stmtsAfter);
		}
	}
}

void RunTimeDesignExtractor::DFSRecursivePrevious(int statments, unordered_set<int> &visitedStmts, unordered_set<int> &stmtsBefore) {
	//Marks statment as visited
	visitedStmts.insert(statments);
	//Get neighbouring statments.
	unordered_set<int> adjacentStmts = pkb->getPrev(statments);

	//For each neighbouring statments
	for (int stmts : adjacentStmts) {
		std::unordered_set<int>::const_iterator visited = stmtsBefore.find(stmts);

		if (visited == stmtsBefore.end()) {
			stmtsBefore.insert(stmts);
		}
		//Check if neighbouring statments has been visited
		std::unordered_set<int>::const_iterator exist = visitedStmts.find(stmts);

		//If neighbouring statments has not been visited, visit it.
		if (exist == visitedStmts.end()) {
			DFSRecursivePrevious(stmts, visitedStmts,stmtsBefore);
		}
	}
}







bool RunTimeDesignExtractor::isAffect(int stmt, int stmt1) {
	//Check if stmt is able to affect stmt1.
	bool isPossible = isAffectPossible(stmt, stmt1);

	if (isPossible) {
		//If possible, we do a CFG check.
		std::unordered_set<int> path;
		return DFSRecursiveCheckAffectsPair(stmt, stmt1, stmt, path, true);
	}
	else {
		return false;
	}
}

bool RunTimeDesignExtractor::DFSRecursiveCheckAffectsPair(int start, int target, int current, std::unordered_set<int> &cfgPath, bool isStart) {

	//If is not starting node we will not check if it affects or breaks affects
	//Subsequently, if we visit the starting node again we will go into this clause.
	if (!isStart) {
		// We have reached target, and we know it is possible to Affect
		if (current == target) {
			return true;
		}
		//We have encountered something in this path that breaks Affects. Therefore we return false.
		else if (isLastModifiedBroken(current, start)) {
			return false;
		}
	}

	//Add stmt to CFGPath if is start of DFS
	//Subsequently it will always add to cfgPath
	if (isStart) {
		cfgPath.insert(current);
	}

	//Get neighbouring next statements
	std::unordered_set<int> nextStatements = pkb->getNext(current);

	//For each neighbouring procedure
	for (int nextStmt : nextStatements) {

		//Check if next Statemnt is inside the CFGPath.
		std::unordered_set<int>::const_iterator currPath = cfgPath.find(nextStmt);

		//If next statement has not been visited, visit it.
		if (currPath == cfgPath.end()) {
			bool result = DFSRecursiveCheckAffectsPair(start, target, nextStmt, cfgPath, false);
			if (result) {
				return true;
			}
		}
	}
	return false;
}

bool RunTimeDesignExtractor::isStatementAffectingAnother(int stmt) {
	if (pkb->getStmType(stmt) != stmType::assign) {
		return false;
	}
	std::unordered_set<int> cfgPath;
	return DFSRecursiveCheckAffecting(stmt, stmt, cfgPath, false);
}

bool RunTimeDesignExtractor::DFSRecursiveCheckAffecting(int start, int current, std::unordered_set<int> &cfgPath, bool isStart) {
	//Add stmt to CFGPath if is start of DFS
	//Subsequently it will always add to cfgPath
	if (isStart) {
		cfgPath.insert(current);
	}

	//If is not starting node we will not check if it affects or breaks affects
	//Subsequently, if we visit the starting node again we will go into this clause.
	if (!isStart) {
		if (isAffectPossible(start, current)) {
			return true;
		}
		//We have encountered something in this path that breaks Affects. Therefore we return false.
		else if (isLastModifiedBroken(current, start)) {
			return false;
		}
	}

	//Get neighbouring next statements
	std::unordered_set<int> nextStatements = pkb->getNext(current);

	//For each neighbouring procedure
	for (int nextStmt : nextStatements) {

		//Check if next Statemnt is inside the CFGPath.
		std::unordered_set<int>::const_iterator currPath = cfgPath.find(nextStmt);

		//If next statement has not been visited, visit it.
		if (currPath == cfgPath.end()) {
			bool result = DFSRecursiveCheckAffecting(start, nextStmt, cfgPath, false);
			if (result) {
				return true;
			}
		}
	}
	return false;
}

bool RunTimeDesignExtractor::isStatementAffectedByAnother(int stmt) {
	if (pkb->getStmType(stmt) != stmType::assign) {
		return false;
	}
	std::unordered_set<int> cfgPath;
	return DFSRecursiveCheckAffectedBy(stmt, stmt, cfgPath, false);
}

bool RunTimeDesignExtractor::DFSRecursiveCheckAffectedBy(int end, int current, std::unordered_set<int> &cfgPath, bool isStart) {
	//Add stmt to CFGPath if is start of DFS
	//Subsequently it will always add to cfgPath
	if (isStart) {
		cfgPath.insert(current);
	}

	//If is not starting node we will not check if it affects or breaks affects
	//Subsequently, if we visit the starting node again we will go into this clause.
	if (!isStart) {
		if (isAffectPossible(current, end)) {
			return true;
		}
	}

	//Get neighbouring prev statements
	std::unordered_set<int> prevStatements = pkb->getPrev(current);

	//For each neighbouring prev statements
	for (int prevStmt : prevStatements) {

		//Check if prev Statemnt is inside the CFGPath.
		std::unordered_set<int>::const_iterator currPath = cfgPath.find(prevStmt);

		//If next statement has not been visited, visit it.
		if (currPath == cfgPath.end()) {
			bool result = DFSRecursiveCheckAffecting(end, prevStmt, cfgPath, false);
			if (result) {
				return true;
			}
		}
	}
	return false;
}

bool RunTimeDesignExtractor::hasAffectsRelation() {
	int stmNumber = pkb->getTotalStmNo();

	for (int i = 0; i < stmNumber; i++) {
		if (isStatementAffectingAnother(i)) {
			return true;
		}
	}
	return false;
}

vector<int> RunTimeDesignExtractor::getStatementsAffectedByAnother(int stmt) {

}

vector<int> RunTimeDesignExtractor::getStatementsAffectingAnother(int stmt) {
	if (pkb->getStmType(stmt) != stmType::assign) {
		return vector<int>();
	}
	std::unordered_set<int> cfgPath;
	std::vector<int> affectedList;
	DFSRecursiveGetAffectingList(stmt, stmt, cfgPath, true, affectedList);
}

void RunTimeDesignExtractor::DFSRecursiveGetAffectingList(int start, int current, std::unordered_set<int> &cfgPath, bool isStart, std::vector<int> &affectedList) {
	//Add stmt to CFGPath if is not start of DFS
	//Subsequently it will always add to cfgPath
	if (!isStart) {
		cfgPath.insert(current);
	}

	//If is not starting node we will not check if it affects or breaks affects
	//Subsequently, if we visit the starting node again we will go into this clause.
	if (!isStart) {
		if (isAffectPossible(start, current)) {
			affectedList.push_back(current);
		}
		//We have encountered something in this path that breaks Affects. Therefore we return false.
		else if (isLastModifiedBroken(current, start)) {
			return;
		}
	}

	//Get neighbouring next statements
	std::unordered_set<int> nextStatements = pkb->getNext(current);

	//For each neighbouring procedure
	for (int nextStmt : nextStatements) {

		//Check if next Statemnt is inside the CFGPath.
		std::unordered_set<int>::const_iterator currPath = cfgPath.find(nextStmt);

		//If next statement has not been visited, visit it.
		if (currPath == cfgPath.end()) {
			DFSRecursiveCheckAffecting(start, nextStmt, cfgPath, false);
		}
	}
	return;
}

bool RunTimeDesignExtractor::isAffectPossible(int stmt, int stmt1) {
	//Check if stmt and stmt1 are Assign Statements
	if (pkb->getStmType(stmt) != stmType::assign) {
		return false;
	}
	else if (pkb->getStmType(stmt1) != stmType::assign) {
		return false;
	}

	//Check if stmt modifies something that stmt1 uses
	std::unordered_set<std::string> modifiedInStmt = pkb->getVarModifiedByStm(stmt);
	std::unordered_set<std::string> usedInStmt1 = pkb->getVarModifiedByStm(stmt1);
	bool isPossible = contains(modifiedInStmt, usedInStmt1);

	return isPossible;
}

bool RunTimeDesignExtractor::isLastModifiedBroken(int current, int start)
{
	stmType type = pkb->getStmType(current);
	if (type == stmType::assign || type == stmType::read || type == stmType::call) {
		std::unordered_set<std::string> modifiedVar = pkb->getVarModifiedByStm(start);
		std::unordered_set<std::string> modifiedInCurrent = pkb->getVarModifiedByStm(current);
		bool found = contains(modifiedVar, modifiedInCurrent);
		return found;
	}
	return false;
}

//Check if modified var can be found in var used in statement.
bool RunTimeDesignExtractor::contains(std::unordered_set<std::string> &modifiedInStmt, std::unordered_set<std::string> &usedInStmt1)
{
	for (std::string modified : modifiedInStmt) {
		for (std::string used : usedInStmt1) {
			if (modified == used) {
				return true;
			}
		}
	}
	return false;
}