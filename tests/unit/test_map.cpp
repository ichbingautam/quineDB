#include "storage/shard.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace quine::storage;

// --- HashMap Tests (Indirectly via Shard or Direct if exposing headers) ---
// Since HashMap is in public header, we can test it directly.
#include "storage/hash_map.hpp"

TEST(HashMapTest, BasicPutGet) {
  HashMap map(16);
  Value v1 = "value1";
  EXPECT_TRUE(map.put("key1", v1));

  Value *res = map.get("key1");
  ASSERT_NE(res, nullptr);
  EXPECT_EQ(std::get<std::string>(*res), "value1");

  Value v2 = "value2";
  EXPECT_FALSE(map.put("key1", v2)); // Update
  res = map.get("key1");
  EXPECT_EQ(std::get<std::string>(*res), "value2");
}

TEST(HashMapTest, Delete) {
  HashMap map(16);
  map.put("key1", "val");
  EXPECT_TRUE(map.del("key1"));
  EXPECT_EQ(map.get("key1"), nullptr);
  EXPECT_FALSE(map.del("key1")); // Double delete
}

TEST(HashMapTest, CollisionHandling) {
  // Small capacity to force collisions
  HashMap map(4);
  // These keys might collide or fill up buffer
  map.put("k1", "v1");
  map.put("k2", "v2");
  map.put("k3", "v3");
  map.put("k4", "v4");

  EXPECT_EQ(std::get<std::string>(*map.get("k1")), "v1");
  EXPECT_EQ(std::get<std::string>(*map.get("k4")), "v4");

  // Full map should throw or handle gracefully (current impl throws
  // runtime_error)
  EXPECT_THROW(map.put("k5", "v5"), std::runtime_error);
}

// --- Shard Tests ---

TEST(ShardTest, SetGet) {
  Shard shard;
  shard.set("foo", "bar");
  auto *val = shard.get("foo");
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(std::get<std::string>(*val), "bar");
}

TEST(ShardTest, Expiry) {
  Shard shard;
  // Set 100ms TTL
  shard.set("temp", "val");
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  shard.set_expiry("temp", now + 100);

  // Immediate get
  ASSERT_NE(shard.get("temp"), nullptr);

  // Sleep 150ms
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  // Get should return nullptr (lazy expire)
  EXPECT_EQ(shard.get("temp"), nullptr);
}

TEST(ShardTest, UpdateClearsExpiry) {
  Shard shard;
  shard.set("persistent", "val");
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  shard.set_expiry("persistent", now + 100);
  // Update without TTL
  shard.set("persistent", "newval");

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  EXPECT_NE(shard.get("persistent"), nullptr);
}

TEST(ShardTest, SetCommands) {
  Shard shard;
  // Helper to construct set
  shard.set("myset", Set{});
  auto *val = shard.get("myset");
  auto *set_ptr = std::get_if<Set>(val);
  ASSERT_NE(set_ptr, nullptr);

  set_ptr->insert("a");
  set_ptr->insert("b");

  EXPECT_EQ(set_ptr->size(), 2);
  EXPECT_TRUE(set_ptr->contains("a"));
  EXPECT_FALSE(set_ptr->contains("c"));
}
