//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include <list>

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete replacer_;
}

Page *BufferPoolManager::FetchPageImpl(page_id_t page_id) {
  frame_id_t p = -1;
  // 首先判断对应page是不是已经保存在frame中了,如果是，那么可以直接pin,然后返回
  latch_.lock();
  if (page_table_.count(page_id) > 0) {
    p = page_table_[page_id];
  }
  latch_.unlock();
  if (p != -1) {
    return PinAndReturn(p);
  }

  // 首先判断，如果freelist为空，并且victim也返回false（为空），那么只能返回nullptr
  latch_.lock();
  if (free_list_.empty() && !replacer_->Victim(&p)) {
    latch_.unlock();
    return nullptr;
  }
  // 从freelist或者replacer中取出victim frame（优先freelist）
  if (!free_list_.empty()) {
    p = free_list_.front();
    free_list_.pop_front();
  } else {
    // 第一个判断之所以为假，必定是freelist和replacer中有一个不空，刚才已经判断了freelist为空，那么一定是replacer不空，那么现在p已经是victim
    // frame了
  }
  latch_.unlock();

  // 更新元数据：1.将数据从磁盘读到victim frame中
  //  2.设置pinCnt为1 3.设置pageTable:修改原来pageId对应的frame为-1，修改现在的pageId对应现在这个frame,
  //  4.修改isDirty为false？
  //  5 修改pageId
  latch_.lock();
  //  这个时候需要writeBack？ 应该是需要的
  if (pages_[p].is_dirty_) {
    FlushPageImpl(pages_[p].GetPageId());
  }
  disk_manager_->ReadPage(page_id, pages_[p].data_);
  pages_[p].pin_count_ = 1;
  int oldPageId = pages_[p].GetPageId();
  if (oldPageId != INVALID_PAGE_ID) {
    page_table_[oldPageId] = -1;
  }
  pages_[p].page_id_ = page_id;
  pages_[p].is_dirty_ = false;
  page_table_[page_id] = p;
  latch_.unlock();

  return &pages_[p];
}

bool BufferPoolManager::UnpinPageImpl(page_id_t page_id, bool is_dirty) {
  latch_.lock();
  frame_id_t p = page_table_[page_id];
  if (pages_[p].GetPinCount() <= 0) {
    latch_.unlock();
    return false;
  }
  if (is_dirty) {
    pages_[p].is_dirty_ = true;
  }
  pages_[p].pin_count_--;
  //  Unpin(T) : This method should be called when the pin_count of a page becomes 0. This method should add the frame
  //  containing the unpinned page to the LRUReplacer.
  if (pages_[p].pin_count_ == 0) {
    replacer_->Unpin(p);
  }
  latch_.unlock();
  return true;
}

//  这个函数都被谁调用？
//  在测试中这个函数会被直接调用，测试没有在外面加锁，需要改进
// 加了锁反而过不了的更多了
bool BufferPoolManager::FlushPageImpl(page_id_t page_id) {
  //  LOG_DEBUG("write back page %d\n", page_id);
  frame_id_t p = -1;
  //  pages_[page_id].RLatch();
  if (page_table_.count(page_id) > 0) {
    p = page_table_[page_id];
    // Make sure you call DiskManager::WritePage!
    disk_manager_->WritePage(page_id, pages_[p].GetData());
    //    pages_[page_id].RUnlatch();
    return true;
  }
  //  pages_[page_id].RUnlatch();
  // return false if the page could not be found in the page table, true otherwise
  return false;
}

// 其实这个的后面一部分和fetchPage很相似，只是不需要readPage
Page *BufferPoolManager::NewPageImpl(page_id_t *page_id) {
  *page_id = disk_manager_->AllocatePage();
  //  LOG_DEBUG("new page id %d\n",*page_id);
  frame_id_t p = -1;

  // 首先判断，如果freelist为空，并且victim也返回false（为空），那么只能返回nullptr
  latch_.lock();
  if (free_list_.empty() && !replacer_->Victim(&p)) {
    latch_.unlock();
    return nullptr;
  }

  // 从freelist或者replacer中取出victim frame（优先freelist）
  if (!free_list_.empty()) {
    p = free_list_.front();
    free_list_.pop_front();
  } else {
    // 第一个判断之所以为假，必定是freelist和replacer中有一个不空，
    // 刚才已经判断了freelist为空，那么一定是replacer不空，那么现在p已经是victim
    // frame了
  }
  latch_.unlock();

  // 更新元数据：
  //  2.设置pinCnt为1 3.设置pageTable:修改原来pageId对应的frame为-1，
  //  修改现在的pageId对应现在这个frame, 4.修改isDirty为false？
  //  5 修改pageId
  latch_.lock();
  //  这个时候需要writeBack？ 应该是需要的
  if (pages_[p].is_dirty_) {
    FlushPageImpl(pages_[p].GetPageId());
  }
  pages_[p].pin_count_ = 1;
  int oldPageId = pages_[p].GetPageId();
  if (oldPageId != INVALID_PAGE_ID) {
    page_table_[oldPageId] = -1;
  }
  pages_[p].page_id_ = *page_id;
  pages_[p].is_dirty_ = false;
  page_table_[*page_id] = p;
  latch_.unlock();

  return &pages_[p];
}

bool BufferPoolManager::DeletePageImpl(page_id_t page_id) {
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  if (page_table_.count(page_id) == 0 || page_table_[page_id] == -1) {
    return true;
  }

  int pin_cnt = -1;
  frame_id_t p = -1;
  latch_.lock();
  p = page_table_[page_id];
  pin_cnt = pages_[p].GetPinCount();
  latch_.unlock();
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  if (pin_cnt > 0) {
    return false;
  }
  // 0.   Make sure you call DiskManager::DeallocatePage!
  //  应该是确认没人在使用才dealloc吧？
  disk_manager_->DeallocatePage(page_id);
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free
  // list.
  //  这里需要write back吗 既然disk上的page都已经删除了 那就不需要写回？
  latch_.lock();
  page_table_[page_id] = -1;
  pages_[p].pin_count_ = 0;

  free_list_.push_back(p);
  latch_.unlock();

  return true;
}

void BufferPoolManager::FlushAllPagesImpl() {
  //  要访问page table,所以加上latch？
  latch_.lock();
  for (auto &[k, v] : page_table_) {
    if (v != -1) {
      FlushPageImpl(k);
    }
  }
  latch_.unlock();
}

}  // namespace bustub
