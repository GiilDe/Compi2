//
// Created by Gilad on 23-Apr-19.
//
#include <algorithm>
#include <stack>
#include "grammar.h"
#include "hw2.h"
#include <unordered_map>


using std::vector;
using std::stack;
using std::set;
using std::unordered_map;
using std::pair;


//std::vector<grammar_rule> grammar;
vector<bool> nullables;
vector<set<tokens>> first;
vector<set<tokens>> follow;
vector<set<tokens>> select;


uint tokens_size = static_cast<int>(EF)-static_cast<int>(IMPORTANT)+1;
uint nonterminal_size = static_cast<int>(NONTERMINAL_ENUM_SIZE);
uint rules_size = grammar.size();


bool is_token(const int &n) {
    return 20 <= n && n <= 38;
}

bool in_nullables(const int &letter) {
    return !is_token(letter) && nullables[letter];
}

bool in_nullables(nonterminal &nonterm) {
    auto index = static_cast<int>(nonterm);
    return in_nullables(index);
}

bool all_nullable(vector<int>& rhs){
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
    nullables = vector<bool>(nonterminal_size);
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


void compute_first(){
    first = vector<set<tokens>>(39);

    //initialize terminals first
    for(int i = 20; i <= 38; i++){
        set<tokens> temp;
        temp.insert(static_cast<tokens>(i));
        first[i] = temp;
    }

    bool changed;
    do {
        changed = false;
        for (grammar_rule& rule : grammar) {
            auto left_index = static_cast<int>(rule.lhs);
            set<tokens>& left_first = first[left_index];
            set<tokens> temp = left_first;
            auto i = rule.rhs.begin();

            while (i != rule.rhs.end() &&
                    (i == rule.rhs.begin() || in_nullables(*(i - 1)))){
                auto right_index = static_cast<int>(*i);
                set<tokens>& right_first = first[right_index];

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
    follow = vector<set<tokens>>(nonterminal_size);
    auto S_index = static_cast<int>(S);
    follow[S_index].insert(EF);

    bool changed;
    do {
        changed = false;
        for (int i = 0; i < nonterminal_size; ++i) {
            set<tokens>& this_follow = follow[i];
            set<tokens> temp = this_follow;

            for (grammar_rule &rule : grammar) {
                auto found = std::find(rule.rhs.begin(), rule.rhs.end(), i);
                auto j = found;

                if(found != rule.rhs.end()){
                    j++;
                }

                bool last_nullable = true;
                bool first_iter = true;
                while (j != rule.rhs.end() &&
                       (first_iter || in_nullables(*(j - 1)))){
                    auto right_index = static_cast<int>(*j);
                    set<tokens>& right_first = first[right_index];

                    this_follow.insert(right_first.begin(),
                                       right_first.end());

                    last_nullable = in_nullables(*j);
                    first_iter = false;
                    j++;
                }
                if(last_nullable && found != rule.rhs.end()){
                    auto left_index = static_cast<int>(rule.lhs);
                    set<tokens>& left_follow = follow[left_index];
                    this_follow.insert(left_follow.begin(),
                                       left_follow.end());
                }
            }

            if(temp != this_follow){
                changed = true;
            }
        }
    }while (changed);
    print_follow(follow);
}

static std::set<tokens> compute_first_recursive(std::vector<int>& rhs) {
    std::set<tokens> computed_first = std::set<tokens>();

    for (const int& term : rhs) {
        if(in_nullables(term)) {
            auto index = term;
            std::set<tokens> &right_first = first[index];

            computed_first.insert(right_first.begin(),
                              right_first.end());
        }
    }
    return computed_first;
}


void compute_select() {
    select = vector<set<tokens>>(grammar.size());

    int index = 0;
    for (grammar_rule &rule : grammar) {
        select[index] = compute_first_recursive(rule.rhs);
        if (all_nullable(rule.rhs)) {
            std::set<tokens> follow_tokens = follow[rule.lhs];
            select[index].insert(follow_tokens.begin(), follow_tokens.end());
        }
        index++;
    }
    print_select(select);
}

void parser(){
    stack<int> Q;
    Q.push(EF);

    //create M
    unordered_map<pair<nonterminal, tokens>, grammar_rule> M;

    for (int i = 0; i < rules_size; ++i) {
        grammar_rule& rule = grammar[i];
        set<tokens>& rule_select = select[i];
        nonterminal& left_side = rule.lhs;

        for(const tokens& t : s){
            M[pair(left_side, t)] = rule;
        }
    }




}
