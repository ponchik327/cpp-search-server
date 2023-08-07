#include <map>
#include <vector>
#include <mutex>
#include <type_traits>
#include <cstdint>
#include <stdexcept>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) 
        : buckets_(bucket_count)
        , mutexs_(bucket_count)
        , bucket_count_(bucket_count)
    {
        if (!std::is_integral_v<Key>) {
            throw std::logic_error("ConcurrentMap supports only integer keys");
        }
    }

    Access operator[](const Key& key) {
        auto index = (uint64_t)key % bucket_count_;
        return Access{ std::lock_guard(mutexs_[index]), buckets_[index][key] };
    }

    void Erase(const Key& key) {
        auto index = (uint64_t)key % bucket_count_;
        std::lock_guard g(mutexs_[index]);
        buckets_[index].erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (uint64_t i = 0; i < bucket_count_; ++i) {
            std::lock_guard g(mutexs_[i]);
            result.insert(buckets_[i].begin(), buckets_[i].end());
        }
        return result;
    }

private:
    std::vector<std::map<Key, Value>> buckets_;
    std::vector<std::mutex> mutexs_;
    uint64_t bucket_count_;
};