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
  
  std::coroutine_handle<promise_type> handle;
  
  bool resume() {
    if(!handle || handle.done()) return false;
    handle.resume();
    return !handle.done();
  }
};