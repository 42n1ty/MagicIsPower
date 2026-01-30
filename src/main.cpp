#include "core/app.hpp"
#include "common/logger.hpp"
//====================================================================================================

//====================================================================================================
int main() {
  
  mip::Application app;
  if(!app.init()) {
    Logger::error("Failed to init app");
    return -1;
  }
  app.run();
  
  return 0;
}