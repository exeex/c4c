#include "src/backend/prealloc/prepared_fact_boundary.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << '\n';
  return 1;
}

int statuses_are_explicit_and_fail_closed() {
  const prepare::PreparedFactBoundaryEvidence positive{
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .function_name = c4c::FunctionNameId{1},
      .block_label = c4c::BlockLabelId{2},
      .value_name = c4c::ValueNameId{3},
      .instruction_index = 4,
  };
  if (!positive || prepare::prepared_fact_boundary_status_name(positive.status) !=
                       "available") {
    return fail("expected a positive identity-bound prepared fact");
  }

  for (const auto status : {prepare::PreparedFactBoundaryStatus::Missing,
                            prepare::PreparedFactBoundaryStatus::Incomplete,
                            prepare::PreparedFactBoundaryStatus::Ambiguous,
                            prepare::PreparedFactBoundaryStatus::Mismatched,
                            prepare::PreparedFactBoundaryStatus::Unsupported}) {
    const prepare::PreparedFactBoundaryEvidence negative{.status = status};
    if (negative) {
      return fail("expected negative named input to fail closed");
    }
  }
  return 0;
}

int public_headers_have_only_inventoried_compatibility_payloads() {
  // Step 1 inventory: these are legacy public compatibility/proof payloads.
  // The guard makes additions fail while their owning producer seams migrate.
  const std::unordered_map<std::string, std::size_t> expected_hits{
      {"publication_plans.hpp", 6},
      {"value_locations.hpp", 6},
  };
  const std::regex forbidden(
      R"(\b(Route[0-9][A-Za-z0-9_]*(Record|Index)|route[0-9]_[A-Za-z0-9_]+)\b)");
  std::unordered_map<std::string, std::size_t> actual_hits;

  for (const auto& entry : std::filesystem::recursive_directory_iterator(
           C4C_PREALLOC_SOURCE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".hpp") {
      continue;
    }
    std::ifstream input(entry.path());
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    actual_hits[entry.path().filename().string()] +=
        std::distance(std::sregex_iterator(source.begin(), source.end(), forbidden),
                      std::sregex_iterator());
  }
  for (const auto& [file, count] : actual_hits) {
    const auto expected = expected_hits.find(file);
    if (count != 0 && (expected == expected_hits.end() || expected->second != count)) {
      return fail("public prepared header gained an uninventoried route record or index");
    }
  }
  for (const auto& [file, count] : expected_hits) {
    if (actual_hits[file] != count) {
      return fail("prepared route compatibility inventory changed; update by migration");
    }
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = statuses_are_explicit_and_fail_closed(); status != 0) {
    return status;
  }
  return public_headers_have_only_inventoried_compatibility_payloads();
}
