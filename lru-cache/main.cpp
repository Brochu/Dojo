#include <cstdio>
#include <map>
#include <mutex>

template <class K, class V, size_t N>
class LRUCacheSystem {
public:
    LRUCacheSystem() = default;

    bool Get(K key, V& val) {
        //TODO: Mutex placement
        auto pos = cached_values.find(key);
        if (pos == cached_values.end()) {
            return false;
        }
        else {
            update_lru(key);
            val = cached_values[key];
            return true;
        }
    }

    void Put(K key, V val) {
        //TODO: Mutex placement
        auto pos = cached_values.find(key);
        if (pos == cached_values.end()) {
            new_value(key, val);
        }
        else {
            cached_values[key] = val;
            update_lru(key);
        }
    }

private:
    bool exists(K key) {
        auto pos = cached_values.find(key);
    }

    void new_value(K key, V val) {
        if (tail - head >= N) {
            K evicted_key = evict_lru();
            cached_values.erase(evicted_key);
        }
        cached_values[key] = val;
        recency[head++] = key;
    }

    K evict_lru() {
        tail++;
        return {};
    }

    void update_lru(K key) {
        //TODO: Find key in recency, swap to head
        // probably with a memmove_s
    }

    //TODO: Create entry type to bundle value and index in recency / mutex arrays
    std::map<K, V> cached_values;
    //TODO: One mutex per value in the cache?
    std::mutex mu;

    K recency[N];
    size_t head;
    size_t tail;
};

int main() {
    printf("[LRU] system online\n");

    LRUCacheSystem<uint64_t, char*, 20> sys;

    printf("[LRU] system offline\n");
}
