#include "CYK.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input_file>\n";
    return 1;
  }

  const char* filename = argv[1];
  std::vector<std::string> words;
  try {
    Grammar G = read_input(filename, words);
    CYKParser parser = CYKParser::fit(G);

    for (const auto& word : words) {
      bool ok = parser.predict(word);
      std::cout << (ok ? "Yes" : "No") << '\n';
    }
  } catch (const std::exception& e) {
    std::cerr << "Input error: " << e.what() << '\n';
    return 1;
  }
}
