// bplus.cpp - in-memory B+ tree. Build: g++ -O2 -o bplus bplus.cpp   Run: ./bplus init 2 < test.txt
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace std;

int D;

struct Node {
    bool leaf;
    vector<int> keys;
    vector<int> ptrs;
    vector<Node*> kids;
    Node* next;
    Node(bool l) : leaf(l), next(NULL) {}
};

Node* root = NULL;
bool dupFound = false;

int childIndex(Node* n, int key) {
    return upper_bound(n->keys.begin(), n->keys.end(), key) - n->keys.begin();
}

Node* findLeaf(int key) {
    Node* n = root;
    while (!n->leaf) n = n->kids[childIndex(n, key)];
    return n;
}

int search(int key) {
    Node* n = findLeaf(key);
    vector<int>::iterator it = lower_bound(n->keys.begin(), n->keys.end(), key);
    if (it != n->keys.end() && *it == key) {
        int p = n->ptrs[it - n->keys.begin()];
        printf("%d found, point is %d\n", key, p);
        return p;
    }
    printf("%d not found\n", key);
    return -1;
}

bool ins(Node* n, int k, int p, int& up, Node*& right) {
    if (n->leaf) {
        int i = lower_bound(n->keys.begin(), n->keys.end(), k) - n->keys.begin();
        if (i < (int)n->keys.size() && n->keys[i] == k) { dupFound = true; return false; }
        n->keys.insert(n->keys.begin() + i, k);
        n->ptrs.insert(n->ptrs.begin() + i, p);
        if ((int)n->keys.size() <= 2 * D) return false;
        right = new Node(true);
        int mid = (n->keys.size() + 1) / 2;
        right->keys.assign(n->keys.begin() + mid, n->keys.end());
        right->ptrs.assign(n->ptrs.begin() + mid, n->ptrs.end());
        n->keys.resize(mid);
        n->ptrs.resize(mid);
        right->next = n->next;
        n->next = right;
        up = right->keys[0];
        return true;
    }
    int i = childIndex(n, k), u;
    Node* r;
    if (!ins(n->kids[i], k, p, u, r)) return false;
    n->keys.insert(n->keys.begin() + i, u);
    n->kids.insert(n->kids.begin() + i + 1, r);
    if ((int)n->keys.size() <= 2 * D) return false;
    right = new Node(false);
    int mid = n->keys.size() / 2;
    up = n->keys[mid];
    right->keys.assign(n->keys.begin() + mid + 1, n->keys.end());
    right->kids.assign(n->kids.begin() + mid + 1, n->kids.end());
    n->keys.resize(mid);
    n->kids.resize(mid + 1);
    return true;
}

bool insert(int k, int p) {
    int up;
    Node* r;
    dupFound = false;
    bool split = ins(root, k, p, up, r);
    if (dupFound) {
        printf("(%d, %d) not inserted. %d found.\n", k, p, k);
        return false;
    }
    if (split) {
        Node* nr = new Node(false);
        nr->keys.push_back(up);
        nr->kids.push_back(root);
        nr->kids.push_back(r);
        root = nr;
    }
    printf("(%d, %d) inserted\n", k, p);
    return true;
}

void mergeNodes(Node* P, Node* A, Node* B, int sep) {
    if (!A->leaf) A->keys.push_back(P->keys[sep]);
    A->keys.insert(A->keys.end(), B->keys.begin(), B->keys.end());
    A->ptrs.insert(A->ptrs.end(), B->ptrs.begin(), B->ptrs.end());
    A->kids.insert(A->kids.end(), B->kids.begin(), B->kids.end());
    A->next = B->next;
    P->keys.erase(P->keys.begin() + sep);
    P->kids.erase(P->kids.begin() + sep + 1);
    delete B;
}

void fixChild(Node* P, int i) {
    Node* C = P->kids[i];
    if ((int)C->keys.size() >= D) return;
    if (i > 0) {
        Node* L = P->kids[i - 1];
        if ((int)L->keys.size() > D) {
            if (C->leaf) {
                C->keys.insert(C->keys.begin(), L->keys.back());
                C->ptrs.insert(C->ptrs.begin(), L->ptrs.back());
                L->ptrs.pop_back();
                P->keys[i - 1] = C->keys[0];
            } else {
                C->keys.insert(C->keys.begin(), P->keys[i - 1]);
                C->kids.insert(C->kids.begin(), L->kids.back());
                L->kids.pop_back();
                P->keys[i - 1] = L->keys.back();
            }
            L->keys.pop_back();
        } else mergeNodes(P, L, C, i - 1);
    } else {
        Node* R = P->kids[1];
        if ((int)R->keys.size() > D) {
            if (C->leaf) {
                C->keys.push_back(R->keys[0]);
                C->ptrs.push_back(R->ptrs[0]);
                R->ptrs.erase(R->ptrs.begin());
                R->keys.erase(R->keys.begin());
                P->keys[0] = R->keys[0];
            } else {
                C->keys.push_back(P->keys[0]);
                C->kids.push_back(R->kids[0]);
                R->kids.erase(R->kids.begin());
                P->keys[0] = R->keys[0];
                R->keys.erase(R->keys.begin());
            }
        } else mergeNodes(P, C, R, 0);
    }
}

bool del(Node* n, int k) {
    if (n->leaf) {
        vector<int>::iterator it = lower_bound(n->keys.begin(), n->keys.end(), k);
        if (it == n->keys.end() || *it != k) return false;
        int i = it - n->keys.begin();
        n->keys.erase(n->keys.begin() + i);
        n->ptrs.erase(n->ptrs.begin() + i);
        return true;
    }
    int i = childIndex(n, k);
    if (!del(n->kids[i], k)) return false;
    fixChild(n, i);
    return true;
}

bool remove(int k) {
    if (!del(root, k)) {
        printf("%d not found, not deleted.\n", k);
        return false;
    }
    if (!root->leaf && root->keys.empty()) {
        Node* old = root;
        root = root->kids[0];
        delete old;
    }
    printf("%d deleted.\n", k);
    return true;
}

void rangeSearch(int k1, int k2) {
    vector<pair<int, int> > res;
    Node* n = findLeaf(k1);
    bool done = false;
    while (n && !done) {
        for (size_t i = 0; i < n->keys.size(); i++) {
            if (n->keys[i] > k2) { done = true; break; }
            if (n->keys[i] >= k1) res.push_back(make_pair(n->keys[i], n->ptrs[i]));
        }
        n = n->next;
    }
    if (res.empty()) {
        printf("no records in the range [%d, %d]\n", k1, k2);
        return;
    }
    printf("found\n");
    for (size_t i = 0; i < res.size(); i++)
        printf("(%d, %d)\n", res[i].first, res[i].second);
}

void traverse(bool show, int& height, int& nodes, int& keys) {
    height = nodes = keys = 0;
    vector<Node*> cur(1, root);
    while (!cur.empty()) {
        if (show) printf("Level %d: ", height);
        vector<Node*> nxt;
        for (size_t c = 0; c < cur.size(); c++) {
            Node* n = cur[c];
            nodes++;
            if (show) printf("[");
            for (size_t i = 0; i < n->keys.size(); i++) {
                if (show) {
                    if (n->leaf) printf("(%d: %d)", n->keys[i], n->ptrs[i]);
                    else printf("(%d)", n->keys[i]);
                    if (i + 1 < n->keys.size()) printf(" ");
                }
                if (n->leaf) keys++;
            }
            if (show) printf("]%s", c + 1 < cur.size() ? " " : "");
            nxt.insert(nxt.end(), n->kids.begin(), n->kids.end());
        }
        if (show) printf("\n");
        cur = nxt;
        height++;
    }
}

void printTree() { int h, n, k; traverse(true, h, n, k); }

void printStatistics() {
    int h, n, k;
    traverse(false, h, n, k);
    printf("Tree Height: %d\nTotal Nodes: %d\nTotal Keys: %d\n", h, n, k);
}

int main(int argc, char** argv) {
    if (argc != 3 || string(argv[1]) != "init" || atoi(argv[2]) < 1) {
        fprintf(stderr, "usage: ./bplus init <d> < test.txt\n");
        return 1;
    }
    D = atoi(argv[2]);
    root = new Node(true);

    string line;
    while (getline(cin, line)) {
        istringstream ss(line);
        string cmd, arg;
        if (!(ss >> cmd)) continue;
        for (size_t i = 0; i < cmd.size(); i++) cmd[i] = toupper(cmd[i]);
        int a, b;
        if (cmd == "INSERT" && ss >> a >> b) insert(a, b);
        else if (cmd == "SEARCH" && ss >> a) search(a);
        else if (cmd == "DELETE" && ss >> a) remove(a);
        else if (cmd == "RANGESEARCH" && ss >> a >> b) rangeSearch(a, b);
        else if (cmd == "PRINT") {
            ss >> arg;
            for (size_t i = 0; i < arg.size(); i++) arg[i] = toupper(arg[i]);
            if (arg == "STATISTICS") printStatistics(); else printTree();
        }
    }
    return 0;
}