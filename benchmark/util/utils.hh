#include <cstdio>
#include <cstring>
#include <iostream>

#define DLINEAR_RUNTIME_ERROR(...)                               \
  std::runtime_error(fmt::format("{}:{} ", __FILE__, __LINE__) + \
                     fmt::format(__VA_ARGS__))

namespace dreal {
/**
 * @brief Checks whether the string ends with the suffix.
 * @param str string to check. It needs to be at least as big as the suffix
 * @param suffix suffix that must be contained in the string
 * @return whether the string contains the suffix
 */
bool EndsWith(const char str[], const char suffix[]);

// bool OutputIsSat(const std::stringstream& output);
}  // namespace dreal
