#include <cctype>
#include <deque>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Symbol {
  enum class Kind { noterminal, terminal, other };

  Kind kind_ = Kind::other;
  char real_value_ = '0';
  int value_ = -1;

  Symbol(Kind kind, char real_value, int value)
      : kind_(kind), real_value_(real_value), value_(value) {}
};

struct Noterminal : Symbol {
  bool isInitial_ = false;
  static int counter;

  explicit Noterminal(char value, bool is_initial = false)
      : Symbol(Kind::noterminal, value, counter), isInitial_(is_initial) {
    ++counter;
  }
};



struct Terminal : Symbol {
  static int counter;

  explicit Terminal(char value) : Symbol(Kind::terminal, value, counter) {
    ++counter;
  }
};


struct Rule {
  Noterminal* left_ = nullptr;
  std::deque<Symbol> right_;
  Rule() = default;
};

struct Grammar {
  std::deque<Rule> rules_;
  std::deque<Rule> new_rules_;
  std::deque<Noterminal> noterms_;
  std::deque<Terminal> terminals_;
  Noterminal* start_ = nullptr;
  bool has_eps = false;

  Grammar() = default;

  Grammar(const Grammar&) = delete;
  Grammar& operator=(const Grammar&) = delete;

  Grammar(Grammar&&) = default;
  Grammar& operator=(Grammar&&) = default;

  void rm_nongenerating_noterms() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<char> generating(noterms_.size(), 0);
    bool has_changed = true;
    while (has_changed) {
      has_changed = false;
      for (const auto& r : rules_) {
        if (generating[r.left_->value_]) {
          continue;
        }
        bool all_ok = true;
        for (const auto& s : r.right_) {
          if (s.kind_ == Symbol::Kind::noterminal) {
            if (!generating[s.value_]) {
              all_ok = false;
              break;
            }
          }
        }
        if (all_ok) {
          generating[r.left_->value_] = 1;
          has_changed = true;
        }
      }
    }

    if (start_ != nullptr) {
      if (!generating[start_->value_]) {
        new_rules_.clear();
        rules_.clear();
        return;
      }
    }
    new_rules_.clear();
    for (const auto& r : rules_) {
      if (!generating[r.left_->value_]) {
        continue;
      }
      bool ok_right = true;
      for (const auto& s : r.right_) {
        if (s.kind_ == Symbol::Kind::noterminal) {
          if (!generating[s.value_]) {
            ok_right = false;
            break;
          }
        }
      }
      if (ok_right) {
        new_rules_.push_back(r);
      }
    }
    rules_ = new_rules_;
  }

  void rm_unreachable_noterms() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<char> reachable(noterms_.size(), 0);
    reachable[start_->value_] = 1;
    bool has_changed = true;
    while (has_changed) {
      has_changed = false;
      for (const auto& r : rules_) {
        if (!reachable[r.left_->value_]) {
          continue;
        }
        for (const Symbol& s : r.right_) {
          if (s.kind_ == Symbol::Kind::noterminal) {
            if (!reachable[s.value_]) {
              reachable[s.value_] = 1;
              has_changed = true;
            }
          }
        }
      }
    }
    new_rules_.clear();
    for (const auto& r : rules_) {
      if (!reachable[r.left_->value_]) {
        continue;
      }
      new_rules_.push_back(r);
    }
    rules_ = new_rules_;
  }

  void rm_mixed_rules() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<Noterminal*> term_wrap(terminals_.size(), nullptr);
    std::vector<Rule> added;
    for (auto& r : rules_) {
      bool has_term = false;
      bool has_noterm = false;
      for (const auto& s : r.right_) {
        if (s.kind_ == Symbol::Kind::terminal) {
          has_term = true;
        } else if (s.kind_ == Symbol::Kind::noterminal) {
          has_noterm = true;
        }
        if (has_term && has_noterm) {
          break;
        }
      }
      if (!has_term || r.right_.size() < 2) {
        continue;
      }
      for (auto& s : r.right_) {
        if (s.kind_ == Symbol::Kind::noterminal) {
          continue;
        }
        Noterminal* noterm_ptr = term_wrap[s.value_];

        if (!noterm_ptr) {
          noterms_.emplace_back(Noterminal(terminals_[s.value_].real_value_));
          noterm_ptr = &noterms_.back();
          Rule new_rule;
          new_rule.left_ = noterm_ptr;
          new_rule.right_.emplace_back(Symbol::Kind::terminal,
                                       terminals_[s.value_].real_value_,
                                       s.value_);
          added.push_back(new_rule);
        }
        term_wrap[s.value_] = noterm_ptr;
        s = Symbol(Symbol::Kind::noterminal, noterm_ptr->real_value_,
                   noterm_ptr->value_);
      }
    }
    for (const Rule& r : added) {
      rules_.push_back(r);
    }
    new_rules_ = rules_;
  }

  void rm_long_rules() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<Rule> added;

    for (auto& r : rules_) {
      if (r.right_.size() <= 2) {
        continue;
      }
      std::vector<Symbol> rhs;
      for (const auto& s : r.right_) {
        rhs.push_back(s);
      }
      r.right_.clear();
      noterms_.emplace_back(Noterminal('#'));
      Noterminal* cur_nt_ptr = &noterms_.back();
      r.right_.push_back(rhs[0]);
      r.right_.emplace_back(Symbol::Kind::noterminal, cur_nt_ptr->real_value_,
                            cur_nt_ptr->value_);
      for (std::size_t i = 1; i + 2 < rhs.size(); ++i) {
        noterms_.emplace_back(Noterminal('#'));
        Noterminal* next_nt_ptr = &noterms_.back();
        Rule mid;
        mid.left_ = cur_nt_ptr;
        mid.right_.push_back(rhs[i]);
        mid.right_.emplace_back(Symbol::Kind::noterminal,
                                next_nt_ptr->real_value_, next_nt_ptr->value_);
        added.push_back(mid);
        cur_nt_ptr = next_nt_ptr;
      }
      Rule last;
      last.left_ = cur_nt_ptr;
      last.right_.push_back(rhs[rhs.size() - 2]);
      last.right_.push_back(rhs[rhs.size() - 1]);
      added.push_back(last);
    }
    for (const auto& r : added) {
      rules_.push_back(r);
    }
    new_rules_ = rules_;
  }

  void rm_eps_rules() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<char> nullable(noterms_.size(), 0);
    bool has_changed = true;
    while (has_changed) {
      has_changed = false;
      for (const auto& r : rules_) {
        if (nullable[r.left_->value_]) {
          continue;
        }
        if (r.right_.empty()) {
          nullable[r.left_->value_] = 1;
          has_changed = true;
          continue;
        }
        bool all_nullable = true;
        for (const auto& s : r.right_) {
          if (s.kind_ == Symbol::Kind::terminal) {
            all_nullable = false;
            break;
          }
          if (s.kind_ == Symbol::Kind::noterminal && !nullable[s.value_]) {
            all_nullable = false;
            break;
          }
        }
        if (all_nullable) {
          nullable[r.left_->value_] = 1;
          has_changed = true;
        }
      }
    }
    if (nullable[start_->value_]) {
      has_eps = true;
    }

    new_rules_.clear();
    for (const auto& r : rules_) {
      if (r.right_.empty()) {
        continue;
      }
      if (r.right_.size() == 1) {
        new_rules_.push_back(r);
        continue;
      }

      if (r.right_.size() == 2) {
        const Symbol& b = r.right_[0];
        const Symbol& c = r.right_[1];
        new_rules_.push_back(r);

        bool is_b_nullable =
            b.kind_ == Symbol::Kind::noterminal && nullable[b.value_];
        bool is_c_nullable =
            c.kind_ == Symbol::Kind::noterminal && nullable[c.value_];
        if (is_b_nullable && !is_c_nullable) {
          Rule new_rule;
          new_rule.left_ = r.left_;
          new_rule.right_.push_back(c);
          new_rules_.push_back(new_rule);
        } else if (!is_b_nullable && is_c_nullable) {
          Rule new_rule;
          new_rule.left_ = r.left_;
          new_rule.right_.push_back(b);
          new_rules_.push_back(new_rule);
        } else if (is_c_nullable && is_b_nullable) {
          Rule new_rule1;
          new_rule1.left_ = r.left_;
          new_rule1.right_.push_back(b);
          new_rules_.push_back(new_rule1);

          Rule new_rule2;
          new_rule2.left_ = r.left_;
          new_rule2.right_.push_back(c);
          new_rules_.push_back(new_rule2);
        }
        continue;
      }
    }
    rules_ = new_rules_;
  }

  void add_epsilon() {
    if (has_eps) {
      Rule r;
      noterms_.emplace_back(Noterminal('&'));
      r.left_ = &noterms_.back();
      rules_.push_back(r);
      r.right_.emplace_back(Symbol::Kind::noterminal, start_->real_value_,
                            start_->value_);
      rules_.push_back(r);
      new_rules_ = rules_;
      start_ = &noterms_.back();
    }
  }

  void rm_chained_rules() {
    if (noterms_.empty()) {
      rules_.clear();
      new_rules_.clear();
      return;
    }
    std::vector<std::vector<bool>> chain(noterms_.size(),
                                         std::vector<bool>(noterms_.size(), 0));
    for (int i = 0; i < noterms_.size(); ++i) {
      chain[i][i] = true;
    }
    for (const auto& r : rules_) {
      if (r.right_.size() == 1 &&
          r.right_[0].kind_ == Symbol::Kind::noterminal) {
        chain[r.left_->value_][r.right_[0].value_] = true;
      }
    }
    bool has_changed = true;
    while (has_changed) {
      has_changed = false;
      for (int i = 0; i < noterms_.size(); ++i) {
        for (int j = 0; j < noterms_.size(); ++j) {
          if (!chain[i][j]) {
            continue;
          }
          for (int k = 0; k < noterms_.size(); ++k) {
            if (chain[j][k] && !chain[i][k]) {
              chain[i][k] = true;
              has_changed = true;
            }
          }
        }
      }
    }
    new_rules_.clear();
    for (int i = 0; i < noterms_.size(); ++i) {
      for (const auto& r : rules_) {
        if (!chain[i][r.left_->value_]) {
          continue;
        }
        if (r.right_.size() == 1 &&
            r.right_[0].kind_ == Symbol::Kind::noterminal) {
          continue;
        }
        Rule new_rule;
        new_rule.left_ = &noterms_[i];
        new_rule.right_ = r.right_;
        new_rules_.push_back(new_rule);
      }
    }

    rules_ = new_rules_;
  }
};

inline Grammar read_input(const std::string& filename,
                   std::vector<std::string>& words) {
  Noterminal::counter = 0;
  Terminal::counter = 0;
  std::ifstream fin(filename);
  if (!fin) {
    throw std::runtime_error("Cannot open file: " + filename);
  }
  int nonterm_count;
  int term_count;
  int rules_count;
  if (!(fin >> nonterm_count >> term_count >> rules_count)) {
    throw std::runtime_error(
        "failed to read counts of rules, terms or noterms");
  }
  Grammar G;
  G.rules_.resize(rules_count);
  std::vector<int> nonterm_index(256, -1);
  std::vector<int> term_index(256, -1);

  char c;
  for (int i = 0; i < nonterm_count; ++i) {
    if (!(fin >> c)) {
      throw std::runtime_error("failed to read nonterminal");
    }
    G.noterms_.emplace_back(c, true);
    nonterm_index[c] = i;
  }

  for (int i = 0; i < term_count; ++i) {
    if (!(fin >> c)) {
      throw std::runtime_error("failed to read terminal");
    }
    G.terminals_.emplace_back(c);
    term_index[c] = i;
  }

  std::string line;
  getline(fin, line);
  for (int i = 0; i < rules_count; ++i) {
    if (!std::getline(fin, line)) {
      throw std::runtime_error("failed to read rule line");
    }
    if (line.empty()) {
      --i;
      continue;
    }
    std::vector<Symbol> tokens;
    for (char ch : line) {
      if (std::isspace(static_cast<unsigned char>(ch))) {
        continue;
      }
      if (ch == '-' || ch == '>') {
        tokens.emplace_back(Symbol::Kind::other, ch, -1);
      } else if (nonterm_index[ch] != -1) {
        const Noterminal& nt = G.noterms_[nonterm_index[ch]];
        tokens.emplace_back(Symbol::Kind::noterminal, nt.real_value_,
                            nt.value_);
      } else if (term_index[ch] != -1) {
        const Terminal& t = G.terminals_[term_index[ch]];
        tokens.emplace_back(Symbol::Kind::terminal, t.real_value_, t.value_);
      } else {
        throw std::runtime_error("Unknown symbol in rule: " +
                                 std::string(1, ch));
      }
    }
    if (tokens.size() < 3) {
      throw std::runtime_error("Rule is too short: " + line);
    }
    if (tokens[0].kind_ != Symbol::Kind::noterminal ||
        tokens[1].real_value_ != '-' || tokens[2].real_value_ != '>') {
      throw std::runtime_error("Rule is not formated as A -> a: " + line);
    }
    Rule rule;
    {
      char left_char = tokens[0].real_value_;
      if (nonterm_index[left_char] < 0) {
        throw std::runtime_error("Left-hand nonterminal not declared");
      }
      rule.left_ = &G.noterms_[nonterm_index[left_char]];
    }

    for (std::size_t j = 3; j < tokens.size(); ++j) {
      rule.right_.push_back(tokens[j]);
    }
    G.rules_[i] = rule;
  }

  char start;
  if (!(fin >> start)) {
    throw std::runtime_error("failed to read start symbol");
  }
  G.start_ = &G.noterms_[nonterm_index[start]];

  int m;
  if (!(fin >> m) or m < 1) {
    throw std::runtime_error("failed to read number of words");
  }

  words.clear();
  for (int i = 0; i < m; ++i) {
    std::string w;
    if (!(fin >> w)) {
      throw std::runtime_error("failed to read a word");
    }
    words.push_back(w);
  }
  G.new_rules_ = G.rules_;
  return G;
}

inline std::ostream& operator<<(std::ostream& os, const Grammar& g) {
  auto print_noterm = [&](int id) {
    const Noterminal& nt = g.noterms_[id];
    if (nt.isInitial_) {
      os << nt.real_value_;
    } else {
      os << '(' << nt.value_ << ')';
    }
  };
  for (const auto& r : g.rules_) {
    print_noterm(r.left_->value_);
    os << "->";
    if (r.right_.empty()) {
      os << '\n';
      continue;
    }
    for (const auto& s : r.right_) {
      if (s.kind_ == Symbol::Kind::terminal) {
        os << s.real_value_;
      } else if (s.kind_ == Symbol::Kind::noterminal) {
        print_noterm(s.value_);
      } else {
      }
    }
    os << '\n';
  }
  return os;
}
