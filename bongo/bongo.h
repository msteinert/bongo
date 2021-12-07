// Copyright The Go Authors.

#pragma once

#include <bongo/runtime/chan.h>
#include <bongo/runtime/defer.h>
#include <bongo/runtime/nil.h>
#include <bongo/runtime/rune.h>
#include <bongo/runtime/select.h>

namespace bongo {

using runtime::chan;
using runtime::default_select_case;
using runtime::defer;
using runtime::recv_select_case;
using runtime::rune;
using runtime::select;
using runtime::select_case;
using runtime::send_select_case;

constexpr static runtime::nil_t nil;

}  // namespace bongo
