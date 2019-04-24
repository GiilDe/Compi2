//
// Created by Gilad on 23-Apr-19.
//
#include "grammar.h"

std::vector<grammar_rule> grammar;


bool is_token(int n){
    return 20 <= n && n <= 38;
}

bool in_nullables(int& nonterm, std::set<nonterminal>& nullables){
    return nullables.find(static_cast<nonterminal>(nonterm)) != nullables.end();
}

bool in_nullables(nonterminal& nonterm, std::set<nonterminal>& nullables){
    return nullables.find(nonterm) != nullables.end();
}

bool all_nullable(std::vector<int>& rhs, std::set<nonterminal>& nullables){
    for(int& letter : rhs){
        if(is_token(letter) || !in_nullables(letter, nullables))  {
            return false;
        }
    }
    return true;
}

void compute_nullable(){
    std::set<nonterminal> nullables;
    bool changed;
    do {
        changed = false;
        for (grammar_rule& rule : grammar) {
            if ((rule.rhs.empty() || all_nullable(rule.rhs, nullables)) &&
                                     !in_nullables(rule.lhs, nullables)) {
                nullables.insert(rule.lhs);
                changed = true;
            }
        }
    }while (!changed);
    std::vector res(nullables.begin(), nullables.end());
    print_nullable(res);
}