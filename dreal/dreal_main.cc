#include <csignal>

#include "dreal/main_program.h"
#include "dreal/util/timer.h"

namespace {
void HandleSigInt(const int) {
  // Properly exit so that we can see stat information produced by destructors
  // even if a user press C-c.
  std::exit(1);
}
}  // namespace

int main(int argc, const char* argv[]) {
  std::signal(SIGINT, HandleSigInt);
  dreal::main_timer.start();
  dreal::MainProgram main_program{argc, argv};
  return main_program.Run();
}
