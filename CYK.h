#include "Grammar.h"

struct CYKParser {
  const Grammar* grammar_;
  std::vector<std::vector<std::vector<bool>>> dp_table_;
  size_t word_length_ = 0;

  explicit CYKParser(const Grammar& grammar) : grammar_(&grammar) {}

  static CYKParser fit(Grammar& G){
    G.rm_nongenerating_noterms();
    G.rm_unreachable_noterms();
    G.rm_mixed_rules();
    G.rm_long_rules();
    G.rm_eps_rules();
    G.rm_chained_rules();
    return CYKParser(G);
  }

  void process_words(int len) {
    for (int start = 0; start + len <= word_length_; ++start) {
      int end = start + len;
      for (const Rule& r : grammar_->rules_) {
        if (r.right_.size() != 2) {
          continue;
        }
        const Symbol& b = r.right_[0];
        const Symbol& c = r.right_[1];
        for (int mid = start + 1; mid < end; ++mid) {
          if (dp_table_[b.value_][start][mid] &&
              dp_table_[c.value_][mid][end]) {
            dp_table_[r.left_->value_][start][end] = true;
            break;
          }
        }
      }
    }
  }

  bool predict(const std::string& word) {
    if (word.empty()) {
      if (grammar_->has_eps) {
        return true;
      }
      return false;
    }
    dp_table_.assign(
        grammar_->noterms_.size(),
        std::vector(word.size() + 1, std::vector(word.size() + 1, false)));
    word_length_ = word.size();
    for (int i = 0; i < word_length_; ++i) {
      char c = word[i];
      for (const Rule& r : grammar_->rules_) {
        if (r.right_.size() != 1) {
          continue;
        }
        const Symbol& s = r.right_[0];
        if (s.real_value_ == c) {
          dp_table_[r.left_->value_][i][i + 1] = true;
        }
      }
    }

    for (int i = 2; i <= word.size(); ++i) {
      process_words(i);
    }
    return dp_table_[grammar_->start_->value_][0][word_length_];
  }

};
