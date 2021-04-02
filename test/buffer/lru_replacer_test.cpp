//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer_test.cpp
//
// Identification: test/buffer/lru_replacer_test.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <thread>  // NOLINT
#include <vector>

#include "buffer/lru_replacer.h"
#include "gtest/gtest.h"

namespace bustub {

TEST(LRUReplacerTest, SampleTest) {
  LRUReplacer lru_replacer(7);
  //  poolsize是7，但是unpin了1~6 6个frame，所以size返回6，注意size返回的是可以evict的elem的数量
  //  实现1：unpin的时候（实际上pin的时候也一样），首先判断在不在list里，如果在，unpin不需要做任何事
  // Scenario: unpin six elements, i.e. add them to the replacer.
  lru_replacer.Unpin(1);
  lru_replacer.Unpin(2);
  lru_replacer.Unpin(3);
  lru_replacer.Unpin(4);
  lru_replacer.Unpin(5);
  lru_replacer.Unpin(6);
  lru_replacer.Unpin(1);
  EXPECT_EQ(6, lru_replacer.Size());

  //  假设最近使用的放在最前面，那么此时是 1 6 5 4 3
  //  2，根据LRU，victim应该是最后面的，为什么这里是1呢，应该是指，1已经在replacer里了，unpin对他是没有作用的
  //  实现1：victim，直接back加pop_back,为空就返回nullptr
  //  那么问题来了，lru_replacer是根据什么维护LRU的性质？按照他们被放进lru_replacer的顺序？
  // 如果只看被放进replacer的顺序，那么此时应该是6,5,4,3,2,1 所以victim是1，2，3没错,最后replacer中剩下6,5,4 Scenario:
  // get three victims from the lru.
  int value;
  lru_replacer.Victim(&value);
  EXPECT_EQ(1, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(2, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(3, value);

  //  Pin说明该page不应该被victim,是不是就是根据pin来改变lru中的顺序？当然pin(3)没有用，因为已经victim了，而pin(4)会把4放到最前面，变成4,6,5?
  //  实现1：pin是为了从replacer中移除，这里3已经不在replacer中了，所以不做任何事，4在里面，所以从里面移除
  // 但是看下面的size，变成了2，貌似意思是，一旦pin了，就从replacer中移除？不过size返回的是可以evict的数量，pin了之后本身也不可以evict了
  // 问题在于，pin之后，他还在replacer中吗？还是说这个问题不重要？
  // pin了，就说了有人在使用它，肯定不能被evict，所以不在replacer中 Scenario: pin elements in the replacer. Note that 3
  // has already been victimized, so pinning 3 should have no effect.
  lru_replacer.Pin(3);
  lru_replacer.Pin(4);
  EXPECT_EQ(2, lru_replacer.Size());

  //  刚pin了4，现在对4 unpin，相当于又加入replacer？ 这里说的ref
  // bit是指什么？设为1？到底要维护pin_cnt吗？可是为什么page类 也维护了pin_cnt呢
  // //  实现1：不知道这个ref bit是什么鬼 是需要维护一个ref cnt吗？多个pin需要几个unpin？（这个问题很关键）
  // Scenario: unpin 4. We expect that the reference bit of 4 will be set to 1.
  lru_replacer.Unpin(4);
  //  现在victim又依次是5，6，4，确实符合之前说的顺序
  // Scenario: continue looking for victims. We expect these victims.
  lru_replacer.Victim(&value);
  EXPECT_EQ(5, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(6, value);
  lru_replacer.Victim(&value);
  EXPECT_EQ(4, value);
}

}  // namespace bustub
