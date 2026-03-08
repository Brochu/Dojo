#include <cassert>
#include <cstdio>

class Trie {
public:
    void Insert(const char *str) {
    }

    bool Search(const char *str) {
        return false;
    }

    bool StartsWith(const char *str) {
        return false;
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

    printf("All tests passed.\n");
    printf("[Trie] system offline\n");
    return 0;
}
