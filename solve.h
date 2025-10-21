#pragma once
#include <algorithm>
#include <cassert>
#include <climits>
#include <stdexcept>
#include <string>
#include <vector>

struct SyntaxError : std::runtime_error {
  size_t pos;
  char token;
  SyntaxError(const std::string& msg, size_t p, char t)
      : runtime_error(msg), pos(p), token(t) {}
};

struct Answer {
  bool isInf = false;
  long long value = 0;
  explicit Answer(long long ans) {
    if (ans == LLONG_MAX) {
      isInf = true;
    } else {
      value = ans;
    }
  }
};

namespace solution {
struct Special_llong {
  long long value_ = 0;
  bool pos_inf_ = false;
  bool neg_inf_ = false;

  Special_llong() = default;

  Special_llong(long long k, bool pos_inf, bool neg_inf)
      : value_(k), pos_inf_(pos_inf), neg_inf_(neg_inf) {}

  explicit Special_llong(long long k) : value_(k) {}

  explicit Special_llong(bool is_neg_inf)
      : pos_inf_(!is_neg_inf), neg_inf_(is_neg_inf) {}

  Special_llong& operator=(const Special_llong& other) = default;

  Special_llong(const Special_llong& other) = default;

  bool GreaterOrEqual0() const {
    return !neg_inf_ and (pos_inf_ or value_ >= 0);
  }

  bool Greater0() const { return !neg_inf_ and (pos_inf_ or value_ > 0); }
};

Special_llong Special_max(const Special_llong& x, const Special_llong& y) {
  if (x.pos_inf_ or y.pos_inf_) {
    return Special_llong(false);
  }
  if (x.neg_inf_) {
    return y;
  }
  if (y.neg_inf_) {
    return x;
  }
  return Special_llong(std::max(x.value_, y.value_));
}

Special_llong Special_min(const Special_llong& x, const Special_llong& y) {
  if (x.neg_inf_ or y.neg_inf_) {
    return Special_llong(true);
  }
  if (x.pos_inf_) {
    return y;
  }
  if (y.pos_inf_) {
    return x;
  }
  return Special_llong(std::min(x.value_, y.value_));
}

Special_llong Special_add(const Special_llong& x, const Special_llong& y) {
  if (x.neg_inf_ or y.neg_inf_) {
    return Special_llong(true);
  }
  if (x.pos_inf_ or y.pos_inf_) {
    return Special_llong(false);
  }
  return Special_llong(x.value_ + y.value_);
}

struct subLang {
  bool nullable = false;
  Special_llong end_length_ = Special_llong(0LL);
  Special_llong length_ = Special_llong(0LL);
};

bool IsLetter(char c) { return c == 'a' || c == 'b' || c == 'c'; }

subLang BaseSymbol(char sym, char x) {
  subLang s;
  if (sym == '1') {
    s.nullable = true;
    s.end_length_ = Special_llong(0LL);
    s.length_ = Special_llong(0LL);
    return s;
  }
  if (sym == x) {
    s.nullable = false;
    s.end_length_ = Special_llong(1LL);
    s.length_ = Special_llong(1LL);
    return s;
  }
  s.nullable = false;
  s.end_length_ = Special_llong(0LL);
  s.length_ = Special_llong(true);
  return s;
}

subLang RegOr(const subLang& A, const subLang& B) {
  subLang R;
  R.nullable = A.nullable || B.nullable;
  R.end_length_ = Special_max(A.end_length_, B.end_length_);
  R.length_ = Special_max(A.length_, B.length_);
  return R;
}

subLang RegAnd(const subLang& A, const subLang& B) {
  subLang R;
  R.nullable = A.nullable && B.nullable;
  R.length_ = Special_add(A.length_, B.length_);
  if (B.length_.GreaterOrEqual0()) {
    R.end_length_ = Special_max(B.end_length_, Special_add(A.end_length_, B.length_));
    return R;
  }
  R.end_length_ = B.end_length_;
  return R;
}

subLang RegStar(const subLang& A) {
  subLang R;
  R.nullable = true;
  if (A.length_.Greater0()) {
    R.length_ = Special_llong(false);
    R.end_length_ = Special_llong(false);
    return R;
  }
  R.length_ = Special_llong(0LL);
  R.end_length_ = A.end_length_;

  return R;
}
}  // namespace solution

Answer Solve(const std::string& reqular, char x) {
  if (!solution::IsLetter(x)) {
    throw SyntaxError("x must be one of {a,b,c}", 0, x);
  }

  std::vector<solution::subLang> st;
  st.reserve(reqular.size());

  for (size_t i = 0; i < reqular.size(); ++i) {
    char c = reqular[i];
    if (solution::IsLetter(c) || c == '1') {
      st.push_back(solution::BaseSymbol(c, x));
    } else if (c == '*') {
      if (st.empty()) {
        throw SyntaxError("operator '*' needs operand", i, c);
      }
      solution::subLang A = st.back();
      st.pop_back();
      st.push_back(solution::RegStar(A));
    } else if (c == '.') {
      if (st.size() < 2) {
        throw SyntaxError("operator . needs 2 operands", i, c);
      }
      solution::subLang B = st.back();
      st.pop_back();
      solution::subLang A = st.back();
      st.pop_back();
      st.push_back(solution::RegAnd(A, B));
    } else if (c == '+') {
      if (st.size() < 2) {
        throw SyntaxError("operator + needs 2 operands", i, c);
      }
      solution::subLang B = st.back();
      st.pop_back();
      solution::subLang A = st.back();
      st.pop_back();
      st.push_back(solution::RegOr(A, B));
    } else {
      throw SyntaxError("invalid token", i, c);
    }
  }
  if (st.empty()) {
    throw SyntaxError("empty expression", reqular.size(), '\0');
  }
  if (st.size() != 1) {
    throw SyntaxError("expression finished with extra items on stack",
                      reqular.size(), '\0');
  }
  solution::subLang S = st.back();
  long long res = LLONG_MAX;
  if (!S.end_length_.pos_inf_) {
    res = S.end_length_.value_;
  }
  return Answer(res);
}