#ifndef __HW2__
#define __HW2__

#include "grammar.h"
#include <stack>
#include <algorithm>
#include <map>

#define FOR_EACH(iter, ds, name) \
    for (ds::iterator iter = name.begin(); iter != name.end(); iter++)

using std::vector;
using std::stack;
using std::set;
using std::map;

//std::vector<grammar_rule> grammar;
vector<bool> nullables;
vector<set<tokens> > first;
vector<set<tokens> > follow;
vector<set<tokens> > select;

typedef unsigned int uint;

const uint nonterminal_size = static_cast<int>(NONTERMINAL_ENUM_SIZE);

bool is_token(const int &n) {
    return 20 <= n && n <= 38;
}

bool in_nullables(const int &letter) {
    return !is_token(letter) && nullables[letter];
}

bool in_nullables(nonterminal &nonterm) {
    int index = static_cast<int>(nonterm);
    return in_nullables(index);
}

bool all_nullable(vector<int>& rhs){
    FOR_EACH(iter, vector<int>, rhs) {
        const int& letter = *iter;
        if(is_token(letter) || !in_nullables(letter))  {
            return false;
        }
    }
    return true;
}

void insert_to_nullables(nonterminal& t){
    int index = static_cast<int>(t);
    nullables[index] = true;
}

/**
 * determines which variables are nullable, i.e. can produce an empty word
 * calls print_nullable when finished
 */
void compute_nullable() {
    nullables = vector<bool>(nonterminal_size);
    bool changed;
    do {
        changed = false;
        FOR_EACH(iter, vector<grammar_rule>, grammar) {
            grammar_rule& rule = *iter;
            if ((rule.rhs.empty() || all_nullable(rule.rhs)) &&
                !in_nullables(rule.lhs)) {
                changed = true;
                insert_to_nullables(rule.lhs);
            }
        }
    }while (changed);
    print_nullable(nullables);
}

/**
 * computes first for all nonterminal (see nonterminal enum in grammar.h)
 * calls print_first when finished
 */
void compute_first() {
    first = vector<set<tokens> >(39);

    //initialize terminals first
    for(int i = 20; i <= 38; i++){
        set<tokens> temp;
        temp.insert(static_cast<tokens>(i));
        first[i] = temp;
    }

    bool changed;
    do {
        changed = false;
        FOR_EACH(iter, vector<grammar_rule>, grammar) {
            grammar_rule& rule = *iter;

            int left_index = static_cast<int>(rule.lhs);
            set<tokens>& left_first = first[left_index];
            set<tokens> temp = left_first;
            vector<int>::iterator i = rule.rhs.begin();

            while (i != rule.rhs.end() &&
                   (i == rule.rhs.begin() || in_nullables(*(i - 1)))){
                int right_index = static_cast<int>(*i);
                set<tokens>& right_first = first[right_index];

                left_first.insert(right_first.begin(),
                                  right_first.end());
                i++;
            }

            if(left_first != temp){
                changed = true;
            }
        }
    } while (changed);
    print_first(first);
}

/**
 * computes follow for all nonterminal (see nonterminal enum in grammar.h)
 * calls print_follow when finished
 */
void compute_follow() {
    follow = vector<set<tokens> >(nonterminal_size);
    int S_index = static_cast<int>(S);
    follow[S_index].insert(EF);

    bool changed;
    do {
        changed = false;
        for (int i = 0; i < nonterminal_size; ++i) {
            set<tokens>& this_follow = follow[i];
            set<tokens> temp = this_follow;

            FOR_EACH(iter, vector<grammar_rule>, grammar) {
                grammar_rule &rule = *iter;

                vector<int>::iterator found = std::find(rule.rhs.begin(), rule.rhs.end(), i);
                vector<int>::iterator j = found;

                if(found != rule.rhs.end()){
                    j++;
                }

                bool last_nullable = true;
                bool first_iter = true;
                while (j != rule.rhs.end() &&
                       (first_iter || in_nullables(*(j - 1)))){
                    int right_index = static_cast<int>(*j);
                    set<tokens>& right_first = first[right_index];

                    this_follow.insert(right_first.begin(),
                                       right_first.end());

                    last_nullable = in_nullables(*j);
                    first_iter = false;
                    j++;
                }
                if(last_nullable && found != rule.rhs.end()){
                    int left_index = static_cast<int>(rule.lhs);
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

    FOR_EACH(iter, vector<int>, rhs) {
        const int& term = *iter;
        std::set<tokens> &right_first = first[term];
        computed_first.insert(right_first.begin(), right_first.end());

        if(!in_nullables(term)) {
            break;
        }
    }
    return computed_first;
}

/**
 * computes select for all grammar rules (see grammar global variable in grammar.h)
 * calls print_select when finished
 */
void compute_select() {
    select = vector<set<tokens> >(grammar.size());

    int index = 0;
    FOR_EACH(iter, vector<grammar_rule>, grammar) {
        grammar_rule &rule = *iter;
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

typedef vector<vector<int> > predict_map;

static void push_all(stack<int>& Q, const vector<int>& rule_rhs) {
    vector<int>::const_iterator term;

    for (term = rule_rhs.end(); term != rule_rhs.begin(); term--) {
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
//    map<int, int>& token_map = M[X];
    // map<int, int>::iterator iter;

    // iter = token_map.find(t);
    if (std::find(M[X].begin(), M[X].end(), t) == M[X].end()) {
        // Not found
        throw PredictException();
    } else {
        Q.pop();
        int map_entry = M[X][t];
        grammar_rule rule = grammar[map_entry];
        push_all(Q, rule.rhs);
        printf("%d\n", map_entry);
    }
}

/*
 * implemented in lex.yy.c (generated from lexer.lex)
 */
int yylex();


/**
 * implements an LL(1) parser for the grammar using yylex()
 */
void parser() {
    compute_nullable();
    compute_first();
    compute_follow();
    compute_select();

    stack<int> Q;
    Q.push(S);

    //create M
    const uint rules_size = grammar.size();
    predict_map M(rules_size);

    for (int i = 0; i < rules_size; ++i) {
        grammar_rule& rule = grammar[i];
        set<tokens>& rule_select = select[i];
        nonterminal left_side = rule.lhs;


        vector<int> v(nonterminal_size);
        FOR_EACH(iter, set<tokens>, rule_select) {
            v[*iter] = i;
        }
        M[left_side] = v;
    }

    tokens t = static_cast<tokens>(yylex());

    while (!Q.empty()) {
        const int stack_top = Q.top();
        if (is_token(stack_top)) {
            // Match t,t
            tokens X = static_cast<tokens>(stack_top);
            try {
                match(Q, X, t);
            } catch (const MatchException&) {
                // TODO Handle
                printf("Match Exception\n");
            }
        } else {
            // Predict X,t
            nonterminal X = static_cast<nonterminal>(stack_top);
            try {
                predict(Q, M, X, t);
            } catch (const PredictException&) {
                // TODO Handle
                printf("Predict Exception\n");
            }

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

#endif //__HW2__
