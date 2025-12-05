#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>

using namespace std;

map<char, int> mp;

vector<string> Keywords = { "let", "fun", "if", "else", "for", "while", "do", "return", "delete", "int", "char", "bool", "void", "float", "true", "false", "break", "continue" };
vector<string> Operators = { "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "=", "*=", "/=", "%=", "+=", "-=", ",", "(", ")", ".", "[", "]", "{", "}" };

const int SIZE_SYMBOLS = 93;

class Trie {
public:
    Trie() {
        root = new Node;
    }
    ~Trie() {
        clear(root);
    }
    struct Node {
        int type = 0;
        Node* to[SIZE_SYMBOLS] = {};
    };
    int get_type(const string& s) {
        Node* now = root;
        for (char c : s) {
            if (!mp.count(c)) return 0;
            int ind = mp[c];
            if (now->to[ind] == nullptr) {
                return 0;
            }
            now = now->to[ind];
        }
        return now->type;
    }
    void add(const string& s, int type) {
        Node* now = root;
        for (char c : s) {
            if (!mp.count(c)) return;
            int ind = mp[c];
            if (now->to[ind] == nullptr) {
                now->to[ind] = new Node;
            }
            now = now->to[ind];
        }
        now->type = type;
    }
private:
    Node* root;
    void clear(Node* node) {
        if (node == nullptr) return;
        for (int i = 0; i < SIZE_SYMBOLS; ++i) {
            clear(node->to[i]);
        }
        delete node;
    }
};

Trie trie;

void build_symbols() {
    int index = 1;
    for (int i = 0; i < 26; ++i) mp['a' + i] = index++;
    for (int i = 0; i < 26; ++i) mp['A' + i] = index++;
    for (int i = 0; i < 10; ++i) mp['0' + i] = index++;
    string special = "+-*/%=!<>&|,(){}[];:\"'.";
    for (char c : special) mp[c] = index++;
}

bool isalpha(char c) {
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

bool isalnum(char c) {
    return isalpha(c) || '0' <= c && c <= '9';
}

void build_trie(Trie& trie) {
    for (const string& s : Keywords) trie.add(s, 1);
    for (const string& s : Operators) trie.add(s, 4);
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

bool is_operator_char(char c) {
    return string("+-*/%=!<>&|,().[]{}").find(c) != string::npos;
}


vector<pair<string, int>> tokenize(const string& s) {
    vector<pair<string, int>> tokens;
    int i = 0, n = s.length();
    while (i < n) {
        if (is_whitespace(s[i])) {
            i++;
            continue;
        }
        if (s[i] == '"') {
            string str = "\"";
            i++;
            while (i < n && s[i] != '"') {
                str += s[i++];
            }
            if (i < n) {
                str += s[i++];
            }
            tokens.push_back({ str, 2 });
            continue;
        }
        if (isdigit(s[i]) || (s[i] == '.' && i + 1 < n && isdigit(s[i + 1]))) {
            string num;
            bool has_dot = false;
            if (s[i] == '.') {
                num += s[i++];
                has_dot = true;
            }
            while (i < n && isdigit(s[i])) {
                num += s[i++];
            }
            if (!has_dot && i < n && s[i] == '.') {
                num += s[i++];
                while (i < n && isdigit(s[i])) {
                    num += s[i++];
                }
            }
            tokens.push_back({ num, 2 });
            continue;
        }
        if (is_identifier_start(s[i])) {
            string ident;
            while (i < n && is_identifier_char(s[i])) {
                ident += s[i++];
            }
            int type = trie.get_type(ident);
            if (type == 1) {
                tokens.push_back({ ident, 1 });
            } else {
                tokens.push_back({ ident, 3 });
            }
            continue;
        }
        if (is_operator_char(s[i])) {
            string op;
            op += s[i++];
            if (i < n && is_operator_char(s[i])) {
                string potential_op = op + s[i];
                if (trie.get_type(potential_op) == 4) {
                    tokens.push_back({ potential_op, 4 });
                    i++;
                    continue;
                }
            }
            if (trie.get_type(op) == 4) {
                tokens.push_back({ op, 4 });
            } else {
                tokens.push_back({ op, 5 });
            }
            continue;
        }
        tokens.push_back({ string(1, s[i]), 5 });
        i++;
    }
    return tokens;
}

int main() {
    ifstream Fin("input.txt");

    build_symbols();
    build_trie(trie);

    vector<pair<string, int>> all_tokens;
    string input;

    while (getline(Fin, input)) {
        vector<pair<string, int>> tokens = tokenize(input);
        all_tokens.insert(all_tokens.end(), tokens.begin(), tokens.end());
    }

    cout << "\nTokens:\n";

    for (const auto& token : all_tokens) {
        string type_name;
        if (token.second == 1) {
            type_name = "KEYWORD";
        } else if (token.second == 2) {
            type_name = "LITERAL";
        } else if (token.second == 3) {
            type_name = "IDENTIFIER";
        } else if (token.second == 4) {
            type_name = "OPERATOR";
        } else if (token.second == 5) {
            type_name = "OTHER";
        } else {
            type_name = "CHO ITO??";
        }

        cout << "[" << token.first << "] : " << type_name << " (" << token.second << ")" << '\n';
    }

    Fin.close();
    return 0;
}