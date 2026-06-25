#include <iostream>
#include <string_view>

namespace {

void print_help(std::ostream& out) {
  out << "Usage:\n"
      << "  c4c-as [options] <input.s>\n\n"
      << "Options:\n"
      << "  -h, --help       Show this help message\n"
      << "  --version        Print version information\n"
      << "  -o <path>        Output object path\n\n"
      << "Status:\n"
      << "  placeholder only; assembly parsing and object emission are not implemented yet\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 1) {
    print_help(std::cout);
    return 0;
  }

  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      print_help(std::cout);
      return 0;
    }
    if (arg == "--version") {
      std::cout << "c4c-as placeholder\n";
      return 0;
    }
  }

  std::cerr << "c4c-as: assembler input is not implemented yet\n";
  return 1;
}
