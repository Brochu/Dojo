#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class Trie {
public:
    Trie() {
        node_pool = (Node*)calloc(nodes_max_count, sizeof(Node));
        root = &node_pool[num_nodes++];
    }

    ~Trie() {
        root = nullptr;
        num_nodes = 0;
        free(node_pool);
    }

    void Insert(const char *str) {
        const size_t len = strlen(str);
        Node *current = root;

        for (int32_t i = 0; i < len; i++) {
            int32_t index = str[i] - 'a';
            if (current->children[index] != nullptr) {
                current = current->children[index];
                continue;
            }

            assert(num_nodes < nodes_max_count);
            current->children[index] = &node_pool[num_nodes++];
            current = current->children[index];
        }

        current->is_final = true;
    }

    bool Search(const char *str) {
        Node *n = FindNode(root, str);
        return n != nullptr && n->is_final;
    }

    bool StartsWith(const char *str) {
        Node *n = FindNode(root, str);
        return n != nullptr;
    }

    bool WildcardSearch(const char *pattern) {
        return WildcardSearchInner(root, pattern);
    }

    void PrefixCollect(const char *prefix, char results[][64], int32_t max_results, int32_t &count) {
        Node *start = FindNode(root, prefix);

        if (start == nullptr || max_results == 0) {
            count = 0;
            return;
        }
        assert(results != nullptr);

        char buffer[word_max_len];
        memset(buffer, 0, sizeof(char) * word_max_len);
        strcpy_s(buffer, prefix);

        PrefixCollectInner(start, buffer, strlen(prefix), results, max_results, count);
    }

private:
    static constexpr int32_t nodes_max_count = 1024;
    static constexpr int32_t alphabet_size = 26;
    static constexpr int32_t word_max_len = 128;
    struct Node {
        bool is_final = false;
        Node *children[alphabet_size];
    };

    Node *node_pool;
    int32_t num_nodes = 0;

    Node *root = nullptr;

    Node *FindNode(Node *start, const char *str) {
        const size_t len = strlen(str);
        Node *current = start;

        for (int32_t i = 0; i < len; i++) {
            int32_t index = str[i] - 'a';
            if (current->children[index] == nullptr) {
                return nullptr;
            }

            current = current->children[index];
        }

        return current;
    }

    bool WildcardSearchInner(Node *node, const char *str) {
        if (node == nullptr) {
            return false;
        }

        const size_t len = strlen(str);
        if (str[0] == '\0') {
            return node->is_final;
        }

        int32_t index = str[0] - 'a';
        if (str[0] != '.') {
            return WildcardSearchInner(node->children[index], str+1);
        }

        bool res = false;
        for (int32_t i = 0; i < alphabet_size; i++) {
            res |= WildcardSearchInner(node->children[i], str+1);
        }
        return res;
    }

    void PrefixCollectInner(Node *node, char word[word_max_len], int32_t depth, char results[][64], int32_t max_results, int32_t &count) {
        if (node == nullptr || count >= max_results) {
            return;
        }

        if (node->is_final) {
            strcpy_s(results[count++], 64, word);
        }

        for (int32_t i = 0; i < alphabet_size; i++) {
            word[depth] = 'a' + i;
            PrefixCollectInner(node->children[i], word, depth+1, results, max_results, count);
        }
        word[depth] = '\0';
    }
};

int main()
{
    printf("[Trie] system online\n");

    bool result = false;
    // 1. Empty trie, search miss
    {
        Trie trie;
        result = trie.Search("hello");
        assert(!result);
    }

    // 2. Insert and exact search
    {
        Trie trie;
        trie.Insert("hello");
        result = trie.Search("hello");
        assert(result);
        result = trie.Search("hell");
        assert(!result);
    }

    // 3. Prefix query
    {
        Trie trie;
        trie.Insert("hello");
        result = trie.StartsWith("hel");
        assert(result);
        result = trie.StartsWith("hello");
        assert(result);
        result = trie.StartsWith("helloo");
        assert(!result);
    }

    // 4. Multiple words, shared prefix
    {
        Trie trie;
        trie.Insert("hello");
        trie.Insert("help");
        result = trie.Search("hello");
        assert(result);
        result = trie.Search("help");
        assert(result);
        result = trie.Search("hel");
        assert(!result);
        result = trie.StartsWith("hel");
        assert(result);
    }

    // 5. Word that is a prefix of another
    {
        Trie trie;
        trie.Insert("he");
        trie.Insert("hello");
        result = trie.Search("he");
        assert(result);
        result = trie.Search("hello");
        assert(result);
        result = trie.Search("hel");
        assert(!result);
    }

    // 6. Duplicate insert is a no-op
    {
        Trie trie;
        trie.Insert("hello");
        trie.Insert("hello");
        result = trie.Search("hello");
        assert(result);
    }

    // 7. Completely disjoint words
    {
        Trie trie;
        trie.Insert("abc");
        trie.Insert("xyz");
        result = trie.Search("abc");
        assert(result);
        result = trie.Search("xyz");
        assert(result);
        result = trie.StartsWith("ay");
        assert(!result);
    }

    // 8. Wildcard single character
    {
        Trie trie;
        trie.Insert("hello");
        trie.Insert("hallo");
        result = trie.WildcardSearch("h.llo");
        assert(result);
        result = trie.WildcardSearch("he.lo");
        assert(result);
        result = trie.WildcardSearch("h..lo");
        assert(result);
    }

    // 9. Wildcard no match
    {
        Trie trie;
        trie.Insert("hello");
        result = trie.WildcardSearch("h.la");
        assert(!result);
        result = trie.WildcardSearch(".....");  // right length, but no 'a' words
        assert(result);                         // "hello" is 5 chars, so this matches
    }

    // 10. Wildcard length mismatch
    {
        Trie trie;
        trie.Insert("hello");
        result = trie.WildcardSearch("h.l");
        assert(!result);
        result = trie.WildcardSearch("h.llooo");
        assert(!result);
    }

    // 11. Wildcard all dots
    {
        Trie trie;
        trie.Insert("abc");
        result = trie.WildcardSearch("...");
        assert(result);
        result = trie.WildcardSearch("..");
        assert(!result);
        result = trie.WildcardSearch("....");
        assert(!result);
    }

    // 12. Prefix collect basic
    {
        Trie trie;
        trie.Insert("hello");
        trie.Insert("help");
        trie.Insert("hero");
        trie.Insert("world");

        char results[8][64];
        int32_t count = 0;

        trie.PrefixCollect("hel", results, 8, count);
        assert(count == 2);
        count = 0;

        trie.PrefixCollect("her", results, 8, count);
        assert(count == 1);
        count = 0;

        trie.PrefixCollect("wo", results, 8, count);
        assert(count == 1);
    }

    // 13. Prefix collect, no matches
    {
        Trie trie;
        trie.Insert("hello");

        char results[8][64];
        int32_t count = 0;

        trie.PrefixCollect("xyz", results, 8, count);
        assert(count == 0);
    }

    // 14. Prefix collect, max_results caps output
    {
        Trie trie;
        trie.Insert("aa");
        trie.Insert("ab");
        trie.Insert("ac");
        trie.Insert("ad");

        char results[2][64];
        int32_t count = 0;

        trie.PrefixCollect("a", results, 2, count);
        assert(count == 2);
    }

    // 15. Prefix collect, prefix is itself a word
    {
        Trie trie;
        trie.Insert("he");
        trie.Insert("hello");
        trie.Insert("help");

        char results[8][64];
        int32_t count = 0;

        trie.PrefixCollect("he", results, 8, count);
        assert(count == 3);
    }

    // 16. Prefix collect, empty prefix returns all words
    {
        Trie trie;
        trie.Insert("abc");
        trie.Insert("xyz");

        char results[8][64];
        int32_t count = 0;

        trie.PrefixCollect("", results, 8, count);
        assert(count == 2);
    }

    printf("All tests passed.\n");
    printf("[Trie] system offline\n");
    return 0;
}
