#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <algorithm>

#include "PKB.h"
#include "QueryEvaluator.h"	
#include "LexicalToken.h"
#include "ExpressionUtil.h"
#include "ContainerUtil.h"

/*
Projects result of the query 
*/
std::unordered_set<std::string> QueryEvaluator::projectResult(
	std::unordered_map<std::string, std::string> declarations,
	std::vector<std::string> selectedVar,
	std::vector<std::pair<std::string, std::pair<std::string, std::string>>> suchThatCondition,
	std::vector<std::pair<std::string, std::pair<std::string, std::string>>> patternCondition,
	std::vector<std::pair<std::string, std::string>> withCondition) {
	std::pair<std::string, std::unordered_map<std::string, std::vector<std::string>>> resultPair = evaluateTable(
		declarations, suchThatCondition, patternCondition, withCondition);
	std::string status = resultPair.first;
	std::unordered_map<std::string, std::vector<std::string>> resultTable = resultPair.second;
	std::unordered_set<std::string> resultSet;
	if (selectedVar[0] == "BOOLEAN") {
		resultSet.insert(status);
		return resultSet;
	}
	else {
		if (status == "FALSE" || (resultTable.size() != 0 && resultTable.begin()->second.size() == 0)) {
			return resultSet;
		}
		else {
			std::unordered_map<std::string, std::vector<std::string>> projectTable;
			std::vector<std::string> notInResult;
			for (std::vector<std::string>::size_type i = 0; i != selectedVar.size(); i++) {
				if (resultTable.count(selectedVar[i]) == 0) {
					notInResult.push_back(selectedVar[i]);
				}
				else {
					projectTable.insert({ selectedVar[i], resultTable[selectedVar[i]] });
				}
			}
			for (std::vector<std::string>::size_type i = 0; i != notInResult.size(); i++) {
				projectTable = ContainerUtil::product(projectTable, getStmts(declarations, selectedVar[i]));
			}
			int projectedSize = projectTable.begin()->second.size();
			for (std::vector<std::string>::size_type i = 0; i != projectedSize; i++) {
				std::string tuple;
				for (auto columnIt = projectTable.begin(); columnIt != projectTable.end(); ++columnIt) {
					if (tuple.size() == 0) {
						tuple = columnIt->second[i];
					} 
					else {
						tuple = tuple + " " + columnIt->second[i];
					}
				}
				resultSet.insert(tuple);
			}
		}

		return resultSet;
	}
}

/*
Merge all the results of each clause
*/
std::pair<std::string, std::unordered_map<std::string, std::vector<std::string>>> QueryEvaluator::evaluateTable(
	std::unordered_map<std::string, std::string> declarations,
	std::vector<std::pair<std::string, std::pair<std::string, std::string>>> suchThatCondition,
	std::vector<std::pair<std::string, std::pair<std::string, std::string>>> patternCondition,
	std::vector<std::pair<std::string, std::string>> withCondition) {
	std::unordered_map<std::string, std::vector<std::string>> resultTable;
	std::string status = "TRUE";
	if (suchThatCondition.size() != 0) {
		for (std::vector<std::pair<std::string, std::pair<std::string, std::string>>>::size_type i = 0;
			i != suchThatCondition.size();
			i++) {
			std::string relation = suchThatCondition[i].first;
			std::string firstArgument = suchThatCondition[i].second.first;
			std::string secondArgument = suchThatCondition[i].second.second;
			std::string trivialness = isSuchThatTrivial(relation, firstArgument, secondArgument);
			if (trivialness == "false") {
				status = "FALSE";
				break;
			}
			if (trivialness == "not trivial") {
				std::unordered_map<std::string, std::vector<std::string>> newTable = evaluateSuchThat(
					declarations, relation, firstArgument, secondArgument);
				resultTable = ContainerUtil::product(resultTable, newTable);
				if (resultTable.begin()->second.size() == 0) {
					status == "FALSE";
					break;
				}
			}
		}
	}
	if (patternCondition.size() != 0) {
		for (std::vector<std::pair<std::string, std::pair<std::string, std::string>>>::size_type i = 0;
			i != patternCondition.size();
			i++) {
			std::unordered_map<std::string, std::vector<std::string>> newTable = evaluatePatternCondition(
				declarations, patternCondition[i]);
			resultTable = ContainerUtil::product(resultTable, newTable);
			if (resultTable.begin()->second.size() == 0) {
				status == "FALSE";
				break;
			}
		}
	}
	std::pair<std::string, std::unordered_map<std::string, std::vector<std::string>>> resultPair(status,
		resultTable);
	return resultPair;
}

/*
The function evaluates
pattern clauses
*/
std::unordered_map<std::string, std::vector<std::string>> QueryEvaluator::evaluatePatternCondition(
	std::unordered_map<std::string, std::string> declarations,
	std::pair<std::string, std::pair<std::string, std::string>> pattern) {
	std::string patternSynonym = pattern.first;
	std::string patternType = declarations[patternSynonym];
	std::string leftArgument = pattern.second.first;
	std::string rightArgument = pattern.second.second;

	if (patternType == "assign") {
		if (rightArgument == "_") {
			if (leftArgument == "_") {
				return getStmts(declarations, patternSynonym);
			}
			else if (isQuoted(leftArgument)) {
				return ContainerUtil::to_mapvec(patternSynonym,
					PKB().findPattern(trimFrontEnd(leftArgument), "", false));
			}
			return ContainerUtil::to_mapvec(patternSynonym, leftArgument, PKB().findPatternPairs("", false));
		}
		else if (isQuoted(rightArgument)) {
			rightArgument = trimFrontEnd(rightArgument);
			rightArgument = ExpressionUtil::convertInfixToPrefix(rightArgument);
			if (leftArgument == "_") {
				return ContainerUtil::to_mapvec(patternSynonym, PKB().findPattern(rightArgument, true));
			}
			else if (isQuoted(leftArgument)) {
				return ContainerUtil::to_mapvec(patternSynonym,
					PKB().findPattern(trimFrontEnd(leftArgument), rightArgument, true));
			}
			return ContainerUtil::to_mapvec(patternSynonym, leftArgument, PKB().findPatternPairs(rightArgument, true));
		}
		else {
			rightArgument = trimFrontEnd(trimFrontEnd(rightArgument));
			rightArgument = ExpressionUtil::convertInfixToPrefix(rightArgument);
			if (leftArgument == "_") {
				return ContainerUtil::to_mapvec(patternSynonym, PKB().findPattern(rightArgument, false));
			}
			else if (isQuoted(leftArgument)) {
				return ContainerUtil::to_mapvec(patternSynonym,
					PKB().findPattern(trimFrontEnd(leftArgument), rightArgument, false));
			}
			return ContainerUtil::to_mapvec(patternSynonym, leftArgument, PKB().findPatternPairs(rightArgument, false));
		}
	}
}

/*
THe function evaluates the such that conditions 
which gives boolean answer
*/
std::string QueryEvaluator::isSuchThatTrivial(std::string relation, std::string firstArgument, 
	std::string secondArgument) {
	bool result;
	if (relation == "Follows") {
		if (firstArgument == "_") {
			if (secondArgument == "_") {
				return truthValue(PKB().hasFollowRelation());
			}
			else if (isInteger(secondArgument)) {
				result = PKB().getStmFollowedBy(stoi(secondArgument)) > 0;
				return truthValue(result);
			}
		}
		else if (isInteger(firstArgument)) {
			if (secondArgument == "_") {
				result = PKB().getFollower(stoi(firstArgument)) > 0;
				return truthValue(result);
 			}
			else if (isInteger(secondArgument)) {
				result = PKB().getFollower(stoi(firstArgument)) == stoi(secondArgument);
				return truthValue(result);
			};
		}
		else if (firstArgument == secondArgument) {
			return "false";
		}
	}
	else if (relation == "Follows*") {
		if (firstArgument == "_") {
			if (secondArgument == "_") {
				return truthValue(PKB().hasFollowRelation());
			}
			else if (isInteger(secondArgument)) {
				result = PKB().getStmFollowedBy(stoi(secondArgument)) > 0;
				return truthValue(result);
			}
		}
		else if (isInteger(firstArgument)) {
			if (secondArgument == "_") {
				result = PKB().getFollower(stoi(firstArgument)) > 0;
				return truthValue(result);
			}
			else if (isInteger(secondArgument)) {
				result = PKB().hasFollowStarPair(stoi(firstArgument), stoi(secondArgument));
				return truthValue(result);
			}
		}
		else if (firstArgument == secondArgument) {
			return "false";
		}
	}
	else if (relation == "Parent") {
		if (firstArgument == "_") {
			if (secondArgument == "_") {
				return truthValue(PKB().hasParentRelation());
			}
			else if (isInteger(secondArgument)) {
				result = PKB().isChild(stoi(secondArgument)) > 0;
				return truthValue(result);
			}
		}
		else if (isInteger(firstArgument)) {
			if (secondArgument == "_") {
				result = PKB().isParent(stoi(firstArgument)) > 0;
				return truthValue(result);
			}
			else if (isInteger(secondArgument)) {
				result = PKB().getParent(stoi(secondArgument)) == stoi(firstArgument);
				return truthValue(result);
			}
		}
		else if (firstArgument == secondArgument) {
			return "false";
		}
	}
	else if (relation == "Parent*") {
		if (firstArgument == "_") {
			if (secondArgument == "_") {
				return truthValue(PKB().hasParentRelation());
			}
			else if (isInteger(secondArgument)) {
				result = PKB().isChild(stoi(secondArgument)) > 0;
				return truthValue(result);
			}
		}
		else if (isInteger(firstArgument)) {
			if (secondArgument == "_") {
				result = PKB().isParent(stoi(firstArgument)) > 0;
				return truthValue(result);
			}
			else if (isInteger(secondArgument)) {
				result = PKB().hasAncDescPair(stoi(firstArgument), stoi(secondArgument));
				return truthValue(result);
			}
		}
		else if (firstArgument == secondArgument) {
			return "false";
		}
	}
	else if (relation == "Uses") {
		if (isInteger(firstArgument)) {
			if (isQuoted(secondArgument)) {
				result = PKB().isStmUsing(stoi(firstArgument), trimFrontEnd(secondArgument));
				return truthValue(result);
			}
			else if (secondArgument == "_") {
				result = PKB().getVarUsedByStm(stoi(firstArgument)).size() > 0;
				return truthValue(result);
			}
		}
		else if (isQuoted(firstArgument)) {
			if (isQuoted(secondArgument)) {
				result = PKB().isProcUsing(trimFrontEnd(firstArgument), trimFrontEnd(secondArgument));
				return truthValue(result);
			}
			else if (secondArgument == "_") {
				result = PKB().getVarUsedByProc(trimFrontEnd(firstArgument)).size() > 0;
				return truthValue(result);
			}
		}
	}
	else if (relation == "Modifies") {
		if (isInteger(firstArgument)) {
			if (isQuoted(secondArgument)) {
				result = PKB().isStmModifying(stoi(firstArgument), trimFrontEnd(secondArgument));
				return truthValue(result);
			}
			else if (secondArgument == "_") {
				result = PKB().getVarModifiedByStm(stoi(firstArgument)).size() > 0;
				return truthValue(result);
			}
		}
		else if (isQuoted(firstArgument)) {
			if (isQuoted(secondArgument)) {
				result = PKB().isProcModifying(trimFrontEnd(firstArgument), trimFrontEnd(secondArgument));
				return truthValue(result);
			}
			else if (secondArgument == "_") {
				result = PKB().getVarModifiedByProc(trimFrontEnd(firstArgument)).size() > 0;
				return truthValue(result);
			}
		}
	}

	return "not trivial";
}

/*
The function evaluates the non-trivial 
Such That Conditions.
*/
std::unordered_map<std::string, std::vector<std::string>> QueryEvaluator::evaluateSuchThat(
	std::unordered_map<std::string, std::string> declarations,
	std::string relation, std::string firstArgument, std::string secondArgument) {
	std::unordered_map<std::string, std::vector<std::string>> tableResult;
	if (relation == "Follows") {
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAllFollowed());
			}
			else if (isInteger(secondArgument)) {
				int result = PKB().getStmFollowedBy(stoi(secondArgument));
				if (result > 0) {
					tableResult = ContainerUtil::to_mapvec(firstArgument, result);
				}
				else {
					std::vector<std::string> emptyVec;
					tableResult.insert({ firstArgument, emptyVec });
				}
			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getFollowPairs());
			}
		}
		if (isSynonym(secondArgument)) {
			if (firstArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getAllFollowers());
			}
			else if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getFollower(
					stoi(firstArgument)));
			}
		}
	}
	else if (relation == "Follows*") {
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAllFollowed());
			}
			else if (isInteger(secondArgument)) {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAllFollowedBy(
					stoi(secondArgument)));
			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getFollowStarPairs());
			}
		}
		if (isSynonym(secondArgument)) {
			if (firstArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getAllFollowers());
			}
			else if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getAllFollowing(
					stoi(firstArgument)));
			}
		}
	}
	if (relation == "Parent") {
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAllParents());
			}
			else if (isInteger(secondArgument)) {
				int result = PKB().getParent(stoi(secondArgument));
				if (result > 0) {
					tableResult = ContainerUtil::to_mapvec(firstArgument, result);
				}
				else {
					std::vector<std::string> emptyVec;
					tableResult.insert({ firstArgument, emptyVec });
				}

			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getParentChildPairs());
			}
		}
		if (isSynonym(secondArgument)) {
			if (firstArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getAllChildren());
			}
			else if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getChildren(
					stoi(firstArgument)));
			}
		}
	}
	else if (relation == "Parent*") {
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAllParents());
			}
			else if (isInteger(secondArgument)) {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getAncestors(
					stoi(secondArgument)));
			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getAncDescPairs());
			}
		}
		if (isSynonym(secondArgument)) {
			if (firstArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getAllChildren());
			}
			else if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getDescendants(
					stoi(firstArgument)));
			}
		}
	}
	else if (relation == "Uses") {
		if (isSynonym(secondArgument)) {
			if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getVarUsedByStm(
					stoi(firstArgument)));
			}
			else if (isQuoted(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getVarUsedByProc(
					trimFrontEnd(firstArgument)));
			}
			else if (declarations[firstArgument] == "procedure") {
				return ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getProcVarUsePairs());
			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument,
					PKB().getStmVarUsePairs());
			}
		}
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getStmUsing(""));
			}
			else if (isQuoted(secondArgument)) {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getStmUsing(
					trimFrontEnd(secondArgument)));
			}
		}
	}
	else if (relation == "Modifies") {
		if (isSynonym(secondArgument)) {
			if (isInteger(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getVarModifiedByStm(
				stoi(firstArgument)));
			}
			else if (isQuoted(firstArgument)) {
				tableResult = ContainerUtil::to_mapvec(secondArgument, PKB().getVarModifiedByProc(
				trimFrontEnd(firstArgument)));
			}
			else if (declarations[firstArgument] == "procedure") {
				return ContainerUtil::to_mapvec(firstArgument, secondArgument, 
					PKB().getProcVarModifyPairs());
			}
			else {
				tableResult = ContainerUtil::to_mapvec(firstArgument, secondArgument,
					PKB().getStmVarModifyPairs());
			}
		}
		if (isSynonym(firstArgument)) {
			if (secondArgument == "_") {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getStmModifying(""));
			}
			else if (isQuoted(secondArgument)) {
				tableResult = ContainerUtil::to_mapvec(firstArgument, PKB().getStmModifying(
					trimFrontEnd(secondArgument)));
			}
		}
	}

	if (tableResult.size() == 1) {
		if (isSynonym(firstArgument)) {
			return filterType(firstArgument, declarations, tableResult);
		}
		else {
			return filterType(secondArgument, declarations, tableResult);
		}
 	}
	else if (tableResult.size() == 2) {
		return filterType(firstArgument, secondArgument, declarations, tableResult);
	}
	
	return tableResult;
}

/*
The function returns the list of all statements.
*/
std::unordered_set<std::string> QueryEvaluator::getAllStms() {
	unordered_set<std::string> allStms;
	for (int i = 1; i <= PKB().getTotalStmNo(); i++) {
		allStms.insert(to_string(i));
	}

	return allStms;
}

/*
The function retrieves all statements 
of a given type
*/
std::unordered_map<std::string, std::vector<std::string>> QueryEvaluator::getStmts(
	std::unordered_map<std::string, std::string> declarations,
	std::string syn) {
	std::string synType = declarations[syn];
	if (synType == "stmt") {
		return ContainerUtil::to_mapvec(syn, getAllStms());
	}
	else if (synType == "read") {
		return ContainerUtil::to_mapvec(syn, PKB().getReadStms());
	}
	else if (synType == "print") {
		return ContainerUtil::to_mapvec(syn, PKB().getPrintStms());
	}
	else if (synType == "while") {
		return ContainerUtil::to_mapvec(syn, PKB().getWhileStms());
	}
	else if (synType == "if") {
		return ContainerUtil::to_mapvec(syn, PKB().getIfStms());
	}
	else if (synType == "assign") {
		return ContainerUtil::to_mapvec(syn, PKB().getAssignStms());
	}
	else if (synType == "variable") {
		return ContainerUtil::to_mapvec(syn, PKB().getVariables());
	}
	else if (synType == "constant") {
		return ContainerUtil::to_mapvec(syn, PKB().getConstants());
	}
	else if (synType == "procedure") {
		return ContainerUtil::to_mapvec(syn, PKB().getProcList());
	}
}

/*
The function does filtering
for singleton results from PKB 
so that the output satisfy a given
type.
*/
std::unordered_map<std::string, std::vector<std::string>> QueryEvaluator::filterType(std::string synonym,
	std::unordered_map<std::string, std::string> declarations,
	std::unordered_map<std::string, std::vector<std::string>> toBeFiltered) {
	std::unordered_map<std::string, std::vector<std::string>> filteredTable;
	std::string synonymType = declarations[synonym];
	std::unordered_map<std::string, std::vector<std::string>> typeRequiredSet = getStmts(
		declarations, synonym);
	std::vector<std::string> filter = typeRequiredSet[synonym];
	if (synonymType == "stmt") {
		return toBeFiltered;
	} 
	else {
		std::vector<std::string> oldColumn = toBeFiltered[synonym];
		std::vector<std::string> newColumn;
		for (std::vector<std::string>::size_type i = 0; i != oldColumn.size(); i++) {
			if (std::find(filter.begin(), filter.end(), oldColumn[i]) != filter.end()) {
				newColumn.push_back(oldColumn[i]);
			}
		}
		filteredTable.insert({ synonym, newColumn });
		return filteredTable;
	}
}

/*
The function filters the set of strings
of pairs so that the first member and 
second member is in the set of all 
statements in a first type and a second
type respectively.
*/
std::unordered_map<std::string, std::vector<std::string>> QueryEvaluator::filterType(std::string synonym1, 
	std::string synonym2, std::unordered_map<std::string, std::string> declarations,
	std::unordered_map<std::string, std::vector<std::string>> toBeFiltered) {
	std::unordered_map<std::string, std::vector<std::string>> filteredSet;
	std::string synonym1Type = declarations[synonym1];
	std::string synonym2Type = declarations[synonym2];
	std::unordered_map<std::string, std::vector<std::string>> type1RequiredSet = getStmts(
		declarations, synonym1);
	std::unordered_map<std::string, std::vector<std::string>> type2RequiredSet = getStmts(
		declarations, synonym2);
	std::vector<std::string> filter1 = type1RequiredSet[synonym1];
	std::vector<std::string> filter2 = type2RequiredSet[synonym2];
	std::vector<std::string> oldColumn1 = toBeFiltered[synonym1];
	std::vector<std::string> oldColumn2 = toBeFiltered[synonym2];
	if (synonym1Type == "stmt" && synonym2Type == "stmt") {
		return toBeFiltered;
	}
	else if (synonym1Type == "stmt" && synonym2Type != "stmt") {
		for (auto it = toBeFiltered.begin(); it != toBeFiltered.end(); ++it) {
			std::vector<std::string> newColumn;
			for (std::vector<std::string>::size_type i = 0; i != oldColumn2.size(); i++) {
				if (std::find(filter2.begin(), filter2.end(), oldColumn2[i]) != filter2.end()) {
					newColumn.push_back(it->second[i]);
				}
			}
			filteredSet.insert({ it->first, newColumn });
		}
	}
	else if (synonym1Type != "stmt" && synonym2Type == "stmt") {
		for (auto it = toBeFiltered.begin(); it != toBeFiltered.end(); ++it) {
			std::vector<std::string> newColumn;
			for (std::vector<std::string>::size_type i = 0; i != oldColumn1.size(); i++) {
				if (std::find(filter1.begin(), filter1.end(), oldColumn1[i]) != filter1.end()) {
					newColumn.push_back(it->second[i]);
				}
			}
			filteredSet.insert({ it->first, newColumn });
		}
	}
	else if (synonym1Type != "stmt" && synonym2Type != "stmt") {
		for (auto it = toBeFiltered.begin(); it != toBeFiltered.end(); ++it) {
			std::vector<std::string> newColumn;
			for (std::vector<std::string>::size_type i = 0; i != oldColumn1.size(); i++) {
				if (std::find(filter1.begin(), filter1.end(), oldColumn1[i]) != filter1.end() 
					&& std::find(filter2.begin(), filter2.end(), oldColumn2[i]) != filter2.end()) {
					newColumn.push_back(it->second[i]);
				}
			}
			filteredSet.insert({ it->first, newColumn });
		}
	}
	return filteredSet;
}

/*
The function transforms a boolean value
into a string.
*/
string QueryEvaluator::truthValue(bool boolean) {
	if (boolean) {
		return "true";
	}

	return "false";
}

/*
Trims quote in front and end of a string.
*/
string QueryEvaluator::trimFrontEnd(string quotedString) {
	return quotedString.substr(1, quotedString.size() - 2);
}

/*
Checks if the string is an integer
*/
bool QueryEvaluator::isInteger(string s) {
	bool result = LexicalToken::verifyInteger(s);
	return result;
}

/*
Checks if the string is an integer
*/
bool QueryEvaluator::isQuoted(string s) {
	bool result = s[0] == '"';
	return result;
}

/*
Checks if the string is an integer
*/
bool QueryEvaluator::isSynonym(string s) {
	bool result = !isInteger(s) && !isQuoted(s) && s != "_";
	return result;
}
