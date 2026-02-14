#pragma once

#include <coroutine>
#include <exception>

struct Task {
  struct promise_type {
    float waitTime = 0.f;
    
    Task get_return_object() {return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() {return {};}
    std::suspend_always final_suspend() noexcept {return {};}
    void unhandled_exception() {std::terminate();}
    void return_void() {}
    
    std::suspend_always yield_value(float seconds) {
      waitTime = seconds;
      return {};
    }
  };
  
  std::coroutine_handle<promise_type> handle = nullptr;
  
  Task(std::coroutine_handle<promise_type> h) : handle(h) {}
  ~Task() {
    if(handle) {
      handle.destroy();
      Logger::info("Coroutine died.");
    }
  }
  
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  
  Task(Task&& other) noexcept : handle(other.handle) {
    other.handle = nullptr;
  }
  Task& operator=(Task&& other) noexcept {
    if(this != &other) {
      if(handle) handle.destroy();
      handle = other.handle;
      other.handle = nullptr;
    }
    
    return *this;
  }
  
  
  bool resume() {
    if(!handle || handle.done()) return false;
    handle.resume();
    return !handle.done();
  }
};