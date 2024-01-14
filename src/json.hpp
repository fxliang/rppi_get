#pragma once

#include "alias.hpp"
#include <fstream>
#include <string_view>

namespace jsonutils {

inline json load_from_file(std::string_view filename) {
  std::ifstream fin(filename.data());
  if (!fin) {
    return json();
  }
  return std::move(json::parse(fin));
}

inline void save_to_file(const json &obj, std::string_view filename) {
  std::ofstream fout(filename.data());
  if (!fout.good()) {
    return;
  }
  fout << obj.dump(4);
  fout.close();
}

} // namespace jsonutils
