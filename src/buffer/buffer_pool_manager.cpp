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
#include <unordered_map>

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

//  给的参数是page_id,但是pages数组中用的index是frame_id -> 就是通过page_table转换嘛
Page *BufferPoolManager::FetchPageImpl(page_id_t page_id) {
  //  frame_id可能是负数吗？回忆：frame就是bufferpool的entry，所以frame
  // id是0-indexing,取到frameid之后只需要使用pages[id]即可取到page obj
  frame_id_t p = -1;
  latch_.lock();
  // 1.     Search the page table for the requested page (P).
  //  什么情况下不存在于page_table?
  // 这里是两种情况，1.没加入到pagetable 2.加入之后，修改了val=-1
  if (page_table_.count(page_id) > 0) {
    p = page_table_[page_id];
  }
  latch_.unlock();
  if (p != -1) {
    // 1.1    If P exists, pin it and return it immediately.
    return PinAndReturn(p);
  }
  // 这里的不存在，应该是指，物理页面不存在于内存中，所以1是需要在内存中找一个位置(page)保存他 2.从disk上读到对应位置
  // 而找一个位置，就要么从freelist中找一个空闲页，要么从replacer中找一个当前没有使用的页（unpined）来evict
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  latch_.lock();
  if (!free_list_.empty()) {
    //  这里应该使用list的哪个api？应该先拿走后面的还是前面的？需要进一步确定
    // 这里暂时是先拿走后面的，类似LC LRU的例子
    p = free_list_.front();
    free_list_.pop_front();
  }
  latch_.unlock();
  if (p != -1) {
    // 这里如果等于-1，说明上面的freelist判断为空，而不是指freelist pop出来的数有-1
    // 这里的pinandreturn,应该和上面的实现不一样？之前一个是已经在内存中，那么pinCnt++，而这个是在freelist中，应该明确pinCnt就是1
    // 还是说从freeList中出来的一定是0？首先构造函数把他们都加进去时，确实是0，但是还需要看以后加到freelist时，有没有把pinCnt清零
    // ↑ 应该是freelist的cnt都是0
    latch_.lock();
    //  这里是不是需要修改page table
    pages_[p].page_id_ = page_id;
    page_table_[pages_[p].GetPageId()] = -1;
    page_table_[page_id] = p;
    latch_.unlock();
    return PinAndReturn(p);
  }
  // 2.     If R is dirty, write it back to the disk.
  //  三种情况：本来就在内存中，复用freelist中，复用replacer，当然只有最后一种需要evict,以及检查dirty并write back
  latch_.lock();
  if (replacer_->Victim(&p)) {
    // 找到Victim，首先检查dirty bit
    if (pages_[p].is_dirty_) {
      //  写回到磁盘上，应该加上shared lock？但是外面已经有latch了
      //      pages_[p].RLatch();
      FlushPageImpl(pages_[p].GetPageId());
    }
    // 3.     Delete R from the page table and insert P.
    // pageTable是管理pageId到frameId的映射，而现在是把从replacer中取出的一个frame对应到了另外一个page，所以直接修改映射关系即可
    //  page_table怎么删除？直接设为-1？
    // p是一个victim frame,把他原来对应的pageId在pageTable中对应的frameId设为-1,然后让pageId对应到p
    page_table_[pages_[p].GetPageId()] = -1;
    page_table_[page_id] = p;

    //  怎么更新page的元数据？哪些元数据？ is_dirty,pinCnt,pageId,这里应该就是pageId ;
    pages_[p].page_id_ = page_id;
    // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
    disk_manager_->ReadPage(page_id, pages_[p].data_);
    latch_.unlock();
    return PinAndReturn(p);
  }
  latch_.unlock();
  //  For FetchPageImpl,you should return NULL if no page is available in the free list and all other pages are
  //  currently pinned.
  return nullptr;
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
bool BufferPoolManager::FlushPageImpl(page_id_t page_id) {
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

Page *BufferPoolManager::NewPageImpl(page_id_t *page_id) {
  // 0.   Make sure you call DiskManager::AllocatePage!
  // 也许对磁盘的操作，应该等到找到page再做？如果freelist和replacer中都没有，但是已经分配了磁盘上的page，是不是不对？
  *page_id = disk_manager_->AllocatePage();
  frame_id_t victim = -1;
  latch_.lock();
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  if (!free_list_.empty()) {
    victim = free_list_.front();
    free_list_.pop_front();
    // 删除原来页表里的映射:
    page_table_[pages_[victim].GetPageId()] = -1;
    page_table_[*page_id] = victim;
    pages_[victim].pin_count_ = 1;
    pages_[victim].page_id_ = *page_id;
    latch_.unlock();
    return &pages_[victim];
  }
  if (replacer_->Victim(&victim)) {
    if (pages_[victim].is_dirty_) {
      FlushPageImpl(pages_[victim].GetPageId());
    }
    // 删除原来页表里的映射:
    page_table_[pages_[victim].GetPageId()] = -1;
    pages_[victim].ResetMemory();
    pages_[victim].pin_count_ = 1;
    pages_[victim].page_id_ = *page_id;
    page_table_[*page_id] = victim;
    latch_.unlock();
    return &pages_[victim];
  }
  // 之前没有加上下面这一行，导致死锁，这里可以说明go中defer的作用：lock之后，马上defer unlock
  latch_.unlock();
  //   1.   If all the pages in the buffer pool are pinned, return nullptr.
  return nullptr;
}

bool BufferPoolManager::DeletePageImpl(page_id_t page_id) {
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  if (page_table_.count(page_id) == 0) {
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
