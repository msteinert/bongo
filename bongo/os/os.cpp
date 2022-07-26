// Copyright The Go Authors.

#include <string>
#include <system_error>
#include <utility>

#include "bongo/os/os.h"

namespace bongo::os {

std::pair<file, std::error_code> open(std::string&& name) {
  return open_file(std::move(name), o_rdonly, 0);
}

std::pair<file, std::error_code> open(std::string const& name) {
  return open(std::string{name});
}

std::pair<file, std::error_code> create(std::string&& name) {
  return open_file(std::move(name), o_rdwr|o_create|o_trunc, 0666);
}

std::pair<file, std::error_code> create(std::string const& name) {
  return create(std::string{name});
}

std::pair<file, std::error_code> open_file(std::string&& name, int flag, file_mode perm) {
  return open_file_nolog(std::move(name), flag, perm);
}

std::pair<file, std::error_code> open_file(std::string const& name, int flag, file_mode perm) {
  return open_file(std::string{name}, flag, perm);
}

}  // namespace bongo::os
