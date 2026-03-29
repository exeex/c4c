// Mirror of ref/claudes-c-compiler/src/backend/linker_common/archive.rs.
#include "mod.hpp"

#include <algorithm>

namespace c4c::backend::linker_common {
namespace {

std::vector<std::string> collect_defined_symbols(const Elf64Object& object) {
  std::vector<std::string> names;
  for (const auto& symbol : object.symbols) {
    if (symbol.name.empty() || symbol.is_undefined() || symbol.is_local()) continue;
    if (symbol.sym_type() == elf::STT_SECTION) continue;
    names.push_back(symbol.name);
  }
  return names;
}

}  // namespace

bool ArchiveMember::defines_symbol(const std::string& symbol_name) const {
  return std::find(defined_symbols.begin(), defined_symbols.end(), symbol_name) !=
         defined_symbols.end();
}

std::optional<std::size_t> ElfArchive::find_member_index_for_symbol(
    const std::string& symbol_name) const {
  for (std::size_t index = 0; index < members.size(); ++index) {
    if (members[index].defines_symbol(symbol_name)) return index;
  }
  return std::nullopt;
}

std::optional<ElfArchive> parse_elf64_archive(const std::vector<std::uint8_t>& data,
                                              const std::string& source_name,
                                              std::uint16_t expected_machine,
                                              std::string* error) {
  const auto fail = [&](const std::string& message) -> std::optional<ElfArchive> {
    if (error != nullptr) *error = message;
    return std::nullopt;
  };

  if (error != nullptr) error->clear();

  std::string archive_error;
  const auto member_refs = elf::parse_archive_members(data, &archive_error);
  if (!member_refs.has_value()) {
    return fail(source_name + ": " + archive_error);
  }

  std::vector<ArchiveMember> members;
  members.reserve(member_refs->size());
  for (const auto& member_ref : *member_refs) {
    const auto begin = data.begin() + static_cast<std::ptrdiff_t>(member_ref.data_offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(member_ref.data_size);
    std::vector<std::uint8_t> member_data(begin, end);

    ArchiveMember member{
        .name = member_ref.name,
        .data_offset = member_ref.data_offset,
        .data_size = member_ref.data_size,
    };
    if (member_data.size() >= elf::ELF_MAGIC.size() &&
        std::equal(elf::ELF_MAGIC.begin(), elf::ELF_MAGIC.end(), member_data.begin())) {
      const auto full_name = source_name + "(" + member_ref.name + ")";
      std::string object_error;
      auto object = parse_elf64_object(member_data, full_name, expected_machine, &object_error);
      if (!object.has_value()) {
        return fail(object_error);
      }
      member.defined_symbols = collect_defined_symbols(*object);
      member.object = std::move(object);
    }
    members.push_back(std::move(member));
  }

  return ElfArchive{
      .members = std::move(members),
      .source_name = source_name,
  };
}

}  // namespace c4c::backend::linker_common
