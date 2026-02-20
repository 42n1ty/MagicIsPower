#include "core/app.hpp"
#include "common/logger.hpp"

#include <Windows.h>
//====================================================================================================

//====================================================================================================
int main() {
  #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    SetConsoleOutputCP(CP_UTF8); 
  #endif
  
  mip::Application app;
  if(!app.init()) {
    Logger::error("Failed to init app");
    return -1;
  }
  app.run();
  
  return 0;
}