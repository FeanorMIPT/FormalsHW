#include "solve.h"
#include <gtest/gtest.h>
#include <climits>
#include <string>

void ok(const std::string& regular, char x, bool expectInf, long long expectK) {
  Answer a = Solve(regular, x);
  if (expectInf) {
    EXPECT_TRUE(a.isInf) << regular;
  } else {
    EXPECT_FALSE(a.isInf) << regular;
    EXPECT_EQ(a.value, expectK) << regular;
  }
}

TEST(Base, Literals) {
  ok("a", 'a', false, 1);
  ok("b", 'a', false, 1);
  ok("c", 'a', false, 0);
  ok("1", 'a', false, 0);
}

TEST(Base, Plus) {
  ok("ab+", 'a', false, 1);
  ok("1a+", 'a', false, 1);
}

TEST(Base, Mult) {
  ok("a1.", 'a', false, 1);
  ok("1a.", 'a', false, 1);
  ok("ab.", 'a', false, 0);
  ok("ba.", 'a', false, 1);
  ok("aa.", 'a', false, 2);
}

TEST(Base, Star) {
  ok("a*",   'a', true,  0);
  ok("b*",   'a', false, 0);
  ok("1*",   'a', false, 0);
  ok("aa+*", 'a', true,  0);
  ok("ba*.", 'a', true,  0);
  ok("a*b.", 'a', false, 0);
  ok("a*a.", 'a', true,  0);
}

TEST(Base, More) {
  ok("1a.1.", 'a', false, 1);
  ok("ab+c.", 'a', false, 0);
  ok("aa.a.", 'a', false, 3);
}

TEST(Errors, Syntax) {
  EXPECT_THROW(Solve("",   'a'), SyntaxError);
  EXPECT_THROW(Solve("*",  'a'), SyntaxError);
  EXPECT_THROW(Solve("a+", 'a'), SyntaxError);
  EXPECT_THROW(Solve("aa", 'a'), SyntaxError);
  EXPECT_THROW(Solve("d",  'a'), SyntaxError);
  EXPECT_THROW(Solve("a",  'z'), SyntaxError);
}

TEST(Errors, DotNeedsTwoOperands) {
  EXPECT_THROW(Solve(".", 'a'), SyntaxError);
}

TEST(Internals, Special_min) {
  using namespace solution;

  Special_llong ninf(true);
  Special_llong pinf(false);
  Special_llong a(5LL), b(7LL);

  Special_llong r1 = Special_min(ninf, a);
  EXPECT_TRUE(r1.neg_inf_);
  EXPECT_FALSE(r1.pos_inf_);

  Special_llong r2 = Special_min(pinf, a);
  EXPECT_FALSE(r2.neg_inf_);
  EXPECT_FALSE(r2.pos_inf_);
  EXPECT_EQ(r2.value_, 5);

  Special_llong r3 = Special_min(a, pinf);
  EXPECT_FALSE(r3.neg_inf_);
  EXPECT_FALSE(r3.pos_inf_);
  EXPECT_EQ(r3.value_, 5);

  Special_llong r4 = Special_min(a, b);
  EXPECT_FALSE(r4.neg_inf_);
  EXPECT_FALSE(r4.pos_inf_);
  EXPECT_EQ(r4.value_, 5);
}

TEST(Internals, Special_add) {
  using namespace solution;

  Special_llong ninf(true);
  Special_llong pinf(false);
  Special_llong a(10LL), b(20LL);

  Special_llong s1 = Special_add(ninf, a);
  EXPECT_TRUE(s1.neg_inf_);
  EXPECT_FALSE(s1.pos_inf_);

  Special_llong s2 = Special_add(a, pinf);
  EXPECT_FALSE(s2.neg_inf_);
  EXPECT_TRUE(s2.pos_inf_);

  Special_llong s3 = Special_add(a, b);
  EXPECT_FALSE(s3.neg_inf_);
  EXPECT_FALSE(s3.pos_inf_);
  EXPECT_EQ(s3.value_, 30);
}

TEST(Internals, Special_max) {
  using namespace solution;

  Special_llong ninf(true);
  Special_llong pinf(false);
  Special_llong a(3LL), b(9LL);

  Special_llong m1 = Special_max(pinf, a);
  EXPECT_TRUE(m1.pos_inf_);
  EXPECT_FALSE(m1.neg_inf_);

  Special_llong m2 = Special_max(ninf, a);
  EXPECT_FALSE(m2.neg_inf_);
  EXPECT_FALSE(m2.pos_inf_);
  EXPECT_EQ(m2.value_, 3);

  Special_llong m3 = Special_max(a, ninf);
  EXPECT_FALSE(m3.neg_inf_);
  EXPECT_FALSE(m3.pos_inf_);
  EXPECT_EQ(m3.value_, 3);

  Special_llong m4 = Special_max(a, b);
  EXPECT_FALSE(m4.neg_inf_);
  EXPECT_FALSE(m4.pos_inf_);
  EXPECT_EQ(m4.value_, 9);
}

TEST(Internals, Greater) {
  using namespace solution;

  Special_llong ninf(true);
  Special_llong zero(0LL);
  Special_llong pos(3LL);
  Special_llong pinf(false);

  EXPECT_FALSE(ninf.GreaterOrEqual0());
  EXPECT_FALSE(ninf.Greater0());

  EXPECT_TRUE(zero.GreaterOrEqual0());
  EXPECT_FALSE(zero.Greater0());

  EXPECT_TRUE(pos.GreaterOrEqual0());
  EXPECT_TRUE(pos.Greater0());

  EXPECT_TRUE(pinf.GreaterOrEqual0());
  EXPECT_TRUE(pinf.Greater0());
}
