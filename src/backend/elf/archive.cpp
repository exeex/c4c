// Mirror of ref/claudes-c-compiler/src/backend/elf/archive.rs.
#include "mod.hpp"

#include <string>

namespace c4c::backend::elf {
namespace {

constexpr char kArchiveMagic[] = "!<arch>\n";
constexpr std::size_t kArchiveMagicSize = 8;
constexpr std::size_t kArchiveHeaderSize = 60;

bool range_fits(std::size_t offset, std::size_t size, std::size_t total) {
  return offset <= total && size <= total - offset;
}

std::string trim_right_spaces(const std::string& value) {
  const auto end = value.find_last_not_of(' ');
  if (end == std::string::npos) return {};
  return value.substr(0, end + 1);
}

}  // namespace

bool is_archive_file(const std::vector<std::uint8_t>& data) {
  return data.size() >= kArchiveMagicSize &&
         std::equal(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(kArchiveMagicSize),
                    kArchiveMagic);
}

std::optional<std::vector<ArchiveMemberRef>> parse_archive_members(
    const std::vector<std::uint8_t>& data,
    std::string* error) {
  const auto fail = [&](const std::string& message)
      -> std::optional<std::vector<ArchiveMemberRef>> {
    if (error != nullptr) *error = message;
    return std::nullopt;
  };

  if (error != nullptr) error->clear();

  if (!is_archive_file(data)) {
    return fail("not a valid archive file");
  }

  std::vector<ArchiveMemberRef> members;
  std::size_t offset = kArchiveMagicSize;
  while (offset < data.size()) {
    if (!range_fits(offset, kArchiveHeaderSize, data.size())) {
      return fail("archive member header out of bounds");
    }

    const std::string raw_name(
        data.begin() + static_cast<std::ptrdiff_t>(offset),
        data.begin() + static_cast<std::ptrdiff_t>(offset + 16));
    const std::string size_text(
        data.begin() + static_cast<std::ptrdiff_t>(offset + 48),
        data.begin() + static_cast<std::ptrdiff_t>(offset + 58));
    if (data[offset + 58] != '`' || data[offset + 59] != '\n') {
      return fail("invalid archive member trailer");
    }

    std::size_t member_size = 0;
    try {
      member_size = static_cast<std::size_t>(std::stoull(trim_right_spaces(size_text)));
    } catch (const std::exception&) {
      return fail("invalid archive member size");
    }

    const auto data_offset = offset + kArchiveHeaderSize;
    if (!range_fits(data_offset, member_size, data.size())) {
      return fail("archive member data out of bounds");
    }

    auto member_name = trim_right_spaces(raw_name);
    if (member_name == "/" || member_name == "/SYM64/" || member_name == "//") {
      offset = data_offset + member_size;
      if ((offset & 1u) != 0u) ++offset;
      continue;
    }
    if (!member_name.empty() && member_name.back() == '/') {
      member_name.pop_back();
    }

    members.push_back(ArchiveMemberRef{
        .name = std::move(member_name),
        .data_offset = data_offset,
        .data_size = member_size,
    });

    offset = data_offset + member_size;
    if ((offset & 1u) != 0u) ++offset;
  }

  return members;
}

}  // namespace c4c::backend::elf
