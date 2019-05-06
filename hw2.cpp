//
// Created by Gilad on 23-Apr-19.
//
#include <algorithm>
#include <stack>
#include "grammar.h"
#include "hw2.h"
#include <unordered_map>
#include <map>


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

typedef unsigned int uint;
typedef pair<nonterminal, tokens> nonterminal_terminal;
typedef pair<grammar_rule, int> rule_int;

const uint tokens_size = static_cast<int>(EF) - static_cast<int>(IMPORTANT) + 1;
const uint nonterminal_size = static_cast<int>(NONTERMINAL_ENUM_SIZE);
const uint rules_size = grammar.size();

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
        auto index = term;
        std::set<tokens> &right_first = first[index];
        computed_first.insert(right_first.begin(), right_first.end());

        if(!in_nullables(term)) {
            break;
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

class MatchException : public std::exception {};
class PredictException : public std::exception {};

/**
 * MATCH t,t
 * Match the top of the parser stack and the next terminal of the input.
 * @param Q The parser stack
 * @param X The current stack top (terminal)
 * @param t The next input terminal
 */
static void match(stack<int>& Q, const tokens& X, const tokens& t) {
    if (X == t) {
        Q.pop();
    } else {
        throw MatchException();
    }
}

template<>
struct std::hash<nonterminal_terminal> {

    std::size_t operator()(const nonterminal_terminal& ntt) const {
        return ntt.first * ntt.second;
    }
};

typedef std::map<nonterminal, std::map<tokens, int>> predict_map;

static void push_all(stack<int>& Q, const vector<int>& rule_rhs) {
    for (auto term = rule_rhs.end(); term != rule_rhs.begin(); term--) {
        Q.push(*term);
    }
}

/**
 * PREDICT X,t
 * Swap the nonterminal X for an appropriate grammer rule
 * @param Q The parser stack
 * @param M The prediction map
 * @param X The current stack top (nonterminal)
 * @param t The next input terminal
 */
static void predict(stack<int>& Q, predict_map& M, const nonterminal & X, const tokens& t) {
    std::map<tokens, int> token_map = M[X];
    auto iter = token_map.find(t);
    if (iter == token_map.end()) {
        // Not found
        throw PredictException();
    } else {
        Q.pop();
        int map_entry = (*iter).second;
        auto rule = grammar[map_entry];
        push_all(Q, rule.rhs);
        printf("%d\n", map_entry);
    }
}

void parser() {
    stack<int> Q;
    Q.push(S);

    //create M
    predict_map M;

    for (int i = 0; i < rules_size; ++i) {
        grammar_rule& rule = grammar[i];
        set<tokens>& rule_select = select[i];
        nonterminal& left_side = rule.lhs;

        for(const tokens& t : rule_select) {
            M[left_side][t] = i;
        }
    }

    auto t = static_cast<tokens>(yylex());

    while (!Q.empty()) {
        auto stack_top = Q.top();
        if (is_token(stack_top)) {
            // Match t,t
            auto X = static_cast<tokens>(stack_top);
            match(Q, X, t);
        } else {
            // Match X,t
            auto X = static_cast<nonterminal>(stack_top);
            predict(Q, M, X, t);

        }
        t = static_cast<tokens>(yylex());
    }

    if (t == EF) {
        // TODO Report success
        printf("Success\n");
    } else {
        // TODO Report error
        printf("Syntax error\n");
    }
}
