#include "..//CYK.h"
#include <fstream>
#include <gtest/gtest.h>

static std::string write_temp_file(const std::string& content) {
  static int cnt = 0;
  std::string name = "test_input_" + std::to_string(cnt++) + ".txt";
  std::ofstream out(name);
  out << content;
  return name;
}

TEST(ReadInput, SampleFromStatement) {
  const std::string input = "1 2 2\n"
                            "S\n"
                            "ab\n"
                            "S-> aSbS\n"
                            "S ->\n"
                            "S\n"
                            "2\n"
                            "aababb\n"
                            "aabbba\n";

  std::vector<std::string> words;
  std::string path = write_temp_file(input);

  Grammar g = read_input(path, words);

  ASSERT_EQ(words.size(), 2u);
  EXPECT_EQ(words[0], "aababb");
  EXPECT_EQ(words[1], "aabbba");

  ASSERT_NE(g.start_, nullptr);
  EXPECT_EQ(g.start_->real_value_, 'S');
  EXPECT_EQ(g.noterms_.size(), 1u);
  EXPECT_EQ(g.terminals_.size(), 2u);
  EXPECT_EQ(g.rules_.size(), 2u);
}

TEST(ReadInput, UnknownSymbolThrows) {
  const std::string input = "1 1 1\n"
                            "S\n"
                            "a\n"
                            "S->x\n"
                            "S\n"
                            "1\n"
                            "a\n";

  std::vector<std::string> words;
  std::string path = write_temp_file(input);

  EXPECT_THROW(
      {
        Grammar g = read_input(path, words);
        (void)g;
      },
      std::runtime_error);
}

TEST(CYK, SampleFromStatement) {
  const std::string input = "1 2 2\n"
                            "S\n"
                            "ab\n"
                            "S-> aSbS\n"
                            "S ->\n"
                            "S\n"
                            "2\n"
                            "aababb\n"
                            "aabbba\n";

  std::vector<std::string> words;
  std::string path = write_temp_file(input);

  Grammar g = read_input(path, words);
  CYKParser parser = CYKParser::fit(g);

  EXPECT_TRUE(parser.predict("aababb"));
  EXPECT_FALSE(parser.predict("aabbba"));
  EXPECT_TRUE(parser.predict(""));
}

TEST(CYK, EmptyWordWhenNoEps) {
  Noterminal::counter = 0;
  Terminal::counter = 0;

  Grammar g;

  g.noterms_.emplace_back('S', true);
  g.terminals_.emplace_back('a');
  g.start_ = &g.noterms_[0];

  Rule r;
  r.left_ = &g.noterms_[0];
  r.right_.emplace_back(Symbol::Kind::terminal, 'a', g.terminals_[0].value_);
  g.rules_.push_back(r);
  g.new_rules_ = g.rules_;

  CYKParser parser = CYKParser::fit(g);

  EXPECT_FALSE(parser.predict(""));
  EXPECT_TRUE(parser.predict("a"));
  EXPECT_FALSE(parser.predict("aa"));
}

TEST(GrammarTransform, EmptyGrammar) {
  Grammar g;
  g.rm_nongenerating_noterms();
  g.rm_unreachable_noterms();
  g.rm_mixed_rules();
  g.rm_long_rules();
  g.rm_eps_rules();
  g.rm_chained_rules();

  EXPECT_TRUE(g.rules_.empty());
}

static Grammar make_grammar_eps_chain() {
  Noterminal::counter = 0;
  Terminal::counter = 0;

  Grammar g;
  g.noterms_.emplace_back('S', true);
  g.noterms_.emplace_back('A', true);

  g.terminals_.emplace_back('a');
  g.start_ = &g.noterms_[0];

    Rule r1;
    r1.left_ = &g.noterms_[0];
    g.rules_.push_back(r1);

    Rule r2;
    r2.left_ = &g.noterms_[0];
    r2.right_.emplace_back(Symbol::Kind::noterminal, 'A', g.noterms_[1].value_);
    r2.right_.emplace_back(Symbol::Kind::noterminal, 'A', g.noterms_[1].value_);
    g.rules_.push_back(r2);

    Rule r3;
    r3.left_ = &g.noterms_[1];
    r3.right_.emplace_back(Symbol::Kind::noterminal, 'S', g.noterms_[0].value_);
    r3.right_.emplace_back(Symbol::Kind::noterminal, 'S', g.noterms_[0].value_);
    g.rules_.push_back(r3);

    Rule r4;
    r4.left_ = &g.noterms_[1];
    r4.right_.emplace_back(Symbol::Kind::terminal, 'a', g.terminals_[0].value_);
    g.rules_.push_back(r4);

  g.new_rules_ = g.rules_;
  return g;
}

TEST(GrammarTransform, EpsRulesAndAddEpsilon) {
  Grammar g = make_grammar_eps_chain();

  g.rm_eps_rules();
  EXPECT_TRUE(g.has_eps);
  for (const auto& r : g.rules_) {
    EXPECT_FALSE(r.right_.empty());
  }
  Noterminal* old_start = g.start_;
  g.add_epsilon();

  ASSERT_NE(g.start_, nullptr);
  EXPECT_NE(g.start_, old_start);

  bool has_empty_from_new_start = false;
  bool has_new_start_to_old_start = false;

  for (const auto& r : g.rules_) {
    if (r.left_ == g.start_ && r.right_.empty()) {
      has_empty_from_new_start = true;
    }
    if (r.left_ == g.start_ && r.right_.size() == 1 &&
        r.right_[0].kind_ == Symbol::Kind::noterminal &&
        r.right_[0].value_ == old_start->value_) {
      has_new_start_to_old_start = true;
    }
  }

  EXPECT_TRUE(has_empty_from_new_start);
  EXPECT_TRUE(has_new_start_to_old_start);
}

static Grammar make_grammar_chains() {
  Noterminal::counter = 0;
  Terminal::counter = 0;

  Grammar g;
  g.noterms_.emplace_back('S', true);
  g.noterms_.emplace_back('A', true);
  g.noterms_.emplace_back('B', true);

  g.terminals_.emplace_back('a');

  g.start_ = &g.noterms_[0];
    Rule r1;
    r1.left_ = &g.noterms_[0];
    r1.right_.emplace_back(Symbol::Kind::noterminal, 'A', g.noterms_[1].value_);
    g.rules_.push_back(r1);

    Rule r2;
    r2.left_ = &g.noterms_[1];
    r2.right_.emplace_back(Symbol::Kind::noterminal, 'B', g.noterms_[2].value_);
    g.rules_.push_back(r2);

    Rule r3;
    r3.left_ = &g.noterms_[2];
    r3.right_.emplace_back(Symbol::Kind::terminal, 'a', g.terminals_[0].value_);
    g.rules_.push_back(r3);

  g.new_rules_ = g.rules_;
  return g;
}

TEST(GrammarTransform, ChainRulesRemoved) {
  Grammar g = make_grammar_chains();
  g.rm_chained_rules();

  bool has_S_a = false;
  bool has_A_a = false;

  for (const auto& r : g.rules_) {
    if (r.right_.size() == 1 && r.right_[0].kind_ == Symbol::Kind::noterminal) {
      FAIL() << "Chain rule still present";
    }

    if (r.right_.size() == 1 && r.right_[0].kind_ == Symbol::Kind::terminal &&
        r.right_[0].real_value_ == 'a') {
      if (r.left_->real_value_ == 'S') {
        has_S_a = true;
      }
      if (r.left_->real_value_ == 'A') {
        has_A_a = true;
      }
    }
  }

  EXPECT_TRUE(has_S_a);
  EXPECT_TRUE(has_A_a);
}

TEST(GrammarTransform, MixedAndLongCombined) {
  Noterminal::counter = 0;
  Terminal::counter = 0;

  Grammar g;
  g.noterms_.emplace_back('S', true);
  g.noterms_.emplace_back('A', true);
  g.noterms_.emplace_back('B', true);
  g.noterms_.emplace_back('C', true);

  g.terminals_.emplace_back('a');
  g.terminals_.emplace_back('b');

  g.start_ = &g.noterms_[0];

    Rule r;
    r.left_ = &g.noterms_[0];
    r.right_.emplace_back(Symbol::Kind::terminal, 'a', g.terminals_[0].value_);
    r.right_.emplace_back(Symbol::Kind::noterminal, 'A', g.noterms_[1].value_);
    r.right_.emplace_back(Symbol::Kind::noterminal, 'B', g.noterms_[2].value_);
    r.right_.emplace_back(Symbol::Kind::noterminal, 'C', g.noterms_[3].value_);
    g.rules_.push_back(r);


  g.new_rules_ = g.rules_;

  g.rm_mixed_rules();
  g.rm_long_rules();

  for (const auto& rule : g.rules_) {
    EXPECT_LE(rule.right_.size(), 2u);
    bool has_term = false;
    bool has_noterm = false;
    for (const auto& s : rule.right_) {
      if (s.kind_ == Symbol::Kind::terminal) {
        has_term = true;
      } else if (s.kind_ == Symbol::Kind::noterminal) {
        has_noterm = true;
      }
    }
    EXPECT_FALSE(has_term && has_noterm);
  }
}
