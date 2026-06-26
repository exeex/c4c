#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

constexpr std::uint16_t kEtRel = 1;
constexpr std::uint16_t kEmRiscv = 243;
constexpr std::uint32_t kShtSymtab = 2;
constexpr std::uint32_t kShtRela = 4;
constexpr std::uint64_t kShfExecinstr = 0x4;
constexpr std::uint8_t kStbLocal = 0;
constexpr std::uint8_t kStbGlobal = 1;
constexpr std::uint8_t kSttNoType = 0;
constexpr std::uint8_t kSttFunc = 2;
constexpr std::uint16_t kShnUndef = 0;
constexpr std::uint16_t kElf64Shentsize = 64;
constexpr std::uint16_t kElf64Symentsize = 24;

struct CliOptions {
  std::string input_path;
  std::string output_path;
};

struct SectionHeader {
  std::uint32_t name_offset = 0;
  std::uint32_t type = 0;
  std::uint64_t flags = 0;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint32_t link = 0;
  std::uint32_t info = 0;
  std::uint64_t entsize = 0;
  std::string name;
};

struct Symbol {
  std::string name;
  std::uint8_t info = 0;
  std::uint16_t section_index = 0;
  std::uint64_t value = 0;
  std::uint64_t size = 0;
};

struct ObjectLabel {
  std::string name;
  std::uint64_t offset = 0;
};

struct ExtractedObject {
  std::string function_name;
  std::vector<std::uint8_t> text_bytes;
  std::vector<ObjectLabel> local_labels;
};

struct DecodedInsnD {
  std::uint32_t major = 0;
  std::uint32_t operation = 0;
  std::uint32_t destination = 0;
  std::uint32_t lhs = 0;
  std::uint32_t rhs = 0;
  std::uint32_t accumulator = 0;
  std::uint32_t dtype = 0;
};

struct DecodedLi {
  std::uint32_t destination = 0;
  std::int32_t immediate = 0;
};

struct DecodedRet {};

enum class Rv64DecodedFormat {
  RType,
  IType,
  Load,
  Store,
  UType,
  Jalr,
  Branch,
  Jal,
};

struct DecodedRv64I {
  Rv64DecodedFormat format = Rv64DecodedFormat::IType;
  std::string_view mnemonic;
  std::uint32_t destination = 0;
  std::uint32_t lhs = 0;
  std::uint32_t rhs = 0;
  std::int32_t immediate = 0;
  std::string target_label;
};

using DecodedInstruction =
    std::variant<DecodedInsnD, DecodedLi, DecodedRet, DecodedRv64I>;

struct DecodedLine {
  std::uint64_t offset = 0;
  DecodedInstruction instruction;
};

void print_help(std::ostream& out) {
  out << "Usage:\n"
      << "  c4c-objdump [options] <input.o>\n\n"
      << "Options:\n"
      << "  -h, --help       Show this help message\n"
      << "  --version        Print version information\n"
      << "  -o <path>        Output assembly path\n\n"
      << "Status:\n"
      << "  extracts the supported RV64 relocatable ELF .text envelope and reports\n"
      << "  the initial supported decoder output\n";
}

std::optional<CliOptions> parse_cli(int argc, char** argv) {
  CliOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      print_help(std::cout);
      return std::nullopt;
    }
    if (arg == "--version") {
      std::cout << "c4c-objdump 0.1\n";
      return std::nullopt;
    }
    if (arg == "-o") {
      if (index + 1 >= argc) {
        std::cerr << "c4c-objdump: error: -o requires an output path\n";
        return std::nullopt;
      }
      if (!options.output_path.empty()) {
        std::cerr << "c4c-objdump: error: duplicate -o output path\n";
        return std::nullopt;
      }
      options.output_path = argv[++index];
      if (options.output_path.empty()) {
        std::cerr << "c4c-objdump: error: -o requires an output path\n";
        return std::nullopt;
      }
      continue;
    }
    if (!arg.empty() && arg.front() == '-') {
      std::cerr << "c4c-objdump: error: unsupported option '" << arg << "'\n";
      return std::nullopt;
    }
    if (!options.input_path.empty()) {
      std::cerr << "c4c-objdump: error: multiple input files are not supported\n";
      return std::nullopt;
    }
    options.input_path = std::string(arg);
  }

  if (options.input_path.empty()) {
    std::cerr << "c4c-objdump: error: missing input file\n";
    return std::nullopt;
  }
  if (options.output_path.empty()) {
    std::cerr << "c4c-objdump: error: object input requires -o <path>\n";
    return std::nullopt;
  }
  return options;
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path, std::string* error) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    if (error != nullptr) {
      *error = "failed to open input file '" + path + "'";
    }
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

bool range_fits(std::uint64_t offset, std::uint64_t size, std::size_t total) {
  return offset <= static_cast<std::uint64_t>(total) &&
         size <= static_cast<std::uint64_t>(total) - offset;
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint32_t value = 0;
  for (int shift = 0; shift < 32; shift += 8) {
    value |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
  }
  return value;
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset++]) << shift;
  }
  return value;
}

std::uint32_t read_u32_at(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_u64_at(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint64_t word = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    word |= static_cast<std::uint64_t>(bytes[offset++]) << shift;
  }
  return word;
}

std::optional<std::string> read_string_at(const std::vector<std::uint8_t>& bytes,
                                          std::uint64_t table_offset,
                                          std::uint64_t table_size,
                                          std::uint32_t string_offset) {
  if (string_offset >= table_size ||
      !range_fits(table_offset + string_offset, 1, bytes.size())) {
    return std::nullopt;
  }
  std::string value;
  std::uint64_t cursor = table_offset + string_offset;
  const std::uint64_t end = table_offset + table_size;
  while (cursor < end) {
    const auto byte = bytes[static_cast<std::size_t>(cursor++)];
    if (byte == 0) {
      return value;
    }
    value.push_back(static_cast<char>(byte));
  }
  return std::nullopt;
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto byte : bytes) {
    out << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return out.str();
}

std::optional<std::vector<SectionHeader>> read_section_headers(
    const std::vector<std::uint8_t>& bytes,
    std::uint64_t section_header_offset,
    std::uint16_t section_header_size,
    std::uint16_t section_count,
    std::uint16_t section_name_index,
    std::string* error) {
  if (section_header_size != kElf64Shentsize || section_count == 0 ||
      section_name_index >= section_count) {
    if (error != nullptr) *error = "unsupported ELF section header table";
    return std::nullopt;
  }
  const std::uint64_t table_size =
      static_cast<std::uint64_t>(section_header_size) * section_count;
  if (!range_fits(section_header_offset, table_size, bytes.size())) {
    if (error != nullptr) *error = "ELF section header table extends past end of file";
    return std::nullopt;
  }

  std::vector<SectionHeader> sections;
  sections.reserve(section_count);
  for (std::uint16_t index = 0; index < section_count; ++index) {
    const auto offset = static_cast<std::size_t>(
        section_header_offset + static_cast<std::uint64_t>(index) * section_header_size);
    sections.push_back(SectionHeader{
        .name_offset = read_u32(bytes, offset),
        .type = read_u32(bytes, offset + 4),
        .flags = read_u64(bytes, offset + 8),
        .offset = read_u64(bytes, offset + 24),
        .size = read_u64(bytes, offset + 32),
        .link = read_u32(bytes, offset + 40),
        .info = read_u32(bytes, offset + 44),
        .entsize = read_u64(bytes, offset + 56),
    });
  }

  const auto& names = sections[section_name_index];
  if (!range_fits(names.offset, names.size, bytes.size())) {
    if (error != nullptr) *error = "ELF section string table extends past end of file";
    return std::nullopt;
  }
  for (auto& section : sections) {
    const auto name = read_string_at(bytes, names.offset, names.size, section.name_offset);
    if (!name.has_value()) {
      if (error != nullptr) *error = "malformed ELF section name";
      return std::nullopt;
    }
    section.name = *name;
  }
  return sections;
}

std::optional<std::vector<Symbol>> read_symbols(const std::vector<std::uint8_t>& bytes,
                                                const std::vector<SectionHeader>& sections,
                                                const SectionHeader& symtab,
                                                std::string* error) {
  if (symtab.entsize != kElf64Symentsize || symtab.link >= sections.size()) {
    if (error != nullptr) *error = "unsupported ELF symbol table";
    return std::nullopt;
  }
  if (!range_fits(symtab.offset, symtab.size, bytes.size())) {
    if (error != nullptr) *error = "ELF symbol table extends past end of file";
    return std::nullopt;
  }
  const auto& strtab = sections[symtab.link];
  if (!range_fits(strtab.offset, strtab.size, bytes.size())) {
    if (error != nullptr) *error = "ELF string table extends past end of file";
    return std::nullopt;
  }

  std::vector<Symbol> symbols;
  const auto count = symtab.size / symtab.entsize;
  symbols.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t index = 0; index < count; ++index) {
    const auto offset = static_cast<std::size_t>(symtab.offset + index * symtab.entsize);
    const auto name_offset = read_u32(bytes, offset);
    const auto name = read_string_at(bytes, strtab.offset, strtab.size, name_offset);
    if (!name.has_value()) {
      if (error != nullptr) *error = "malformed ELF symbol name";
      return std::nullopt;
    }
    symbols.push_back(Symbol{
        .name = *name,
        .info = bytes[offset + 4],
        .section_index = read_u16(bytes, offset + 6),
        .value = read_u64(bytes, offset + 8),
        .size = read_u64(bytes, offset + 16),
    });
  }
  return symbols;
}

std::optional<ExtractedObject> extract_supported_rv64_object(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F') {
    if (error != nullptr) *error = "input is not an ELF file";
    return std::nullopt;
  }
  if (bytes[4] != 2 || bytes[5] != 1 || bytes[6] != 1) {
    if (error != nullptr) *error = "unsupported ELF format; expected ELF64 little-endian";
    return std::nullopt;
  }
  if (read_u16(bytes, 16) != kEtRel) {
    if (error != nullptr) *error = "unsupported ELF type; expected relocatable object";
    return std::nullopt;
  }
  if (read_u16(bytes, 18) != kEmRiscv) {
    if (error != nullptr) *error = "unsupported ELF machine; expected RV64/RISC-V";
    return std::nullopt;
  }

  std::string local_error;
  auto sections = read_section_headers(bytes, read_u64(bytes, 40), read_u16(bytes, 58),
                                       read_u16(bytes, 60), read_u16(bytes, 62),
                                       &local_error);
  if (!sections.has_value()) {
    if (error != nullptr) *error = local_error;
    return std::nullopt;
  }

  const SectionHeader* text = nullptr;
  std::uint16_t text_index = 0;
  const SectionHeader* symtab = nullptr;
  for (std::uint16_t index = 0; index < sections->size(); ++index) {
    const auto& section = (*sections)[index];
    if (section.name == ".text") {
      text = &section;
      text_index = index;
    }
    if (section.type == kShtSymtab) {
      symtab = &section;
    }
  }
  if (text == nullptr) {
    if (error != nullptr) *error = "supported RV64 object has no .text section";
    return std::nullopt;
  }
  if ((text->flags & kShfExecinstr) == 0) {
    if (error != nullptr) *error = "unsupported .text section; missing executable flag";
    return std::nullopt;
  }
  if (!range_fits(text->offset, text->size, bytes.size())) {
    if (error != nullptr) *error = ".text section extends past end of file";
    return std::nullopt;
  }
  if (text->size == 0) {
    if (error != nullptr) *error = "supported RV64 object has empty .text section";
    return std::nullopt;
  }
  for (const auto& section : *sections) {
    if (section.type == kShtRela && section.info == text_index && section.size != 0) {
      if (error != nullptr) *error = "unsupported .text relocations";
      return std::nullopt;
    }
  }
  if (symtab == nullptr) {
    if (error != nullptr) *error = "supported RV64 object has no symbol table";
    return std::nullopt;
  }

  auto symbols = read_symbols(bytes, *sections, *symtab, &local_error);
  if (!symbols.has_value()) {
    if (error != nullptr) *error = local_error;
    return std::nullopt;
  }

  const Symbol* function = nullptr;
  for (const auto& symbol : *symbols) {
    const auto binding = static_cast<std::uint8_t>(symbol.info >> 4);
    const auto type = static_cast<std::uint8_t>(symbol.info & 0x0f);
    if (binding == kStbGlobal && type == kSttFunc &&
        symbol.section_index == text_index && symbol.section_index != kShnUndef &&
        symbol.value == 0 && !symbol.name.empty()) {
      function = &symbol;
      break;
    }
  }
  if (function == nullptr) {
    if (error != nullptr) {
      *error = "supported RV64 object has no global function symbol at .text start";
    }
    return std::nullopt;
  }

  std::vector<ObjectLabel> local_labels;
  for (const auto& symbol : *symbols) {
    const auto binding = static_cast<std::uint8_t>(symbol.info >> 4);
    const auto type = static_cast<std::uint8_t>(symbol.info & 0x0f);
    if (binding != kStbLocal || type != kSttNoType ||
        symbol.section_index != text_index || symbol.name.empty() ||
        symbol.value > text->size ||
        symbol.name == function->name) {
      continue;
    }
    local_labels.push_back(ObjectLabel{
        .name = symbol.name,
        .offset = symbol.value,
    });
  }
  std::sort(local_labels.begin(), local_labels.end(),
            [](const ObjectLabel& lhs, const ObjectLabel& rhs) {
              if (lhs.offset != rhs.offset) {
                return lhs.offset < rhs.offset;
              }
              return lhs.name < rhs.name;
            });

  const auto text_begin = bytes.begin() + static_cast<std::ptrdiff_t>(text->offset);
  const auto text_end = text_begin + static_cast<std::ptrdiff_t>(text->size);
  return ExtractedObject{.function_name = function->name,
                         .text_bytes = std::vector<std::uint8_t>(text_begin, text_end),
                         .local_labels = std::move(local_labels)};
}

std::optional<DecodedInsnD> decode_insn_d(std::uint64_t word) {
  constexpr std::uint64_t kEv64Prefix = 0x3f;
  if ((word & 0x7fu) != kEv64Prefix ||
      ((word >> 12) & 0x7u) != 0 ||
      ((word >> 45) & 0x7u) != 0) {
    return std::nullopt;
  }
  return DecodedInsnD{
      .major = static_cast<std::uint32_t>((word >> 25) & 0x7fu),
      .operation = static_cast<std::uint32_t>((word >> 32) & 0xffu),
      .destination = static_cast<std::uint32_t>((word >> 7) & 0x1fu),
      .lhs = static_cast<std::uint32_t>((word >> 15) & 0x1fu),
      .rhs = static_cast<std::uint32_t>((word >> 20) & 0x1fu),
      .accumulator = static_cast<std::uint32_t>((word >> 40) & 0x1fu),
      .dtype = static_cast<std::uint32_t>((word >> 48) & 0xffffu),
  };
}

std::optional<DecodedLi> decode_li(std::uint32_t word) {
  const auto opcode = word & 0x7fu;
  const auto destination = (word >> 7) & 0x1fu;
  const auto funct3 = (word >> 12) & 0x7u;
  const auto source = (word >> 15) & 0x1fu;
  const auto immediate = static_cast<std::int32_t>(word) >> 20;
  if (opcode != 0x13 || destination != 10 || funct3 != 0 || source != 0 ||
      immediate != 0) {
    return std::nullopt;
  }
  return DecodedLi{.destination = destination, .immediate = immediate};
}

std::optional<DecodedRet> decode_ret(std::uint32_t word) {
  const auto opcode = word & 0x7fu;
  const auto destination = (word >> 7) & 0x1fu;
  const auto funct3 = (word >> 12) & 0x7u;
  const auto source = (word >> 15) & 0x1fu;
  const auto immediate = static_cast<std::int32_t>(word) >> 20;
  if (opcode != 0x67 || destination != 0 || funct3 != 0 || source != 1 ||
      immediate != 0) {
    return std::nullopt;
  }
  return DecodedRet{};
}

std::int32_t sign_extend(std::uint32_t value, unsigned width) {
  const auto shift = 32u - width;
  return static_cast<std::int32_t>(value << shift) >> shift;
}

std::optional<DecodedRv64I> decode_rv64i(std::uint32_t word) {
  struct RSpec {
    std::string_view mnemonic;
    std::uint32_t opcode;
    std::uint32_t funct3;
    std::uint32_t funct7;
  };
  struct ISpec {
    std::string_view mnemonic;
    std::uint32_t opcode;
    std::uint32_t funct3;
  };
  struct ShiftSpec {
    std::string_view mnemonic;
    std::uint32_t opcode;
    std::uint32_t funct3;
    std::uint32_t funct7;
    std::uint32_t max_shamt;
  };
  struct LoadStoreSpec {
    std::string_view mnemonic;
    std::uint32_t opcode;
    std::uint32_t funct3;
  };
  struct USpec {
    std::string_view mnemonic;
    std::uint32_t opcode;
  };
  struct BranchSpec {
    std::string_view mnemonic;
    std::uint32_t funct3;
  };

  constexpr RSpec r_specs[] = {
      {"add", 0x33, 0, 0x00},  {"sub", 0x33, 0, 0x20},
      {"sll", 0x33, 1, 0x00},  {"slt", 0x33, 2, 0x00},
      {"sltu", 0x33, 3, 0x00}, {"xor", 0x33, 4, 0x00},
      {"srl", 0x33, 5, 0x00},  {"sra", 0x33, 5, 0x20},
      {"or", 0x33, 6, 0x00},   {"and", 0x33, 7, 0x00},
      {"addw", 0x3b, 0, 0x00}, {"subw", 0x3b, 0, 0x20},
      {"sllw", 0x3b, 1, 0x00}, {"srlw", 0x3b, 5, 0x00},
      {"sraw", 0x3b, 5, 0x20},
  };
  constexpr ISpec i_specs[] = {
      {"addi", 0x13, 0}, {"slti", 0x13, 2},  {"sltiu", 0x13, 3},
      {"xori", 0x13, 4}, {"ori", 0x13, 6},   {"andi", 0x13, 7},
      {"addiw", 0x1b, 0},
  };
  constexpr ShiftSpec shift_specs[] = {
      {"slli", 0x13, 1, 0x00, 63}, {"srli", 0x13, 5, 0x00, 63},
      {"srai", 0x13, 5, 0x20, 63}, {"slliw", 0x1b, 1, 0x00, 31},
      {"srliw", 0x1b, 5, 0x00, 31}, {"sraiw", 0x1b, 5, 0x20, 31},
  };
  constexpr LoadStoreSpec load_specs[] = {
      {"lb", 0x03, 0},  {"lh", 0x03, 1},  {"lw", 0x03, 2},
      {"ld", 0x03, 3},  {"lbu", 0x03, 4}, {"lhu", 0x03, 5},
      {"lwu", 0x03, 6},
  };
  constexpr LoadStoreSpec store_specs[] = {
      {"sb", 0x23, 0},
      {"sh", 0x23, 1},
      {"sw", 0x23, 2},
      {"sd", 0x23, 3},
  };
  constexpr USpec u_specs[] = {
      {"lui", 0x37},
      {"auipc", 0x17},
  };
  constexpr BranchSpec branch_specs[] = {
      {"beq", 0},
      {"bne", 1},
      {"blt", 4},
      {"bge", 5},
      {"bltu", 6},
      {"bgeu", 7},
  };

  const auto opcode = word & 0x7fu;
  const auto rd = (word >> 7) & 0x1fu;
  const auto funct3 = (word >> 12) & 0x7u;
  const auto rs1 = (word >> 15) & 0x1fu;
  const auto rs2 = (word >> 20) & 0x1fu;
  const auto funct7 = (word >> 25) & 0x7fu;

  for (const auto& spec : r_specs) {
    if (opcode == spec.opcode && funct3 == spec.funct3 && funct7 == spec.funct7) {
      return DecodedRv64I{.format = Rv64DecodedFormat::RType,
                          .mnemonic = spec.mnemonic,
                          .destination = rd,
                          .lhs = rs1,
                          .rhs = rs2};
    }
  }

  for (const auto& spec : shift_specs) {
    if (opcode != spec.opcode || funct3 != spec.funct3) {
      continue;
    }
    const auto imm12 = (word >> 20) & 0xfffu;
    const auto shamt_mask = spec.max_shamt == 63 ? 0x3fu : 0x1fu;
    const auto shamt = imm12 & shamt_mask;
    const auto selector = imm12 & ~shamt_mask;
    if (selector != (spec.funct7 << 5)) {
      continue;
    }
    if (shamt > spec.max_shamt) {
      return std::nullopt;
    }
    return DecodedRv64I{.format = Rv64DecodedFormat::IType,
                        .mnemonic = spec.mnemonic,
                        .destination = rd,
                        .lhs = rs1,
                        .immediate = static_cast<std::int32_t>(shamt)};
  }

  for (const auto& spec : i_specs) {
    if (opcode == spec.opcode && funct3 == spec.funct3) {
      return DecodedRv64I{.format = Rv64DecodedFormat::IType,
                          .mnemonic = spec.mnemonic,
                          .destination = rd,
                          .lhs = rs1,
                          .immediate = static_cast<std::int32_t>(word) >> 20};
    }
  }

  if (opcode == 0x67 && funct3 == 0) {
    return DecodedRv64I{.format = Rv64DecodedFormat::Jalr,
                        .mnemonic = "jalr",
                        .destination = rd,
                        .lhs = rs1,
                        .immediate = static_cast<std::int32_t>(word) >> 20};
  }

  if (opcode == 0x6f) {
    const auto imm21 = ((word >> 31) << 20) |
                       (((word >> 21) & 0x3ffu) << 1) |
                       (((word >> 20) & 0x1u) << 11) |
                       (((word >> 12) & 0xffu) << 12);
    return DecodedRv64I{.format = Rv64DecodedFormat::Jal,
                        .mnemonic = "jal",
                        .destination = rd,
                        .immediate = sign_extend(imm21, 21)};
  }

  if (opcode == 0x63) {
    for (const auto& spec : branch_specs) {
      if (funct3 != spec.funct3) {
        continue;
      }
      const auto imm13 = ((word >> 31) << 12) |
                         (((word >> 25) & 0x3fu) << 5) |
                         (((word >> 8) & 0xfu) << 1) |
                         (((word >> 7) & 0x1u) << 11);
      return DecodedRv64I{.format = Rv64DecodedFormat::Branch,
                          .mnemonic = spec.mnemonic,
                          .lhs = rs1,
                          .rhs = rs2,
                          .immediate = sign_extend(imm13, 13)};
    }
  }

  for (const auto& spec : load_specs) {
    if (opcode == spec.opcode && funct3 == spec.funct3) {
      return DecodedRv64I{.format = Rv64DecodedFormat::Load,
                          .mnemonic = spec.mnemonic,
                          .destination = rd,
                          .lhs = rs1,
                          .immediate = static_cast<std::int32_t>(word) >> 20};
    }
  }

  for (const auto& spec : store_specs) {
    if (opcode == spec.opcode && funct3 == spec.funct3) {
      const auto imm12 = ((word >> 25) << 5) | ((word >> 7) & 0x1fu);
      return DecodedRv64I{.format = Rv64DecodedFormat::Store,
                          .mnemonic = spec.mnemonic,
                          .lhs = rs1,
                          .rhs = rs2,
                          .immediate = sign_extend(imm12, 12)};
    }
  }

  for (const auto& spec : u_specs) {
    if (opcode == spec.opcode) {
      return DecodedRv64I{.format = Rv64DecodedFormat::UType,
                          .mnemonic = spec.mnemonic,
                          .destination = rd,
                          .immediate = sign_extend(word >> 12, 20)};
    }
  }

  return std::nullopt;
}

std::optional<std::string> label_for_offset(const ExtractedObject& object,
                                            std::uint64_t offset) {
  if (offset == 0) {
    return object.function_name;
  }
  for (const auto& label : object.local_labels) {
    if (label.offset == offset) {
      return label.name;
    }
  }
  return std::nullopt;
}

std::optional<std::uint64_t> relative_target_offset(std::uint64_t current_offset,
                                                    std::int32_t immediate,
                                                    std::uint64_t text_size) {
  const auto signed_target =
      static_cast<std::int64_t>(current_offset) + static_cast<std::int64_t>(immediate);
  if (signed_target < 0 ||
      static_cast<std::uint64_t>(signed_target) > text_size) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(signed_target);
}

std::optional<std::vector<DecodedLine>> decode_text_bytes(
    const ExtractedObject& object,
    std::string* error) {
  const auto& text_bytes = object.text_bytes;
  std::vector<DecodedLine> decoded;
  for (std::size_t offset = 0; offset < text_bytes.size();) {
    if (offset + 8 <= text_bytes.size()) {
      if (auto insn = decode_insn_d(read_u64_at(text_bytes, offset)); insn.has_value()) {
        decoded.push_back(DecodedLine{.offset = static_cast<std::uint64_t>(offset),
                                      .instruction = *insn});
        offset += 8;
        continue;
      }
    }
    if (offset + 4 <= text_bytes.size()) {
      const auto word = read_u32_at(text_bytes, offset);
      if (auto li = decode_li(word); li.has_value()) {
        decoded.push_back(DecodedLine{.offset = static_cast<std::uint64_t>(offset),
                                      .instruction = *li});
        offset += 4;
        continue;
      }
      if (auto ret = decode_ret(word); ret.has_value()) {
        decoded.push_back(DecodedLine{.offset = static_cast<std::uint64_t>(offset),
                                      .instruction = *ret});
        offset += 4;
        continue;
      }
      if (auto rv64i = decode_rv64i(word); rv64i.has_value()) {
        if (rv64i->format == Rv64DecodedFormat::Branch ||
            rv64i->format == Rv64DecodedFormat::Jal) {
          const auto target =
              relative_target_offset(static_cast<std::uint64_t>(offset),
                                     rv64i->immediate,
                                     text_bytes.size());
          if (!target.has_value()) {
            if (error != nullptr) {
              std::ostringstream message;
              message << "unsupported RV64 control-flow target at .text offset 0x"
                      << std::hex << offset;
              *error = message.str();
            }
            return std::nullopt;
          }
          auto label = label_for_offset(object, *target);
          if (!label.has_value()) {
            if (error != nullptr) {
              std::ostringstream message;
              message << "unsupported RV64 control-flow target without label at .text offset 0x"
                      << std::hex << offset;
              *error = message.str();
            }
            return std::nullopt;
          }
          rv64i->target_label = std::move(*label);
        }
        decoded.push_back(DecodedLine{.offset = static_cast<std::uint64_t>(offset),
                                      .instruction = *rv64i});
        offset += 4;
        continue;
      }
    }
    if (error != nullptr) {
      std::ostringstream message;
      message << "unsupported RV64 instruction bytes at .text offset 0x"
              << std::hex << offset;
      *error = message.str();
    }
    return std::nullopt;
  }
  for (const auto& label : object.local_labels) {
    if (label.offset == text_bytes.size()) {
      continue;
    }
    const auto label_on_instruction =
        std::any_of(decoded.begin(), decoded.end(), [&](const DecodedLine& line) {
          return line.offset == label.offset;
        });
    if (!label_on_instruction) {
      if (error != nullptr) {
        std::ostringstream message;
        message << "unsupported RV64 label '" << label.name
                << "' at non-instruction .text offset 0x" << std::hex
                << label.offset;
        *error = message.str();
      }
      return std::nullopt;
    }
  }
  return decoded;
}

std::string gpr_name(std::uint32_t reg) {
  constexpr std::string_view names[] = {
      "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
      "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
      "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
      "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",
  };
  if (reg < std::size(names)) {
    return std::string(names[reg]);
  }
  return "x" + std::to_string(reg);
}

std::string instruction_assembly(const DecodedInstruction& instruction) {
  std::ostringstream out;
  if (const auto* insn_d = std::get_if<DecodedInsnD>(&instruction)) {
    out << ".insn.d " << insn_d->major
        << ", " << insn_d->operation
        << ", v" << insn_d->destination
        << ", v" << insn_d->lhs
        << ", v" << insn_d->rhs
        << ", v" << insn_d->accumulator
        << ", " << insn_d->dtype;
    return out.str();
  }
  if (const auto* li = std::get_if<DecodedLi>(&instruction)) {
    out << "li " << gpr_name(li->destination) << ", " << li->immediate;
    return out.str();
  }
  if (const auto* rv64i = std::get_if<DecodedRv64I>(&instruction)) {
    out << rv64i->mnemonic << ' ';
    switch (rv64i->format) {
      case Rv64DecodedFormat::RType:
        out << gpr_name(rv64i->destination) << ", " << gpr_name(rv64i->lhs)
            << ", " << gpr_name(rv64i->rhs);
        break;
      case Rv64DecodedFormat::IType:
        out << gpr_name(rv64i->destination) << ", " << gpr_name(rv64i->lhs)
            << ", " << rv64i->immediate;
        break;
      case Rv64DecodedFormat::Load:
      case Rv64DecodedFormat::Jalr:
        out << gpr_name(rv64i->destination) << ", " << rv64i->immediate
            << '(' << gpr_name(rv64i->lhs) << ')';
        break;
      case Rv64DecodedFormat::Store:
        out << gpr_name(rv64i->rhs) << ", " << rv64i->immediate
            << '(' << gpr_name(rv64i->lhs) << ')';
        break;
      case Rv64DecodedFormat::UType:
        out << gpr_name(rv64i->destination) << ", " << rv64i->immediate;
        break;
      case Rv64DecodedFormat::Branch:
        out << gpr_name(rv64i->lhs) << ", " << gpr_name(rv64i->rhs)
            << ", " << rv64i->target_label;
        break;
      case Rv64DecodedFormat::Jal:
        out << gpr_name(rv64i->destination) << ", " << rv64i->target_label;
        break;
    }
    return out.str();
  }
  return "ret";
}

bool write_canonical_assembly(const ExtractedObject& object,
                              const std::vector<DecodedLine>& decoded,
                              const std::string& output_path) {
  std::ofstream output(output_path, std::ios::trunc);
  if (!output) {
    std::cerr << "c4c-objdump: error: failed to open output file '" << output_path << "'\n";
    return false;
  }
  output << ".text\n"
         << ".globl " << object.function_name << '\n'
         << object.function_name << ":\n";
  auto next_label = object.local_labels.begin();
  for (const auto& line : decoded) {
    while (next_label != object.local_labels.end() &&
           next_label->offset == line.offset) {
      output << next_label->name << ":\n";
      ++next_label;
    }
    output << "  " << instruction_assembly(line.instruction) << '\n';
  }
  while (next_label != object.local_labels.end() &&
         next_label->offset == object.text_bytes.size()) {
    output << next_label->name << ":\n";
    ++next_label;
  }
  if (!output) {
    std::cerr << "c4c-objdump: error: failed to write output file '" << output_path << "'\n";
    return false;
  }
  return true;
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
      std::cout << "c4c-objdump 0.1\n";
      return 0;
    }
  }

  const auto options = parse_cli(argc, argv);
  if (!options.has_value()) {
    return 1;
  }

  std::string read_error;
  const auto bytes = read_file_bytes(options->input_path, &read_error);
  if (!read_error.empty()) {
    std::cerr << "c4c-objdump: error: " << read_error << '\n';
    return 1;
  }

  std::string parse_error;
  const auto object = extract_supported_rv64_object(bytes, &parse_error);
  if (!object.has_value()) {
    std::cerr << "c4c-objdump: error: " << options->input_path << ": "
              << parse_error << '\n';
    return 1;
  }

  std::string decode_error;
  const auto decoded = decode_text_bytes(*object, &decode_error);
  if (!decoded.has_value()) {
    std::cerr << "c4c-objdump: error: " << options->input_path << ": "
              << decode_error << '\n';
    return 1;
  }
  if (!write_canonical_assembly(*object, *decoded, options->output_path)) {
    return 1;
  }

  std::cout << "c4c-objdump: extracted " << object->text_bytes.size()
            << " .text byte(s) from " << options->input_path
            << " symbol " << object->function_name << " into "
            << options->output_path << ": " << hex_bytes(object->text_bytes)
            << "; decoded " << decoded->size() << " instruction(s)\n";
  return 0;
}
