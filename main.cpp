#include <cstdint> 
#include <iomanip> 
#include <iostream> 
#include <optional> 
#include <sstream> 
#include <stdexcept> 
#include <string> 
#include <utility> 
#include <vector> 
 
// This educational hash is intentionally provided by the assignment. 
// It is not suitable for storing real passwords. uint64_t demoHash(const std::string& s) {     uint64_t h = 1469598103934665603ULL; 
    for (unsigned char c : s) {         h = (h ^ c) * 1099511628211ULL; 
    }     return h; 
} 
 
enum class CollisionStrategy { 
    Chaining, 
    LinearProbing, 
    QuadraticProbing, 
    DoubleHashing 
}; 
 
std::string strategyName(CollisionStrategy strategy) {     switch (strategy) {         case CollisionStrategy::Chaining: return "separate chaining";         case CollisionStrategy::LinearProbing: return "linear probing";         case CollisionStrategy::QuadraticProbing: return "quadratic probing";         case CollisionStrategy::DoubleHashing: return "double hashing"; 
    } 
    return "unknown"; 
} 
 
template <typename Value> class HashTable { private: 
    struct Entry {         std::string key; 
        Value value; 
    }; 
 
    enum class SlotState { Empty, Occupied, Deleted }; 
 
    struct Slot { 
        SlotState state = SlotState::Empty;         std::optional<Entry> entry; 
    }; 
 
    CollisionStrategy strategy_; 
    std::vector<std::vector<Entry>> chains_;     std::vector<Slot> slots_;     std::size_t itemCount_ = 0;     std::size_t minimumCapacity_; 
 
    static bool isPrime(std::size_t n) {         if (n < 2) return false;         for (std::size_t d = 2; d * d <= n; ++d) {             if (n % d == 0) return false; 
        }         return true; 
    } 
 
    static std::size_t nextPrime(std::size_t n) {         while (!isPrime(n)) ++n;         return n; 
    } 
 
    std::size_t indexFor(const std::string& key) const {         return static_cast<std::size_t>(demoHash(key) % capacity()); 
    } 
 
    std::size_t secondStep(const std::string& key) const { 
        // A nonzero step is required for double hashing. 
        return 1 + static_cast<std::size_t>(demoHash("step:" + key) % (capacity() - 1)); 
    } 
 
    std::size_t probeIndex(const std::string& key, std::size_t attempt) const {         const std::size_t start = indexFor(key);         switch (strategy_) {             case CollisionStrategy::LinearProbing:                 return (start + attempt) % capacity();             case CollisionStrategy::QuadraticProbing: 
                return (start + attempt * attempt) % capacity();             case CollisionStrategy::DoubleHashing: 
                return (start + attempt * secondStep(key)) % capacity();             case CollisionStrategy::Chaining: 
                return start; 
        }         return start; 
    } 
 
    bool shouldResize() const {         if (strategy_ == CollisionStrategy::Chaining) {             return loadFactor() > 0.75; 
        } 
        return loadFactor() > 0.50; 
    } 
 
    void insertWithoutResize(const std::string& key, const Value& value) {         if (strategy_ == CollisionStrategy::Chaining) {             auto& bucket = chains_[indexFor(key)];             for (auto& entry : bucket) {                 if (entry.key == key) { 
                    entry.value = value;                     return; 
                } 
            } 
            bucket.push_back({key, value}); 
            ++itemCount_; 
            return; 
        } 
 
        for (std::size_t attempt = 0; attempt < capacity(); ++attempt) {             Slot& slot = slots_[probeIndex(key, attempt)];             if (slot.state != SlotState::Occupied) {                 slot.entry = Entry{key, value};                 slot.state = SlotState::Occupied; 
                ++itemCount_;                 return; 
            } 
            if (slot.entry->key == key) {                 slot.entry->value = value;                 return; 
            } 
        } 
        throw std::runtime_error("hash table has no available slot"); 
    } 
 
    void rehash(std::size_t requestedCapacity) {         const std::size_t newCapacity = nextPrime(requestedCapacity);         std::vector<Entry> oldEntries; 
 
        if (strategy_ == CollisionStrategy::Chaining) {             for (const auto& bucket : chains_) {                 for (const auto& entry : bucket) oldEntries.push_back(entry); 
            } 
            chains_.assign(newCapacity, {}); 
        } else {             for (const auto& slot : slots_) {                 if (slot.state == SlotState::Occupied) oldEntries.push_back(*slot.entry); 
            } 
            slots_.assign(newCapacity, {}); 
        } 
 
        itemCount_ = 0;         for (const auto& entry : oldEntries) {             insertWithoutResize(entry.key, entry.value); 
        } 
    } 
 
public: 
    explicit HashTable(CollisionStrategy strategy = CollisionStrategy::Chaining, 
                       std::size_t initialCapacity = 11) 
        : strategy_(strategy), minimumCapacity_(nextPrime(initialCapacity)) {         if (strategy_ == CollisionStrategy::Chaining) {             chains_.assign(minimumCapacity_, {}); 
        } else { 
            slots_.assign(minimumCapacity_, {}); 
        } 
    } 
 
    std::size_t size() const { return itemCount_; }     std::size_t capacity() const {         return strategy_ == CollisionStrategy::Chaining ? chains_.size() : slots_.size(); 
    } 
    double loadFactor() const {         return static_cast<double>(itemCount_) / static_cast<double>(capacity()); 
    } 
 
    void insert(const std::string& key, const Value& value) {         // Replacing an existing key does not increase the table's size.         if (find(key) != nullptr) {             insertWithoutResize(key, value); 
            return; 
        } 
        if ((static_cast<double>(itemCount_ + 1) / capacity()) >             (strategy_ == CollisionStrategy::Chaining ? 0.75 : 0.50)) {             std::cout << "  Rehash event: " << strategyName(strategy_) 
                      << " table grows from " << capacity();             rehash(capacity() * 2 + 1);             std::cout << " to " << capacity() << " buckets.\n"; 
        } 
        insertWithoutResize(key, value); 
    } 
    Value* find(const std::string& key) {         if (strategy_ == CollisionStrategy::Chaining) {             for (auto& entry : chains_[indexFor(key)]) {                 if (entry.key == key) return &entry.value; 
            }             return nullptr; 
        } 
 
        for (std::size_t attempt = 0; attempt < capacity(); ++attempt) {             Slot& slot = slots_[probeIndex(key, attempt)];             if (slot.state == SlotState::Empty) return nullptr;             if (slot.state == SlotState::Occupied && slot.entry->key == key) {                 return &slot.entry->value; 
            }         }         return nullptr; 
    } 
 
    const Value* find(const std::string& key) const {         return const_cast<HashTable*>(this)->find(key); 
    } 
 
    bool remove(const std::string& key) {         if (strategy_ == CollisionStrategy::Chaining) {             auto& bucket = chains_[indexFor(key)];             for (auto it = bucket.begin(); it != bucket.end(); ++it) {                 if (it->key == key) {                     bucket.erase(it);                     --itemCount_;                     return true; 
                }             }             return false; 
        } 
 
        for (std::size_t attempt = 0; attempt < capacity(); ++attempt) {             Slot& slot = slots_[probeIndex(key, attempt)];             if (slot.state == SlotState::Empty) return false;             if (slot.state == SlotState::Occupied && slot.entry->key == key) { 
                slot.entry.reset();                 slot.state = SlotState::Deleted; // tombstone preserves probe sequences 
                --itemCount_;                 return true; 
            }         }         return false; 
    } 
}; 
 
struct UserRecord { 
    std::string salt;     uint64_t passwordHash; 
}; 
const std::string PEPPER = "classroom-pepper-only"; 
 
uint64_t passwordHash(const std::string& salt, const std::string& password) {     return demoHash(salt + password + PEPPER); 
} 
 
std::string hex(uint64_t value) {     std::ostringstream out;     out << std::hex << std::setw(16) << std::setfill('0') << value;     return out.str(); 
} 
 
UserRecord makeUser(const std::string& username, const std::string& password) { 
    // A deterministic salt makes this classroom demonstration reproducible. 
    const std::string salt = "salt-for-" + username;     return {salt, passwordHash(salt, password)}; 
} 
 
bool login(const HashTable<UserRecord>& users, 
           const std::string& username,            const std::string& submittedPassword) {     const UserRecord* user = users.find(username);     if (user == nullptr) {         std::cout << "Login failed for " << username << ": unknown user.\n"; 
        return false; 
    } 
    const bool correct = passwordHash(user->salt, submittedPassword) == user->passwordHash;     std::cout << (correct ? "Login successful for " : "Login failed for ") << username;     if (!correct) std::cout << ": incorrect password";     std::cout << ".\n";     return correct; 
} 
 
void demonstrateStrategy(CollisionStrategy strategy) {     HashTable<int> table(strategy, 5);     table.insert("alpha", 1);     table.insert("bravo", 2);     table.insert("charlie", 3);     std::cout << strategyName(strategy) << ": find(\"bravo\") = " 
              << *table.find("bravo") << ", load factor = " 
              << std::fixed << std::setprecision(2) << table.loadFactor() << "\n";     table.remove("bravo");     std::cout << "  remove(\"bravo\") -> " 
              << (table.find("bravo") == nullptr ? "removed" : "still present") << "\n"; } 
 
int main() {     std::cout << "Collision strategy demonstrations\n";     demonstrateStrategy(CollisionStrategy::Chaining);     demonstrateStrategy(CollisionStrategy::LinearProbing);     demonstrateStrategy(CollisionStrategy::QuadraticProbing);     demonstrateStrategy(CollisionStrategy::DoubleHashing);     std::cout << "\nAuthentication demo using the selected strategy: separate chaining\n";     HashTable<UserRecord> users(CollisionStrategy::Chaining, 5);     users.insert("alice", makeUser("alice", "apple123"));     users.insert("brandon", makeUser("brandon", "bluebird"));     users.insert("carla", makeUser("carla", "coffeeBean"));     users.insert("diego", makeUser("diego", "dragon42"));     users.insert("emma", makeUser("emma", "eagle!"));     users.insert("frank", makeUser("frank", "forest7")); // causes automatic rehash 
 
    std::cout << "Stored " << users.size() << " users; load factor = " 
              << std::fixed << std::setprecision(2) << users.loadFactor() << "\n";     std::cout << "alice's stored salt: " << users.find("alice")->salt << "\n"; 
    std::cout << "alice's stored demo hash: " << hex(users.find("alice")->passwordHash) << "\n\n"; 
 
    login(users, "alice", "apple123");     login(users, "alice", "wrong-password");     login(users, "unknown", "apple123"); 
 
    std::cout << "\nRemoving carla: " << (users.remove("carla") ? "removed" : "not found") << 
"\n";     std::cout << "Lookup after removal: " 
              << (users.find("carla") == nullptr ? "carla is no longer present" : "carla remains") << "\n"; 
 
    std::cout << "\nFinal table size: " << users.size() 
              << ", final load factor: " << users.loadFactor() << "\n"; 
    return 0; 
} 
