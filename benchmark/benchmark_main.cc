#include <csignal>

#include "benchmark_program.hh"

namespace {
void HandleSigInt(const int) {
  // Properly exit so that we can see stat information produced by destructors
  // even if a user press C-c.
  std::exit(1);
}
}  // namespace

int main(int argc, char* argv[]) {
  std::signal(SIGINT, HandleSigInt);
  dreal::BenchmarkProgram main{argc, argv};
  return main.Run();
}
