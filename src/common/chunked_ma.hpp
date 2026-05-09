#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>

class ChunkedMA {
  struct Chunk {
    uint8_t* m_buffer;
    size_t m_offset;
    size_t m_capacity;
    Chunk* next;
  };
  
  Chunk* m_head;
  Chunk* m_cur;
  size_t m_chSize;
  
  Chunk* alCh(size_t size) {
    Chunk* chunk = new Chunk();
    chunk->m_buffer = (uint8_t*)std::malloc(size);
    chunk->m_capacity = size;
    chunk->m_offset = 0;
    chunk->next = nullptr;
    
    return chunk;
  }
  
public:
  ChunkedMA(size_t defSize = 1024 * 1024)
  : m_chSize(defSize) {
    m_head = alCh(m_chSize);
    m_cur = m_head;
  }
  
  ~ChunkedMA() {
    Chunk* chunk = m_head;
    while(chunk != nullptr) {
      Chunk* next = chunk->next;
      std::free(chunk->m_buffer);
      delete chunk;
      chunk = next;
    }
  }
  
  template<typename T>
  T* alloc(size_t count = 1) {
    size_t align = alignof(T);
    size_t size = sizeof(T) * count;
    //          next step of type align & bitmask to make it multiply of type align; brilliant
    m_cur->m_offset = (m_cur->m_offset + align - 1)   & ~(align - 1);
    
    if(m_cur->m_offset + size > m_cur->m_capacity) {
      if(m_cur->next == nullptr) {
        size_t newSize = std::max(m_chSize, size);
        m_cur->next = alCh(newSize);
      }
      
      m_cur = m_cur->next;
      m_cur->m_offset = (align - 1) & ~(align - 1);
    }
    
    T* ptr = reinterpret_cast<T*>(m_cur->m_buffer + m_cur->m_offset);
    m_cur->m_offset += size;
    return ptr;
  }
  
  void reset() {
    Chunk* chunk = m_head;
    while(chunk != nullptr) {
      chunk->m_offset = 0;
      chunk = chunk->next;
    }
    m_cur = m_head;
  }
  
};