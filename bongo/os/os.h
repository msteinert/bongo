// Copyright The Go Authors.

#pragma once

#include <sys/types.h>

#include <string>
#include <system_error>
#include <utility>

#include <bongo/os/file.h>
#include <bongo/os/types.h>

namespace bongo::os {

static const file stdin  = std::get<0>(make_file(static_cast<uintptr_t>(syscall::stdin), "/dev/stdin"));
static const file stdout = std::get<0>(make_file(static_cast<uintptr_t>(syscall::stdout), "/dev/stdout"));
static const file stderr = std::get<0>(make_file(static_cast<uintptr_t>(syscall::stderr), "/dev/stderr"));

std::pair<file, std::error_code> open(std::string&& name);
std::pair<file, std::error_code> open(std::string const& name);

std::pair<file, std::error_code> create(std::string&& name);
std::pair<file, std::error_code> create(std::string const& name);

std::pair<file, std::error_code> open_file(std::string&& name, int flag, file_mode perm);
std::pair<file, std::error_code> open_file(std::string const& name, int flag, file_mode perm);

}  // namespace bongo::os
