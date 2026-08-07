#pragma once

template <typename  T>
class EventQueue {
  std::vector<T> m_events;
  
public:
  void push(const T& e) { m_events.push_back(e); }
  void push(T&& e) { m_events.push_back(std::move(e)); }
  const std::vector<T>& events() const { return m_events; }
  void clear() { m_events.clear(); }
  bool empty() const { return m_events.empty(); }
};