// Copyright The Go Authors.

#pragma once

#include <utility>

#include <bongo/bufio/reader.h>
#include <bongo/bufio/writer.h>

namespace bongo::bufio {

template <typename T1, typename T2>
struct read_writer : public reader<T1>, public writer<T2> {
  read_writer(reader<T1> rd, writer<T2> wr)
      : reader<T1>{std::move(rd)}
      , writer<T2>{std::move(wr)} {}
};

}  // namespace bongo::bufio
