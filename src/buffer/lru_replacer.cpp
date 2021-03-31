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

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) { cap = num_pages; }

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  if (frames.empty()) {
    frame_id = nullptr;
    return false;
  }
  *frame_id = frames.back();
  frames.pop_back();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  if (cnt[frame_id] == 0) {
    return;
  }
  // list.remove(v),删除链表中等于v的节点，如果不存在，那么什么都不会做
  frames.remove(frame_id);
  cnt[frame_id] = 0;
}

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
