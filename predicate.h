/*
 * Copyright (c) 2003-2008, John Wiegley.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of New Artisans LLC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PREDICATE_H
#define _PREDICATE_H

#include "expr.h"
#include "scope.h"

namespace ledger {

template <typename T>
class item_predicate
{
public:
  expr_t predicate;

  item_predicate() {
    TRACE_CTOR(item_predicate, "");
  }
  item_predicate(const item_predicate& other) : predicate(other.predicate) {
    TRACE_CTOR(item_predicate, "copy");
  }
  item_predicate(const expr_t& _predicate) : predicate(_predicate) {
    TRACE_CTOR(item_predicate, "const expr_t&");
  }
  item_predicate(const string& _predicate) : predicate(expr_t(_predicate)) {
    TRACE_CTOR(item_predicate, "const string&");
  }
  ~item_predicate() throw() {
    TRACE_DTOR(item_predicate);
  }

  bool operator()(const T& item) const {
#if 0
    template context_t<T> context(item);
    return ! predicate || predicate->calc(context).strip_annotations();
#else
    return false;
#endif
  }
};

} // namespace ledger

#endif // _PREDICATE_H