#include "utils.hh"

using std::string;

namespace dreal {

bool EndsWith(const char str[], const char suffix[]) {
  size_t str_len = strlen(str), suffix_len = strlen(suffix);
  return suffix_len > 0 && str_len >= suffix_len &&
         0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

// bool OutputIsSat(const std::stringstream& output) { return true; }
}  // namespace dreal