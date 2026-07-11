#include <array>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>

namespace {

struct RouteInventory {
  int route;
  std::size_t expected_hits;
  std::string_view semantic_owner;
  std::string_view migration_family;
};

// Step 1 inventory.  Counts are deliberately exact: later migration packets may
// only ratchet them down (and update this inventory), never add new authority.
constexpr std::array<RouteInventory, 8> kInventory{{
    {1, 20, "BIR same-block producer semantics", "named producer view"},
    {2, 0, "BIR select-chain semantics", "named select/dependency view"},
    {3, 27, "BIR memory-access semantics", "named memory-access view"},
    {4, 31, "BIR publication semantics", "named publication view"},
    {5, 133, "BIR edge/join semantics", "named edge/join publication view"},
    {6, 0, "BIR call-boundary semantics", "named call-boundary view"},
    {7, 0, "BIR comparison semantics", "named comparison view"},
    {8, 0, "BIR return/control semantics", "named return/control view"},
}};

std::string read_file(const std::string& path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

std::size_t count_matches(const std::string& text, const std::regex& pattern) {
  return static_cast<std::size_t>(
      std::distance(std::sregex_iterator(text.begin(), text.end(), pattern),
                    std::sregex_iterator()));
}

int fail(const std::string& message) {
  std::cerr << "backend common MIR query route-authority guard: " << message
            << '\n';
  return 1;
}

}  // namespace

int main() {
  const std::string root = C4C_SOURCE_DIR;
  const auto header = read_file(root + "/src/backend/mir/query.hpp");
  const auto implementation = read_file(root + "/src/backend/mir/query.cpp");
  if (header.empty() || implementation.empty()) {
    return fail("could not read src/backend/mir/query.{hpp,cpp}");
  }
  const auto common_boundary = header + "\n" + implementation;

  // Direct route headers are never an allowed common-query dependency.
  const std::regex route_header{R"(#\s*include\s*[<\"][^>\"]*route[1-8][^>\"]*[>\"])",
                                std::regex::icase};
  if (std::regex_search(common_boundary, route_header)) {
    return fail("direct route header detected");
  }

  for (const auto& entry : kInventory) {
    const auto spelling = std::string{"(Route"} +
                          std::to_string(entry.route) +
                          R"([A-Za-z0-9_]*|route)" +
                          std::to_string(entry.route) + R"(_[A-Za-z0-9_]*)" + ")";
    const auto hits = count_matches(common_boundary, std::regex{spelling});
    if (hits != entry.expected_hits) {
      return fail("Route " + std::to_string(entry.route) + " inventory drift: " +
                  std::to_string(hits) + " hits, expected " +
                  std::to_string(entry.expected_hits) + "; owner=" +
                  std::string{entry.semantic_owner} + "; migration=" +
                  std::string{entry.migration_family});
    }
  }

  // The sole current public route payload is the Route 5 edge/join index.
  // This check catches route records, indexes, and route-return wrappers added
  // under otherwise generic common-query names.
  const std::regex public_route_payload{R"(\b(bir::)?Route[1-8][A-Za-z0-9_]*\b)"};
  if (count_matches(header, public_route_payload) != 1 ||
      header.find("bir::Route5EdgeJoinSourceIndex") == std::string::npos) {
    return fail("public route payload changed; expected only Route5EdgeJoinSourceIndex");
  }

  const std::regex route_record_or_index{R"(\bRoute[1-8][A-Za-z0-9_]*(Record|Index)\b)"};
  const std::regex route_analysis_entry{R"(\broute[1-8]_(build|find|evaluate|current)[A-Za-z0-9_]*\b)"};
  const std::regex hidden_route_return{R"(\bfind_bir_[A-Za-z0-9_]*\s*\()"};
  if (count_matches(common_boundary, route_record_or_index) == 0 ||
      count_matches(implementation, route_analysis_entry) == 0 ||
      count_matches(implementation, hidden_route_return) == 0) {
    return fail("guard self-check did not detect records/indexes, analysis entry points, and hidden route-return wrappers");
  }

  return 0;
}
