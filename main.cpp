#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <vector>
#define epsilon "ε"
using namespace std;

enum Kind {
    EPSILON,
    CHAR,
    CONNECT,
    OR,
    STAR,
    PLUS
};

class NFA {
private:
    int final;
    map<int, map<string, vector<int>>> transition;

public:
    vector<int> states;
    vector<string> input;
    int start;

public:
    NFA() { input.push_back(epsilon); }
    ~NFA() {}
    int addState() {
        this->states.push_back(states.size());
        return states.size() - 1;
    }
    void addInput(string input) {
        if (find(this->input.begin(), this->input.end(), input) == this->input.end())
            this->input.push_back(input);
    }
    void addTransition(int from, string input, int to) {
        transition[from][input].push_back(to);
    }
    int getStart() { return this->start; }
    int getFinal() { return this->final; }
    vector<int> get_next_states(int state, string input) { return transition[state][input]; }
    void setStart(int start) { this->start = start; }
    void setFinal(int final) { this->final = final; }
};

class Re2NFA {
private:
    string regex;
    vector<string> keep_word;
    NFA *nfa;

public:
    Re2NFA(const string &regex, const vector<string> &keep_word) {
        this->nfa = new NFA();
        this->regex = regex;
        for (auto word : keep_word) {
            this->keep_word.push_back(word);
        }
    }
    ~Re2NFA() {
        clear();
    }
    void clear() {
        regex.clear();
        keep_word.clear();
        delete nfa;
        nfa = NULL;
    }

    void Re_to_NFA() {
        Re_to_NFA(Re2Post(Format()));
    }

    NFA *getNFA() { return nfa; }
    void generate_NFA_png() {
        ofstream dotfile("./nfa.dot");
        if (!dotfile) {
            cout << "Failed to open file" << endl;
            return;
        }
        dotfile << "digraph {\n";
        dotfile << "    rankdir=LR;\n";
        dotfile << "    node [shape=circle];\n";
        dotfile << "    " << to_string(nfa->getFinal()) << " [shape=doublecircle];\n";
        for (auto state : nfa->states) {
            for (auto input : nfa->input) {
                for (auto to : nfa->get_next_states(state, input)) {
                    dotfile << "    " << to_string(state) << " -> " << to_string(to) << " [label=\"" << input << "\"];\n";
                }
            }
        }
        dotfile << "}\n";
        dotfile.close();
        system("dot -Tpng ./nfa.dot -o ./nfa.png");
    }

private:
    vector<string> Format() {
        vector<pair<int, string>> infix; // 0: char, 1: op
        vector<string> res;
        // check [] and replace keep_word
        int i = 0;
        while (i < regex.size()) {
            string tmp = "";
            if (regex[i] == '[') {
                while (regex[i] != ']' && i < regex.size()) {
                    tmp += regex[i++];
                }
                if (i >= regex.size()) {
                    cout << "[] Error" << endl;
                    exit(1);
                }
                tmp += regex[i++];
                infix.push_back(make_pair(0, tmp));
            } else if (regex[i] == '\\') { // if escape char catch next char
                tmp += regex[i++];
                if (i < regex.size())
                    tmp += regex[i];
                infix.push_back(make_pair(0, tmp));
                i++;
            } else if (regex[i] == '(' || regex[i] == ')' || regex[i] == '*' || regex[i] == '+' || regex[i] == '?' || regex[i] == '|') {
                tmp += regex[i++];
                infix.push_back(make_pair(1, tmp));
            } else { // word
                int len_keep_word = 0;
                for (auto word : keep_word) {
                    len_keep_word = word.size();
                    if (regex.substr(i, len_keep_word) == word && i + len_keep_word <= regex.size()) {
                        infix.push_back(make_pair(0, word));
                        i += len_keep_word;
                        break;
                    }
                    len_keep_word = 0;
                }
                if (len_keep_word == 0) {
                    tmp += regex[i++];
                    infix.push_back(make_pair(0, tmp));
                }
            }
        }
        // add CONNECT
        for (int i = 0; i < infix.size() - 1; i++) {
            res.push_back(infix[i].second);
            if (infix[i].first == 0 && (infix[i + 1].first == 0 || infix[i + 1].second == "(")) {
                res.push_back("&"); // between two chars or between char and open bracket
            } else if (infix[i].first == 1 && (infix[i].second == "+" || infix[i].second == "*" || infix[i].second == "?" || infix[i].second == ")") && (infix[i + 1].first == 0 || infix[i + 1].second == "(")) {
                res.push_back("&"); // between op and (char/left bracket); op must be one of "+", "*", "?", ")"
            }
        }
        res.push_back(infix[infix.size() - 1].second);
        return res;
    }

    int priority(const char ch) {
        int p;
        switch (ch) {
        case '*':
        case '+':
        case '?':
            p = 2;
            break;
        case '|':
            p = 0;
            break;
        case '&':
            p = 1;
            break;
        default:
            p = -1;
            break;
        }
        return p;
    }

    vector<string> Re2Post(const vector<string> &infix) {
        vector<string> postfix;
        stack<string> s;
        for (string ch : infix) {
            if (ch == "(") {
                s.push(ch);
            } else if (ch == ")") {
                while (s.top() != "(") {
                    postfix.push_back(s.top());
                    s.pop();
                }
                s.pop();
            } else if (ch == "*" || ch == "+" || ch == "?" || ch == "|" || ch == "&") {
                while (!s.empty() && priority(ch[0]) <= priority(s.top()[0])) {
                    postfix.push_back(s.top());
                    s.pop();
                }
                s.push(ch);
            } else {
                postfix.push_back(ch);
            }
        }
        while (!s.empty()) {
            postfix.push_back(s.top());
            s.pop();
        }
        return postfix;
    }

    void Re_to_NFA(const vector<string> &postfix) {
        stack<pair<int, int>> s;
        for (int i = 0; i < postfix.size(); i++) {
            string tmp = postfix[i];
            if (tmp == "*") {
                pair<int, int> p = s.top();
                s.pop();
                int start = nfa->addState();
                int end = nfa->addState();
                nfa->addTransition(start, epsilon, p.first);
                nfa->addTransition(p.second, epsilon, end);
                nfa->addTransition(start, epsilon, end);
                nfa->addTransition(p.second, epsilon, p.first);
                nfa->setStart(start);
                nfa->setFinal(end);
                s.push(make_pair(start, end));
            } else if (tmp == "+") {
                pair<int, int> p = s.top();
                s.pop();
                int start = nfa->addState();
                int end = nfa->addState();
                nfa->addTransition(start, epsilon, p.first);
                nfa->addTransition(p.second, epsilon, end);
                nfa->addTransition(p.second, epsilon, p.first);
                nfa->setStart(start);
                nfa->setFinal(end);
                s.push(make_pair(start, end));
            } else if (tmp == "|") {
                pair<int, int> p = s.top();
                s.pop();
                pair<int, int> q = s.top();
                s.pop();
                int start = nfa->addState();
                int end = nfa->addState();
                nfa->addTransition(start, epsilon, q.first);
                nfa->addTransition(start, epsilon, p.first);
                nfa->addTransition(q.second, epsilon, end);
                nfa->addTransition(p.second, epsilon, end);
                nfa->setStart(start);
                nfa->setFinal(end);
                s.push(make_pair(start, end));
            } else if (tmp == "&") {
                pair<int, int> p = s.top(); // second
                s.pop();
                pair<int, int> q = s.top(); // first
                s.pop();
                nfa->addTransition(q.second, epsilon, p.first);
                nfa->setStart(q.first);
                nfa->setFinal(p.second);
                s.push(make_pair(q.first, p.second));
            } else if (tmp == "?") {
                pair<int, int> p = s.top();
                s.pop();
                int start = nfa->addState();
                int end = nfa->addState();
                nfa->addTransition(start, epsilon, p.first);
                nfa->addTransition(p.second, epsilon, end);
                nfa->addTransition(start, epsilon, end);
                nfa->setStart(start);
                nfa->setFinal(end);
                s.push(make_pair(start, end));
            } else {
                int start = nfa->addState();
                int end = nfa->addState();
                nfa->addInput(tmp);
                nfa->addTransition(start, tmp, end);
                nfa->setStart(start);
                nfa->setFinal(end);
                s.push(make_pair(start, end));
            }
        }
    }
};

class DFA {
public:
    int start;
    vector<int> final;
    vector<string> input;
    vector<set<int>> subsets;
    map<int, map<string, int>> transition;

public:
    DFA() {}
    ~DFA() {}
    void addInput(string input) { this->input.push_back(input); }
    void addTransition(int from, string input, int to) { transition[from][input] = to; }
    int add_subset(set<int> subset) {
        subsets.push_back(subset);
        final.push_back(0);
        return subsets.size() - 1;
    }
    vector<int> getFinal() { return this->final; }
    int get_next_states(int state, string input) {
        if (transition[state].find(input) == transition[state].end())
            return -1;
        return transition[state][input];
    }
    void setFinal(int state) { final[state] = 1; }
    void setStart(int start) { this->start = start; }
};

class NFA2DFA {
private:
    NFA *nfa;
    DFA *dfa;

public:
    NFA2DFA(Re2NFA *re2nfa) {
        re2nfa->Re_to_NFA();
        re2nfa->generate_NFA_png();

        nfa = re2nfa->getNFA();
        dfa = new DFA();
    }
    DFA *getDFA() { return dfa; }
    ~NFA2DFA() {
        nfa = NULL;
        delete dfa;
    }

    set<int> eps_closure(int state) {
        set<int> res;
        stack<int> s;
        vector<int> visit(nfa->states.size(), 0);
        s.push(state);
        while (!s.empty()) {
            int tmp = s.top();
            s.pop();
            visit[tmp]++;
            res.insert(tmp);
            for (auto it : nfa->get_next_states(tmp, epsilon)) {
                if (visit[it] == 0) {
                    s.push(it);
                }
            }
        }
        return res;
    }

    void NFA_to_DFA() {
        for (auto input : nfa->input) {
            if (input != epsilon)
                dfa->addInput(input);
        }
        set<int> closure = eps_closure(nfa->getStart());
        int start = dfa->add_subset(closure);
        dfa->setStart(start);
        queue<pair<int, set<int>>> work_list;
        work_list.push(make_pair(start, closure));
        while (!work_list.empty()) {
            pair<int, set<int>> p = work_list.front();
            work_list.pop();
            for (auto q : p.second) {
                for (auto c : nfa->input) {
                    if (c == epsilon)
                        continue;
                    vector<int> next_states = nfa->get_next_states(q, c);
                    set<int> closure;
                    for (auto state : next_states) {
                        set<int> t = eps_closure(state);
                        closure.insert(t.begin(), t.end());
                    }
                    if (closure.size() == 0)
                        continue;
                    int next = find(dfa->subsets.begin(), dfa->subsets.end(), closure) - dfa->subsets.begin();
                    if (next == dfa->subsets.size()) {
                        next = dfa->add_subset(closure);
                        work_list.push(make_pair(next, closure));
                    }
                    dfa->addTransition(p.first, c, next);
                    if (closure.find(nfa->getFinal()) != closure.end())
                        dfa->setFinal(next);
                }
            }
        }
    }

    void generate_DFA_png() {
        ofstream dotfile("./dfa.dot");
        if (!dotfile) {
            cout << "Failed to open file" << endl;
            return;
        }
        dotfile << "digraph {\n";
        dotfile << "    rankdir=LR;\n";
        dotfile << "    node [shape=circle];\n";
        vector<int> final = dfa->getFinal();
        for (int i = 0; i < final.size(); i++) {
            if (final[i])
                dotfile << "    " << to_string(i) << " [shape=doublecircle];\n";
        }
        for (int i = 0; i < dfa->subsets.size(); i++) {
            for (auto input : dfa->input) {
                int to = dfa->get_next_states(i, input);
                if (to != -1)
                    dotfile << "    " << to_string(i) << " -> " << to_string(to) << " [label=\"" << input << "\"];\n";
            }
        }
        dotfile << "}\n";
        dotfile.close();
        system("dot -Tpng ./dfa.dot -o ./dfa.png");
    }
};

class minDFA {
public:
    int start;
    map<int, map<string, int>> transition;
    map<int, set<int>> subsets_group;
    DFA *dfa;
    vector<int> minDfa_final;

public:
    minDFA(NFA2DFA *nfa2dfa) {
        nfa2dfa->NFA_to_DFA();
        nfa2dfa->generate_DFA_png();
        dfa = nfa2dfa->getDFA();
    }

    map<int, set<int>> split(pair<int, set<int>> group, const vector<int> &group_id, string input) {
        map<int, set<int>> res; // res[0]: null from state by input; res[i]: group i from state by input
        for (auto it : group.second) {
            int to = dfa->get_next_states(it, input);
            if (to == -1)
                res[0].insert(it);
            res[group_id[to]].insert(it);
        }
        return res;
    }

    map<int, set<int>> Union(const map<int, set<int>> &T1, const map<int, set<int>> &T2, vector<int> &group_id) {
        map<int, set<int>> res;
        int i = 1;
        for (auto it : T1) {
            for (auto it2 : it.second) {
                group_id[it2] = i;
            }
            res[i++] = it.second;
        }
        for (auto it : T2) {
            for (auto it2 : it.second) {
                group_id[it2] = i;
            }
            res[i++] = it.second;
        }
        return res;
    }

    void minimize() {
        // init: split to 2 group
        vector<int> group_id(dfa->subsets.size());
        vector<int> final = dfa->getFinal();
        for (int i = 0; i < dfa->subsets.size(); i++) {
            if (final[i] == 1) { // final state
                group_id[i] = 1;
                subsets_group[1].insert(i);
            } else { // not final state
                subsets_group[2].insert(i);
                group_id[i] = 2;
            }
        }
        for (auto input : dfa->input) {
            while (true) {
                map<int, set<int>> T;
                for (auto it : subsets_group)
                    T = Union(T, split(it, group_id, input), group_id);
                if (T.size() == subsets_group.size())
                    break;
                subsets_group = T;
            }
        }

        for (auto p : subsets_group) {
            // set final
            for (auto state : p.second) {
                if (final[state] == 1) {
                    minDfa_final.push_back(p.first);
                    break;
                }
            }
            // set transition
            auto from = *(p.second.begin());
            for (auto input : dfa->transition[from]) {
                transition[group_id[from]][input.first] = group_id[input.second];
            }
        }
    }

    void generate_minDFA_png() {
        ofstream dotfile("./minDfa.dot");
        if (!dotfile) {
            cout << "Failed to open file" << endl;
            return;
        }
        dotfile << "digraph {\n";
        dotfile << "    rankdir=LR;\n";
        dotfile << "    node [shape=circle];\n";
        for (auto state : minDfa_final) {
            dotfile << "    " << to_string(state) << " [shape=doublecircle];\n";
        }
        for (auto p : subsets_group) {
            for (auto input : dfa->input) {
                if (transition[p.first].find(input) == transition[p.first].end())
                    continue;
                int to = transition[p.first][input];
                dotfile << "    " << to_string(p.first) << " -> " << to_string(to) << " [label=\"" << input << "\"];\n";
            }
        }
        dotfile << "}\n";
        dotfile.close();
        system("dot -Tpng ./minDfa.dot -o ./minDfa.png");
    }
};

int main() {
    Re2NFA *re2nfa = new Re2NFA("letter(letter|digit)*", {"letter", "digit"});
    NFA2DFA *nfa2dfa = new NFA2DFA(re2nfa);
    minDFA *mDFA = new minDFA(nfa2dfa);
    mDFA->minimize();
    mDFA->generate_minDFA_png();
    return 0;
}