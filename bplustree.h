#include <stdlib.h>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <bit>
#include <functional>
#include <mutex>
#include <vector>

#include <atomic>

// Define Clock and Key types
typedef std::chrono::high_resolution_clock Clock;
typedef uint64_t Key;

// Compare function for keys
int compare_(const Key& a, const Key& b) {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return +1;
    } else {
        return 0;
    }
}

// B+ Tree class template definition
template<typename Key>
class Bplustree {
   private:
    // Forward declaration of node structures
    struct Node;
    struct InternalNode;
    struct LeafNode;

   public:
    // Constructor: Initializes a B+ Tree with the specified degree (maximum number of children per internal node)
    Bplustree(int degree = 4);

    // Insert function:
    // Inserts a key into the B+ Tree.
    // TODO: Implement insertion, handling leaf node insertion and splitting if necessary.
    void Insert(const Key& key);

    // Contains function:
    // Returns true if the key exists in the tree; otherwise, returns false.
    // TODO: Implement key lookup starting from the root and traversing to the appropriate leaf.
    bool Contains(const Key& key) const;

    // Scan function:
    // Performs a range query starting from the specified key and returns up to 'scan_num' keys.
    // TODO: Traverse leaf nodes using the next pointer and collect keys.
    std::vector<Key> Scan(const Key& key, const int scan_num);

    // Delete function:
    // Removes the specified key from the tree.
    // TODO: Implement deletion, handling key removal, merging, or rebalancing nodes if required.
    bool Delete(const Key& key);

    // Print function:
    // Traverses and prints the internal structure of the B+ Tree.
    // This function is helpful for debugging and verifying that the tree is constructed correctly.
    void Print() const;

   private:
    // Base Node structure. All nodes (internal and leaf) derive from this.
    struct Node {
        bool is_leaf; // Indicates whether the node is a leaf
        // Helper functions to cast a Node pointer to InternalNode or LeafNode pointers.
        InternalNode* as_internal() { return static_cast<InternalNode*>(this); }
        LeafNode* as_leaf() { return static_cast<LeafNode*>(this); }
        virtual ~Node() = default;
    };

    // Internal node structure for the B+ Tree.
    // Stores keys and child pointers.
    struct InternalNode : public Node {
        std::vector<Key> keys;         // Keys used to direct search to the correct child
        std::vector<Node*> children;   // Pointers to child nodes
        InternalNode() { this->is_leaf = false; }
    };

    // Leaf node structure for the B+ Tree.
    // Stores actual keys and a pointer to the next leaf for efficient range queries.
    struct LeafNode : public Node {
        std::vector<Key> keys; // Keys stored in the leaf node
        LeafNode* next;        // Pointer to the next leaf node for range scanning
        LeafNode() : next(nullptr) { this->is_leaf = true; }
    };

    // Helper function to insert a key into an internal node.
    // 'new_child' and 'new_key' are output parameters if the node splits.
    // TODO: Implement insertion into an internal node and handle splitting of nodes.
    void InsertInternal(Node* current, const Key& key, Node*& new_child, Key& new_key);

    // Helper function to delete a key from the tree recursively.
    // TODO: Implement deletion from internal nodes with proper merging or rebalancing.
    bool DeleteInternal(Node* current, const Key& key);

    // Helper function to find the leaf node where the key should reside.
    // TODO: Implement traversal from the root to the appropriate leaf node.
    LeafNode* FindLeaf(const Key& key) const;

    // Helper function to recursively print the tree structure.
    void PrintRecursive(const Node* node, int level) const;

    Node* root;   // Root node of the B+ Tree
    int degree;   // Maximum number of children per internal node
};

// Constructor implementation
// Initializes the tree by creating an empty leaf node as the root.
template<typename Key>
Bplustree<Key>::Bplustree(int degree) : degree(degree) {
    root = new LeafNode();
    // To be implemented by students
}

// Insert function: Inserts a key into the B+ Tree.
template<typename Key>
void Bplustree<Key>::Insert(const Key& key) {
    Node* new_child = nullptr;
    Key new_key;

    InsertInternal(root, key, new_child, new_key);

    // Root split
    if (new_child != nullptr) {
        InternalNode* new_root = new InternalNode();
        new_root->keys.push_back(new_key);
        new_root->children.push_back(root);
        new_root->children.push_back(new_child);
        root = new_root;
    }
}


// Contains function: Checks if a key exists in the B+ Tree.
template<typename Key>
bool Bplustree<Key>::Contains(const Key& key) const {
    LeafNode* leaf = FindLeaf(key);
    return std::binary_search(leaf->keys.begin(), leaf->keys.end(), key);
}


// Scan function: Performs a range query starting from a given key.
template<typename Key>
std::vector<Key> Bplustree<Key>::Scan(const Key& key, const int scan_num) {
    std::vector<Key> result;
    LeafNode* leaf = FindLeaf(key);
    int count = 0;

    while (leaf && count < scan_num) {
        for (const Key& k : leaf->keys) {
            if (k >= key && count < scan_num) {
                result.push_back(k);
                ++count;
            }
        }
        leaf = leaf->next;
    }

    return result;
}


// Delete function: Removes a key from the B+ Tree.
template<typename Key>
bool Bplustree<Key>::Delete(const Key& key) {
    return DeleteInternal(root, key);
}


// InsertInternal function: Helper function to insert a key into an internal node.
template<typename Key>
void Bplustree<Key>::InsertInternal(Node* current, const Key& key, Node*& new_child, Key& new_key) {
    if (current->is_leaf) {
        LeafNode* leaf = current->as_leaf();
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it == leaf->keys.end() || *it != key) {
            leaf->keys.insert(it, key);
        }

        if (leaf->keys.size() >= degree) {
            // Split leaf node
            LeafNode* sibling = new LeafNode();
            int mid = degree / 2;

            sibling->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
            leaf->keys.resize(mid);

            sibling->next = leaf->next;
            leaf->next = sibling;

            new_child = sibling;
            new_key = sibling->keys.front();
        }
        else {
            new_child = nullptr;
        }
    }
    else {
        InternalNode* internal = current->as_internal();
        size_t i = 0;
        while (i < internal->keys.size() && key >= internal->keys[i])
            ++i;

        Node* child = internal->children[i];
        Node* child_new_child = nullptr;
        Key child_new_key;

        InsertInternal(child, key, child_new_child, child_new_key);

        if (child_new_child != nullptr) {
            internal->keys.insert(internal->keys.begin() + i, child_new_key);
            internal->children.insert(internal->children.begin() + i + 1, child_new_child);

            if (internal->keys.size() >= degree) {
                // Split internal node
                InternalNode* sibling = new InternalNode();
                int mid = degree / 2;

                new_key = internal->keys[mid];

                sibling->keys.assign(internal->keys.begin() + mid + 1, internal->keys.end());
                sibling->children.assign(internal->children.begin() + mid + 1, internal->children.end());

                internal->keys.resize(mid);
                internal->children.resize(mid + 1);

                new_child = sibling;
                return;
            }
        }
        new_child = nullptr;
    }
}


// DeleteInternal function: Helper function to delete a key from an internal node.
template<typename Key>
bool Bplustree<Key>::DeleteInternal(Node* current, const Key& key) {
    if (current->is_leaf) {
        LeafNode* leaf = current->as_leaf();
        auto it = std::find(leaf->keys.begin(), leaf->keys.end(), key);
        if (it != leaf->keys.end()) {
            leaf->keys.erase(it);
            return true;
        }
        return false;
    }

    InternalNode* internal = current->as_internal();
    size_t i = 0;
    while (i < internal->keys.size() && key >= internal->keys[i]) ++i;

    Node* child = internal->children[i];
    bool deleted = DeleteInternal(child, key);
    if (!deleted) return false;

    // LeafNode underflow handling
    if (child->is_leaf) {
        LeafNode* leaf = child->as_leaf();
        if (leaf->keys.size() < (degree + 1) / 2) {
            // Try to borrow or merge
            if (i > 0) {
                LeafNode* left_sibling = internal->children[i - 1]->as_leaf();
                if (left_sibling->keys.size() > (degree + 1) / 2) {
                    leaf->keys.insert(leaf->keys.begin(), left_sibling->keys.back());
                    left_sibling->keys.pop_back();
                    internal->keys[i - 1] = leaf->keys.front();
                    return true;
                }
            }

            if (i + 1 < internal->children.size()) {
                LeafNode* right_sibling = internal->children[i + 1]->as_leaf();
                if (right_sibling->keys.size() > (degree + 1) / 2) {
                    leaf->keys.push_back(right_sibling->keys.front());
                    right_sibling->keys.erase(right_sibling->keys.begin());
                    internal->keys[i] = right_sibling->keys.front();
                    return true;
                }
            }

            // Merge
            if (i > 0) {
                LeafNode* left_sibling = internal->children[i - 1]->as_leaf();
                left_sibling->keys.insert(left_sibling->keys.end(), leaf->keys.begin(), leaf->keys.end());
                left_sibling->next = leaf->next;
                delete leaf;
                internal->keys.erase(internal->keys.begin() + i - 1);
                internal->children.erase(internal->children.begin() + i);
            }
            else {
                LeafNode* right_sibling = internal->children[i + 1]->as_leaf();
                leaf->keys.insert(leaf->keys.end(), right_sibling->keys.begin(), right_sibling->keys.end());
                leaf->next = right_sibling->next;
                delete right_sibling;
                internal->keys.erase(internal->keys.begin() + i);
                internal->children.erase(internal->children.begin() + i + 1);
            }
        }
    }
    // InternalNode underflow handling
    else {
        InternalNode* child_internal = child->as_internal();
        if (child_internal->children.size() < (degree + 1) / 2) {
            if (i > 0) {
                InternalNode* left_sibling = internal->children[i - 1]->as_internal();
                if (left_sibling->children.size() > (degree + 1) / 2) {
                    child_internal->children.insert(child_internal->children.begin(), left_sibling->children.back());
                    child_internal->keys.insert(child_internal->keys.begin(), internal->keys[i - 1]);
                    internal->keys[i - 1] = left_sibling->keys.back();
                    left_sibling->keys.pop_back();
                    left_sibling->children.pop_back();
                    return true;
                }
            }

            if (i + 1 < internal->children.size()) {
                InternalNode* right_sibling = internal->children[i + 1]->as_internal();
                if (right_sibling->children.size() > (degree + 1) / 2) {
                    child_internal->children.push_back(right_sibling->children.front());
                    child_internal->keys.push_back(internal->keys[i]);
                    internal->keys[i] = right_sibling->keys.front();
                    right_sibling->keys.erase(right_sibling->keys.begin());
                    right_sibling->children.erase(right_sibling->children.begin());
                    return true;
                }
            }

            if (i > 0) {
                InternalNode* left_sibling = internal->children[i - 1]->as_internal();
                left_sibling->keys.push_back(internal->keys[i - 1]);
                left_sibling->keys.insert(left_sibling->keys.end(), child_internal->keys.begin(), child_internal->keys.end());
                left_sibling->children.insert(left_sibling->children.end(), child_internal->children.begin(), child_internal->children.end());
                delete child_internal;
                internal->keys.erase(internal->keys.begin() + i - 1);
                internal->children.erase(internal->children.begin() + i);
            }
            else {
                InternalNode* right_sibling = internal->children[i + 1]->as_internal();
                child_internal->keys.push_back(internal->keys[i]);
                child_internal->keys.insert(child_internal->keys.end(), right_sibling->keys.begin(), right_sibling->keys.end());
                child_internal->children.insert(child_internal->children.end(), right_sibling->children.begin(), right_sibling->children.end());
                delete right_sibling;
                internal->keys.erase(internal->keys.begin() + i);
                internal->children.erase(internal->children.begin() + i + 1);
            }
        }
    }

    // Shrink root if needed
    if (current == root && !current->is_leaf) {
        InternalNode* root_internal = root->as_internal();
        if (root_internal->children.size() == 1) {
            root = root_internal->children[0];
            delete root_internal;
        }
    }

    return true;
}



// FindLeaf function: Traverses the B+ Tree from the root to find the leaf node that should contain the given key.
template<typename Key>
typename Bplustree<Key>::LeafNode* Bplustree<Key>::FindLeaf(const Key& key) const {
    Node* current = root;
    while (!current->is_leaf) {
        InternalNode* internal = current->as_internal();
        size_t i = 0;
        while (i < internal->keys.size() && key >= internal->keys[i]) {
            ++i;
        }
        current = internal->children[i];
    }
    return current->as_leaf();
}

// Print function: Public interface to print the B+ Tree structure.
template<typename Key>
void Bplustree<Key>::Print() const {
    PrintRecursive(root, 0);
}

// Helper function: Recursively prints the tree structure with indentation based on tree level.
template<typename Key>
void Bplustree<Key>::PrintRecursive(const Node* node, int level) const {
    if (node == nullptr) return;
    // Indent based on the level in the tree.
    for (int i = 0; i < level; ++i)
        std::cout << "  ";
    if (node->is_leaf) {
        // Print leaf node keys.
        const LeafNode* leaf = node->as_leaf();
        std::cout << "[Leaf] ";
        for (const Key& key : leaf->keys)
            std::cout << key << " ";
        std::cout << std::endl;
    } else {
        // Print internal node keys and recursively print children. 
        const InternalNode* internal = node->as_internal();
        std::cout << "[Internal] ";
        for (const Key& key : internal->keys)
            std::cout << key << " ";
        std::cout << std::endl;
        for (const Node* child : internal->children)
            PrintRecursive(child, level + 1);
    }
}