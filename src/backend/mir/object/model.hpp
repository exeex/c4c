#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::mir::object {

struct SectionId {
  std::size_t value = 0;

  friend bool operator==(SectionId lhs, SectionId rhs) {
    return lhs.value == rhs.value;
  }
  friend bool operator!=(SectionId lhs, SectionId rhs) {
    return !(lhs == rhs);
  }
};

struct SymbolId {
  std::size_t value = 0;

  friend bool operator==(SymbolId lhs, SymbolId rhs) {
    return lhs.value == rhs.value;
  }
  friend bool operator!=(SymbolId lhs, SymbolId rhs) {
    return !(lhs == rhs);
  }
};

struct LabelId {
  std::size_t value = 0;

  friend bool operator==(LabelId lhs, LabelId rhs) {
    return lhs.value == rhs.value;
  }
  friend bool operator!=(LabelId lhs, LabelId rhs) {
    return !(lhs == rhs);
  }
};

enum class SectionKind {
  Text,
  Data,
  Bss,
  Custom,
};

enum class SymbolBinding {
  Local,
  Global,
};

enum class SymbolKind {
  NoType,
  Function,
  Object,
  Section,
};

enum class RelocationBase {
  SectionOffset,
  Label,
  Symbol,
};

struct SectionRecord {
  SectionId id{};
  std::string name;
  SectionKind kind = SectionKind::Custom;
  std::vector<std::uint8_t> bytes;
  std::uint64_t size_bytes = 0;
  std::uint64_t align_bytes = 1;
  bool alloc = true;
  bool executable = false;
  bool writable = false;
};

struct SymbolRecord {
  SymbolId id{};
  std::string name;
  SymbolBinding binding = SymbolBinding::Local;
  SymbolKind kind = SymbolKind::NoType;
  std::optional<SectionId> section;
  std::uint64_t value = 0;
  std::uint64_t size_bytes = 0;
};

struct LabelRecord {
  LabelId id{};
  std::string name;
  SectionId section{};
  std::uint64_t offset = 0;
};

struct RelocationRecord {
  SectionId section{};
  std::uint64_t offset = 0;
  std::uint32_t type = 0;
  SymbolId symbol{};
  std::int64_t addend = 0;
  RelocationBase base = RelocationBase::Symbol;
  std::optional<LabelId> label;
};

struct ObjectModule {
  std::vector<SectionRecord> sections;
  std::vector<SymbolRecord> symbols;
  std::vector<LabelRecord> labels;
  std::vector<RelocationRecord> relocations;
};

std::optional<SectionId> find_section_id(const ObjectModule& module,
                                         std::string_view name);
SectionRecord* find_section(ObjectModule& module, SectionId id);
const SectionRecord* find_section(const ObjectModule& module, SectionId id);
SectionRecord* find_section(ObjectModule& module, std::string_view name);
const SectionRecord* find_section(const ObjectModule& module,
                                  std::string_view name);
SectionRecord& create_section(ObjectModule& module, std::string name,
                              SectionKind kind, std::uint64_t align_bytes,
                              bool alloc, bool executable, bool writable);
SectionRecord& get_or_create_section(ObjectModule& module, std::string name,
                                     SectionKind kind,
                                     std::uint64_t align_bytes, bool alloc,
                                     bool executable, bool writable);

std::uint64_t append_section_bytes(SectionRecord& section,
                                   const std::vector<std::uint8_t>& bytes);
std::uint64_t append_section_bytes(SectionRecord& section,
                                   std::initializer_list<std::uint8_t> bytes);
std::uint64_t reserve_section_bytes(SectionRecord& section,
                                    std::uint64_t size_bytes);
std::uint64_t align_section(SectionRecord& section, std::uint64_t align_bytes,
                            std::uint8_t fill_byte = 0);

std::optional<SymbolId> find_symbol_id(const ObjectModule& module,
                                       std::string_view name);
SymbolRecord* find_symbol(ObjectModule& module, SymbolId id);
const SymbolRecord* find_symbol(const ObjectModule& module, SymbolId id);
SymbolRecord* find_symbol(ObjectModule& module, std::string_view name);
const SymbolRecord* find_symbol(const ObjectModule& module,
                                std::string_view name);
SymbolRecord& define_symbol(ObjectModule& module, std::string name,
                            SymbolBinding binding, SymbolKind kind,
                            SectionId section, std::uint64_t value,
                            std::uint64_t size_bytes = 0);
SymbolRecord& declare_undefined_symbol(ObjectModule& module, std::string name,
                                       SymbolBinding binding, SymbolKind kind,
                                       std::uint64_t size_bytes = 0);

LabelRecord& bind_label(ObjectModule& module, std::string name,
                        SectionId section, std::uint64_t offset);
LabelRecord& bind_label_at_current_offset(ObjectModule& module,
                                          std::string name,
                                          const SectionRecord& section);

RelocationRecord& attach_relocation(ObjectModule& module, SectionId section,
                                    std::uint64_t offset, std::uint32_t type,
                                    SymbolId symbol, std::int64_t addend = 0);
RelocationRecord& attach_label_relocation(ObjectModule& module,
                                          SectionId section,
                                          std::uint64_t offset,
                                          std::uint32_t type, SymbolId symbol,
                                          LabelId label,
                                          std::int64_t addend = 0);

const char* section_kind_name(SectionKind kind);
const char* symbol_binding_name(SymbolBinding binding);
const char* symbol_kind_name(SymbolKind kind);
const char* relocation_base_name(RelocationBase base);

bool is_undefined_symbol(const SymbolRecord& symbol);
bool is_section_local_label(const LabelRecord& label, const SectionRecord& section);

}  // namespace c4c::backend::mir::object
