//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"
// TODO 必须要保证是线程安全的？这里没加锁呢
namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) { cap = num_pages; }

LRUReplacer::~LRUReplacer() = default;

/**
 * 当前的实现是：如果replacer为空，那么出参为nullptr，返回false；如果不为空，那么出参为back（list中最后一个），返回true
 * 关键的一点在于：LRU是如何体现的？现在的设计是，按照加入到replacer中的顺序，新加入的放在最前面，LRU一次也就对应最后面的
 * 这样对吗？
 */
bool LRUReplacer::Victim(frame_id_t *frame_id) {
  if (frames.empty()) {
    frame_id = nullptr;
    return false;
  }
  *frame_id = frames.back();
  cnt[*frame_id] = 0;
  frames.pop_back();
  return true;
}

/**
 * 按照specification的说法，这个函数是在BPM中pin了之后才被调用
 * 但是这个说法很confusing，BPM没有pin操作啊，pin不是replacer的吗
 * 不过specification后面也说了，他的一个作用是从replacer中remove对应frame
 */
void LRUReplacer::Pin(frame_id_t frame_id) {
  if (cnt[frame_id] == 0) {
    return;
  }
  // list.remove(v),删除链表中等于v的节点，如果不存在，那么什么都不会做
  frames.remove(frame_id);
  cnt[frame_id] = 0;
}

/**
 * specification：这个函数应该在对应page的pinCnt降到0时被调用
 * （其实LRU这个的实现很简单，唯一的不确定在于LRU的依据；另外一点，是性能现在不是最优化的）
 * 一个作用是将frame添加到replacer
 * @param frame_id
 */
void LRUReplacer::Unpin(frame_id_t frame_id) {
  // 必须要判断存不存在，不要重复插入;如果已经存在，是什么都不需要做的，见测试case1
  if (cnt[frame_id]) {
    return;
  }
  cnt[frame_id] = 1;
  frames.push_front(frame_id);
}

size_t LRUReplacer::Size() { return frames.size(); }

}  // namespace bustub
