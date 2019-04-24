//
// Created by Gilad on 23-Apr-19.
//
#include <algorithm>
#include "grammar.h"

std::vector<grammar_rule> grammar;
std::vector<bool> nullables;
std::vector<std::set<tokens> > first;
std::vector<std::set<tokens> > follow;


bool is_token(int& n){
    return 20 <= n && n <= 38;
}

bool in_nullables(int& nonterm){
    return nullables[nonterm];
}

bool in_nullables(nonterminal& nonterm){
    auto index = static_cast<int>(nonterm);
    return in_nullables(index);
}

bool all_nullable(std::vector<int>& rhs){
    for(int& letter : rhs){
        if(is_token(letter) || !in_nullables(letter))  {
            return false;
        }
    }
    return true;
}

void insert_to_nullables(nonterminal& t){
    auto index = static_cast<int>(t);
    nullables[index] = true;
}

void compute_nullable(){
    nullables = std::vector(static_cast<int>(NONTERMINAL_ENUM_SIZE));
    bool changed;
    do {
        changed = false;
        for (grammar_rule& rule : grammar) {
            if ((rule.rhs.empty() || all_nullable(rule.rhs)) &&
                !in_nullables(rule.lhs)) {
                changed = true;
                insert_to_nullables(rule.lhs);
            }
        }
    }while (changed);
    print_nullable(nullables);
}

void initialize_terminals_first(){
    for(int i = 20; i <= 38; i++){
        std::set<tokens> temp;
        temp.insert(static_cast<tokens>(i));
        first[i] = temp;
    }
}

void compute_first(){
    first = std::vector<std::set<tokens> >(39);
    initialize_terminals_first();

    bool changed;
    do {
        changed = false;
        for (grammar_rule& rule : grammar) {
            auto left_index = static_cast<int>(rule.lhs);
            std::set<tokens>& left_first = first[left_index];
            std::set<tokens> temp = left_first;
            auto i = rule.rhs.begin();

            while (i != rule.rhs.end() &&
                    (i == rule.rhs.begin() || in_nullables(*(i - 1)))){
                auto right_index = static_cast<int>(*i);
                std::set<tokens> &right_first = first[right_index];

                left_first.insert(right_first.begin(),
                                  right_first.end());
                i++;
            }

            if(left_first != temp){
                changed = true;
            }
        }
    }while (changed);
    print_first(first);
}

void compute_follow(){
    follow = std::vector<std::set<tokens> >(static_cast<int>(NONTERMINAL_ENUM_SIZE));
    auto S_index = static_cast<int>(S);
    follow[S_index].insert(EF);

    bool changed;
    do {
        changed = false;
        for (int i = 0; i < static_cast<int>(NONTERMINAL_ENUM_SIZE); ++i) {
            std::set<tokens>& this_follow = follow[i];
            std::set<tokens> temp = this_follow;

            for (grammar_rule &rule : grammar) {
                auto k = std::find(rule.rhs.begin(), rule.rhs.end(), i);

                if(k != rule.rhs.end()){
                    k++;
                }
                auto j = k;
                while (j != rule.rhs.end() &&
                       (j == k || in_nullables(*(j - 1)))){
                    auto right_index = static_cast<int>(*j);
                    std::set<tokens> &right_follow = follow[right_index];

                    this_follow.insert(right_follow.begin(),
                                       right_follow.end());

                    j++;
                }
                if(j == rule.rhs.end()){
                    auto left_index = static_cast<int>(rule.lhs);
                    std::set<tokens>& left_follow = follow[left_index];
                    this_follow.insert(left_follow.begin(),
                                       left_follow.end());
                }
            }

            if(temp != this_follow){
                changed = true;
            }
        }
    }while (changed);

}