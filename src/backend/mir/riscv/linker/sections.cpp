// Translated from src/backend/riscv/linker/sections.rs
// Self-contained section merging pass for the RISC-V linker.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {
namespace {

constexpr std::uint32_t kShtSymtab = 2;
constexpr std::uint32_t kShtStrtab = 3;
constexpr std::uint32_t kShtRela = 4;
constexpr std::uint32_t kShtGroup = 17;
constexpr std::uint32_t kShtNobits = 8;
constexpr std::uint32_t kShtRiscvAttributes = 0x70000003;

constexpr std::uint64_t kShfWrite = 0x1;
constexpr std::uint64_t kShfAlloc = 0x2;
constexpr std::uint64_t kShfExecinstr = 0x4;
constexpr std::uint64_t kShfTls = 0x400;

struct Section {
  std::string name;
  std::uint32_t sh_type = 0;
  std::uint64_t flags = 0;
  std::uint64_t addralign = 1;
  std::uint64_t size = 0;
};

struct ElfObject {
  std::vector<Section> sections;
  std::vector<std::vector<std::uint8_t>> section_data;
};

bool starts_with(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::string section_basename(std::string_view name) {
  return std::string(name);
}

std::optional<std::string> output_section_name(std::string_view name,
                                               std::uint32_t sh_type,
                                               std::uint64_t sh_flags) {
  if ((sh_flags & kShfAlloc) == 0 && sh_type != kShtRiscvAttributes) {
    return std::nullopt;
  }
  if (sh_type == kShtGroup) {
    return std::nullopt;
  }

  const std::pair<std::string_view, std::string_view> prefixes[] = {
      {".text.", ".text"},
      {".rodata.", ".rodata"},
      {".data.rel.ro", ".data"},
      {".data.", ".data"},
      {".bss.", ".bss"},
      {".tdata.", ".tdata"},
      {".tbss.", ".tbss"},
      {".sdata.", ".sdata"},
      {".sbss.", ".sbss"},
  };

  for (const auto& [prefix, output] : prefixes) {
    if (starts_with(name, prefix)) {
      return std::string(output);
    }
  }

  for (std::string_view arr : {".init_array", ".fini_array", ".preinit_array"}) {
    if (name == arr || starts_with(name, std::string(arr) + ".")) {
      return std::string(arr);
    }
  }

  return section_basename(name);
}

}  // namespace

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = 0;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
  std::uint64_t align = 1;
};

struct InputSecRef {
  std::size_t obj_idx = 0;
  std::size_t sec_idx = 0;
  std::size_t merged_sec_idx = 0;
  std::uint64_t offset_in_merged = 0;
};

std::tuple<std::vector<MergedSection>,
           std::unordered_map<std::string, std::size_t>,
           std::vector<InputSecRef>>
merge_sections(const std::vector<std::pair<std::string, ElfObject>>& input_objs) {
  std::vector<MergedSection> merged_sections;
  std::unordered_map<std::string, std::size_t> merged_map;
  std::vector<InputSecRef> input_sec_refs;

  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const ElfObject& obj = input_objs[obj_idx].second;
    for (std::size_t sec_idx = 0; sec_idx < obj.sections.size(); ++sec_idx) {
      const Section& sec = obj.sections[sec_idx];
      if (sec.name.empty() || sec.sh_type == kShtSymtab || sec.sh_type == kShtStrtab ||
          sec.sh_type == kShtRela || sec.sh_type == kShtGroup) {
        continue;
      }

      const std::optional<std::string> out_name =
          output_section_name(sec.name, sec.sh_type, sec.flags);
      if (!out_name.has_value()) {
        continue;
      }

      const std::vector<std::uint8_t>& sec_data = obj.section_data[sec_idx];

      if (*out_name == ".note.GNU-stack") {
        continue;
      }

      if (*out_name == ".riscv.attributes" || starts_with(*out_name, ".note.")) {
        if (merged_map.find(*out_name) == merged_map.end()) {
          const std::size_t merged_idx = merged_sections.size();
          merged_map.emplace(*out_name, merged_idx);
          merged_sections.push_back(MergedSection{
              *out_name,
              sec.sh_type,
              sec.flags,
              sec_data,
              0,
              std::max<std::uint64_t>(1, sec.addralign),
          });
          input_sec_refs.push_back(InputSecRef{
              obj_idx,
              sec_idx,
              merged_idx,
              0,
          });
        }
        continue;
      }

      const auto existing = merged_map.find(*out_name);
      std::size_t merged_idx = 0;
      if (existing != merged_map.end()) {
        merged_idx = existing->second;
      } else {
        const bool is_bss = sec.sh_type == kShtNobits || *out_name == ".bss" ||
                            *out_name == ".sbss";
        merged_idx = merged_sections.size();
        merged_map.emplace(*out_name, merged_idx);
        merged_sections.push_back(MergedSection{
            *out_name,
            is_bss ? kShtNobits : sec.sh_type,
            sec.flags,
            {},
            0,
            std::max<std::uint64_t>(1, sec.addralign),
        });
      }

      MergedSection& ms = merged_sections[merged_idx];
      ms.sh_flags |= sec.flags & (kShfWrite | kShfAlloc | kShfExecinstr | kShfTls);
      ms.align = std::max(ms.align, std::max<std::uint64_t>(1, sec.addralign));

      const std::size_t align = static_cast<std::size_t>(std::max<std::uint64_t>(1, sec.addralign));
      const std::size_t cur_len = ms.data.size();
      const std::size_t aligned = (cur_len + align - 1) & ~(align - 1);
      if (aligned > cur_len) {
        ms.data.resize(aligned, 0);
      }
      const std::uint64_t offset_in_merged = static_cast<std::uint64_t>(ms.data.size());

      if (sec.sh_type == kShtNobits) {
        ms.data.resize(ms.data.size() + static_cast<std::size_t>(sec.size), 0);
      } else {
        ms.data.insert(ms.data.end(), sec_data.begin(), sec_data.end());
      }

      input_sec_refs.push_back(InputSecRef{
          obj_idx,
          sec_idx,
          merged_idx,
          offset_in_merged,
      });
    }
  }

  return {std::move(merged_sections), std::move(merged_map), std::move(input_sec_refs)};
}

}  // namespace c4c::backend::riscv::linker
