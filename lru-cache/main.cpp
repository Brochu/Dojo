#include <cassert>
#include <cstdint>
#include <cstdio>
#include <list>
#include <map>
#include <mutex>

template <class K, class V, size_t N>
class LRUCacheSystem {
public:
    LRUCacheSystem() = default;

    bool Get(K key, V& val) {
        std::scoped_lock lock(mu);

        auto it = cached_values.find(key);
        if (it == cached_values.end()) {
            // Cache-Miss
            return false;
        }
        else {
            // Cache-Hit
            cached_entry &e = it->second;
            // Move value to the most recently used slot
            recency.splice(recency.end(), recency, e.it);
            val = e.value;
            return true;
        }
    }

    void Put(K key, V val) {
        std::scoped_lock lock(mu);
        auto it = cached_values.find(key);

        if (it == cached_values.end()) {
            if (recency.size() >= N) {
                // Evict LRU
                K evicted = recency.front();
                recency.pop_front();
                cached_values.erase(evicted);
            }
            cached_entry e { val };
            e.it = recency.insert(recency.end(), key);
            cached_values[key] = e;
        }
        else {
            cached_entry &e = it->second;
            // Move value to the most recently used slot
            recency.splice(recency.end(), recency, e.it);
            e.value = val;
        }
    }

private:
    struct cached_entry {
        V value;
        typename std::list<K>::iterator it;
    };

    std::map<K, cached_entry> cached_values;
    std::mutex mu;

    std::list<K> recency; // END = MRU; FRONT = LRU
};

int main()
{
    printf("[LRU] system online\n");
    float val = 0.0f;
    bool found = false;

    // 1. Empty cache miss
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        found = cache.Get(1, val);
        assert(!found);
    }

    // 2. Put then Get
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 10.0f);
        found = cache.Get(1, val);
        assert(found);
        assert(val == 10.0f);
    }

    // 3. Overwrite existing key
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 10.0f);
        cache.Put(1, 99.0f);
        found = cache.Get(1, val);
        assert(found);
        assert(val == 99.0f);
    }

    // 4. Fill to capacity, all present
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);

        found = cache.Get(1, val);
        assert(found && val == 1.0f);
        found = cache.Get(2, val);
        assert(found && val == 2.0f);
        found = cache.Get(3, val);
        assert(found && val == 3.0f);
    }

    // 5. Eviction of LRU element
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Put(4, 4.0f); // should evict key 1

        found = cache.Get(1, val);
        assert(!found);
        found = cache.Get(4, val);
        assert(found && val == 4.0f);
    }

    // 6. Get promotes element, changes eviction order
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Get(1, val);   // promote key 1, LRU is now key 2
        cache.Put(4, 4.0f);  // should evict key 2

        found = cache.Get(2, val);
        assert(!found);
        found = cache.Get(1, val);
        assert(found && val == 1.0f);
    }

    // 7. Put (update) promotes element
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Put(1, 11.0f); // update key 1, LRU is now key 2
        cache.Put(4, 4.0f);  // should evict key 2

        found = cache.Get(2, val);
        assert(!found);
        found = cache.Get(1, val);
        assert(found && val == 11.0f);
    }

    // 8. Sequential evictions
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Put(4, 4.0f); // evicts 1
        cache.Put(5, 5.0f); // evicts 2

        found = cache.Get(1, val);
        assert(!found);
        found = cache.Get(2, val);
        assert(!found);
        found = cache.Get(3, val);
        assert(found && val == 3.0f);
        found = cache.Get(4, val);
        assert(found && val == 4.0f);
        found = cache.Get(5, val);
        assert(found && val == 5.0f);
    }

    // 9. Failed Get does NOT affect eviction order
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Get(999, val);  // miss, should be a no-op
        cache.Put(4, 4.0f);  // should still evict key 1 (oldest)

        found = cache.Get(1, val);
        assert(!found);
        found = cache.Get(2, val);
        assert(found);
    }

    // 10. Reinsert after eviction
    {
        LRUCacheSystem<uint64_t, float, 3> cache;
        cache.Put(1, 1.0f);
        cache.Put(2, 2.0f);
        cache.Put(3, 3.0f);
        cache.Put(4, 4.0f); // evicts 1

        found = cache.Get(1, val);
        assert(!found);
        cache.Put(1, 100.0f); // reinsert key 1, evicts 2
        found = cache.Get(1, val);
        assert(found && val == 100.0f);
        found = cache.Get(2, val);
        assert(!found);
    }

    printf("All tests passed.\n");
    printf("[LRU] system offline\n");
    return 0;
}
