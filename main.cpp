#include "solve.h"
#include <iostream>
#include <string>

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::string regular;
  char x;
  if (!(std::cin >> regular >> x)) {
    std::cerr << "format: <RPN> <x>\n";
    return 1;
  }
  try {
    Answer r = Solve(regular, x);
    if (r.isInf) {
      std::cout << "INF\n";
    } else {
      std::cout << r.value << "\n";
    }
    return 0;
  } catch (const SyntaxError& e) {
    std::cerr << "syntax error at pos " << e.pos << " token='"
              << (e.token ? e.token : '?') << "': " << e.what() << "\n";
    return 2;
  }
}
