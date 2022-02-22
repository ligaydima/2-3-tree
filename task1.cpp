#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

template<class ValueType>
class Set {
private:
    size_t size_;

    template<class T>
    class Tree {
    public:
        Tree() : root_(nullptr) {}

        class Node {
        public:
            Node* parent;
            vector<Node*> children;

            T min_key;
            T max_key;

            Node(const vector<Node*>& children, Node* parent) : parent(parent), children(children) {
                sort_children();
                max_key = children.back()->max_key;
                min_key = children[0]->min_key;
            }

            Node(const T& x, Node* parent) : parent(parent), children({}), min_key(x), max_key(x) {}

            Node(const Node* x) : parent(x->parent), children(x->children), min_key(x->min_key), max_key(x->max_key) {}

            void sort_children() {
                sort(children.begin(), children.end(),
                     [](const Node* a, const Node* b) { return a->max_key < b->max_key; });
            }

            void recalc() {
                if (children.empty()) {
                    return;
                }
                max_key = children.back()->max_key;
                min_key = children[0]->min_key;
            }

            void delete_child(Node* v) {
                for (auto it = children.begin(); it != children.end(); ++it) {
                    if ((*it) == v) {
                        children.erase(it);
                        break;
                    }
                }
                recalc();
            }

            size_t child_pos(Node* v) {
                for (size_t i = 0; i < children.size(); ++i) {
                    if (children[i] == v) {
                        return i;
                    }
                }
                return 0;
            }

            void add_child(Node* v) {
                v->parent = this;
                children.push_back(v);
                sort_children();
                recalc();
            }
        };

        class Iterator {
        public:
            Iterator() : node_(nullptr), root_(nullptr) {}

            Iterator(const Node* node, const Node* root) : node_(node), root_(root) {
            }

            const T& operator*() const {
                return node_->max_key;
            }

            const T* operator->() const {
                return &(node_->max_key);
            }

            bool operator==(const Iterator& ot) const {
                return node_ == ot.node_;
            }

            bool operator!=(const Iterator& ot) const {
                return node_ != ot.node_;
            }

            Iterator& operator++() {
                increment();
                return *this;
            }

            Iterator operator++(int) {
                increment();
                return *this;
            }

            Iterator& operator--() {
                decrement();
                return *this;
            }

            Iterator operator--(int) {
                decrement();
                return *this;
            }

        private:
            void increment() {
                if (node_ == nullptr) return;
                T cur_key = node_->max_key;
                while (node_ != nullptr && !(cur_key < node_->max_key)) {
                    node_ = node_->parent;
                }
                if (node_ == nullptr) {
                    return;
                }
                for (const Node* i: node_->children) {
                    if (cur_key < i->max_key) {
                        node_ = i;
                        break;
                    }
                }
                while (!node_->children.empty()) {
                    node_ = node_->children[0];
                }
            }

            void decrement() {
                if (node_ == nullptr) {
                    node_ = root_;
                    while (node_ != nullptr && !node_->children.empty()) {
                        node_ = node_->children.back();
                    }
                    return;
                };
                T cur_key = node_->min_key;
                while (node_ != nullptr && !(node_->min_key < cur_key)) {
                    node_ = node_->parent;
                }
                if (node_ == nullptr) {
                    return;
                }
                for (auto it = node_->children.rbegin(); it != node_->children.rend(); ++it) {
                    if ((*it)->min_key < cur_key) {
                        node_ = *it;
                        break;
                    }
                }
                while (!node_->children.empty()) {
                    node_ = node_->children.back();
                }
            }

            const Node* node_;
            const Node* root_;
        };

        bool operator==(const Tree& other) const {
            return root_ == other.root_;
        }

        Iterator begin() const {
            Node* cur = root_;
            if (cur == nullptr) return {root_, root_};
            while (cur->children.size() > 0) {
                cur = cur->children[0];
            }
            return Iterator(cur, root_);
        }

        Iterator end() const {
            return Iterator(nullptr, root_);
        }

        bool insert(const T& x) {
            if (root_ == nullptr) {
                root_ = new Node(x, nullptr);
                return true;
            }
            Node* cur = descend(x);
            if (!(cur->min_key < x) && !(x < cur->min_key)) {
                return false;
            }
            if (cur->parent == nullptr) {
                Node* new_root = new Node(vector<Node*>{cur}, nullptr);
                root_->parent = new_root;
                root_ = new_root;
                root_->add_child(new Node(x, root_));
                return true;
            }
            cur = cur->parent;
            Node* new_node = new Node(x, cur);
            cur->add_child(new_node);
            recalc_on_path(new_node);
            split_parent(cur);
            return true;
        }

        bool erase(const T& x) {
            if (root_ == nullptr) return false;
            Node* cur = descend(x);
            if (cur->min_key < x || x < cur->min_key) return false;
            Node* to_delete = cur;
            cur = cur->parent;
            if (cur == nullptr) {
                delete root_;
                root_ = nullptr;
                return true;
            }
            cur->delete_child(to_delete);
            recalc_on_path(to_delete);
            delete to_delete;
            while (cur->children.size() == 1) {
                if (cur->parent == nullptr) {
                    Node* new_root = cur->children[0];
                    new_root->parent = nullptr;
                    delete root_;
                    root_ = new_root;
                    break;
                }
                size_t cur_pos = cur->parent->child_pos(cur);
                size_t neig_pos = 0;
                if (cur_pos != 1) neig_pos = 1;
                Node* neig_node = cur->parent->children[neig_pos];
                neig_node->add_child(cur->children[0]);
                cur->parent->delete_child(cur);
                split_parent(neig_node);
                Node* hanging = cur;
                cur = cur->parent;
                delete hanging;
            }
            return true;
        }

        Iterator lower_bound(const T& x) const {
            Node* res = descend(x);
            if (res == nullptr || res->min_key < x) return end();
            return {res, root_};
        }

        Iterator find(const T& x) const {
            Iterator res = lower_bound(x);
            if (res == end() || x < (*res)) return end();
            return res;
        }

        ~Tree() {
            dfs_delete(root_);
        }

        void clear_tree() {
            dfs_delete(root_);
            root_ = nullptr;
        }

        void deep_copy(const Tree& other) {
            if (other.root_ == nullptr) {
                root_ = nullptr;
                return;
            }
            root_ = new Node(other.root_);
            dfs_copy(root_);
        }

    private:
        Node* root_;

        void dfs_copy(Node* v) {
            if (v == nullptr) return;
            for (size_t i = 0; i < v->children.size(); ++i) {
                v->children[i] = new Node(v->children[i]);
                v->children[i]->parent = v;
                dfs_copy(v->children[i]);
            }
        }

        void dfs_delete(Node* v) {
            if (v == nullptr) return;
            for (Node* x: v->children) {
                dfs_delete(x);
            }
            delete v;
        }

        Node* descend(const T& x) const {
            if (root_ == nullptr) return root_;
            Node* cur = root_;
            while (!cur->children.empty()) {
                bool fl = false;
                for (Node* i: cur->children) {
                    if (!(i->max_key < x)) {
                        fl = true;
                        cur = i;
                        break;
                    }
                }
                if (!fl) {
                    cur = cur->children.back();
                }
            }
            return cur;
        }

        void recalc_on_path(Node* v) {
            if (v == nullptr) return;
            v = v->parent;
            while (v != nullptr) {
                v->recalc();
                v = v->parent;
            }
        }

        void split_parent(Node* v) {
            while (v != nullptr && v->children.size() > 3) {
                Node* a = new Node(vector<Node*>{v->children[2], v->children[3]}, v->parent);
                v->children[2]->parent = a;
                v->children[3]->parent = a;
                v->children.pop_back();
                v->children.pop_back();
                v->recalc();
                if (v->parent == nullptr) {
                    Node* new_root = new Node(vector<Node*>{root_, a}, nullptr);
                    root_->parent = new_root;
                    a->parent = new_root;
                    root_ = new_root;
                    break;
                }
                v->parent->add_child(a);
                v = v->parent;
            }
        }

    };

    Tree<ValueType> tree_;
public:
    Set() : size_(0), tree_() {}

    template<typename Iterator>
    Set(Iterator first, Iterator last) : size_(0), tree_() {
        for (Iterator it = first; it != last; ++it) insert(*it);
    }

    Set(std::initializer_list<ValueType> elems) : size_(0), tree_() {
        size_ = 0;
        for (auto it = elems.begin(); it != elems.end(); ++it) insert(*it);
    }

    Set(const Set<ValueType>& other) {
        size_ = other.size_;
        tree_.deep_copy(other.tree_);
    }

    Set<ValueType>& operator=(const Set<ValueType>& other) {
        if (tree_ == other.tree_) return *this;
        tree_.clear_tree();
        tree_.deep_copy(other.tree_);
        size_ = other.size_;
        return *this;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    void insert(const ValueType& elem) {
        if (tree_.insert(elem)) ++size_;
    }

    void erase(const ValueType& elem) {
        if (tree_.erase(elem)) --size_;
    }

    typedef typename Tree<ValueType>::Iterator iterator;

    iterator begin() const {
        return tree_.begin();
    }

    iterator end() const {
        return tree_.end();
    }

    iterator find(const ValueType& elem) const {
        return tree_.find(elem);
    }

    iterator lower_bound(const ValueType& elem) const {
        return tree_.lower_bound(elem);
    }
};
