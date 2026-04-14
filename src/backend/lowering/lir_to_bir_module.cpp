#include "lir_to_bir.hpp"

#include "call_decode.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace c4c::backend {

namespace {

using ValueMap = std::unordered_map<std::string, bir::Value>;
using LocalSlotTypes = std::unordered_map<std::string, bir::TypeKind>;
using LocalPointerSlots = std::unordered_map<std::string, std::string>;

struct GlobalAddress {
  std::string global_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
};

struct GlobalInfo {
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t element_count = 0;
  std::size_t storage_size_bytes = 0;
  bool supports_direct_value = false;
  bool supports_linear_addressing = false;
  std::string type_text;
  std::optional<GlobalAddress> known_global_address;
  std::string initializer_symbol_name;
  bir::TypeKind initializer_offset_type = bir::TypeKind::Void;
  std::size_t initializer_byte_offset = 0;
  std::unordered_map<std::size_t, GlobalAddress> pointer_initializer_offsets;
};

using GlobalTypes = std::unordered_map<std::string, GlobalInfo>;
using TypeDeclMap = std::unordered_map<std::string, std::string>;

struct GlobalPointerSlotKey {
  std::string global_name;
  std::size_t byte_offset = 0;

  bool operator==(const GlobalPointerSlotKey& other) const {
    return global_name == other.global_name && byte_offset == other.byte_offset;
  }
};

struct GlobalPointerSlotKeyHash {
  std::size_t operator()(const GlobalPointerSlotKey& key) const {
    return std::hash<std::string>{}(key.global_name) ^
           (std::hash<std::size_t>{}(key.byte_offset) << 1);
  }
};

using GlobalPointerMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalObjectPointerMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalAddressIntMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalObjectAddressIntMap = std::unordered_map<std::string, GlobalAddress>;
using LocalAddressSlots = std::unordered_map<std::string, GlobalAddress>;
using GlobalAddressSlots = std::unordered_map<std::string, std::optional<GlobalAddress>>;
using FunctionSymbolSet = std::unordered_set<std::string>;
using AddressedGlobalPointerSlots =
    std::unordered_map<GlobalPointerSlotKey,
                       std::optional<GlobalAddress>,
                       GlobalPointerSlotKeyHash>;

struct LocalArraySlots {
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::vector<std::string> element_slots;
};

using LocalArraySlotMap = std::unordered_map<std::string, LocalArraySlots>;

struct DynamicLocalPointerArrayAccess {
  std::vector<std::string> element_slots;
  bir::Value index;
};

using DynamicLocalPointerArrayMap =
    std::unordered_map<std::string, DynamicLocalPointerArrayAccess>;

struct LocalPointerArrayBase {
  std::vector<std::string> element_slots;
  std::size_t base_index = 0;
};

using LocalPointerArrayBaseMap = std::unordered_map<std::string, LocalPointerArrayBase>;

struct DynamicGlobalPointerArrayAccess {
  std::string global_name;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

using DynamicGlobalPointerArrayMap =
    std::unordered_map<std::string, DynamicGlobalPointerArrayAccess>;

struct DynamicGlobalAggregateArrayAccess {
  std::string global_name;
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

using DynamicGlobalAggregateArrayMap =
    std::unordered_map<std::string, DynamicGlobalAggregateArrayAccess>;

struct LocalAggregateSlots {
  std::string type_text;
  std::unordered_map<std::size_t, std::string> leaf_slots;
};

using LocalAggregateSlotMap = std::unordered_map<std::string, LocalAggregateSlots>;
using LocalAggregateFieldSet = std::unordered_set<std::string>;
using LocalPointerValueAliasMap = std::unordered_map<std::string, bir::Value>;

struct CompareExpr {
  bir::BinaryOpcode opcode = bir::BinaryOpcode::Eq;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::Value lhs;
  bir::Value rhs;
};

using CompareMap = std::unordered_map<std::string, CompareExpr>;
using BlockLookup = std::unordered_map<std::string, const c4c::codegen::lir::LirBlock*>;

struct BranchChain {
  std::vector<std::string> labels;
  std::string leaf_label;
  std::string join_label;
};

struct ParsedTypedOperand {
  std::string type_text;
  c4c::codegen::lir::LirOperand operand;
};

struct AggregateField {
  std::size_t byte_offset = 0;
  std::string type_text;
};

struct AggregateTypeLayout {
  enum class Kind : unsigned char {
    Invalid,
    Scalar,
    Array,
    Struct,
  };

  Kind kind = Kind::Invalid;
  bir::TypeKind scalar_type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::size_t array_count = 0;
  std::string element_type_text;
  std::vector<AggregateField> fields;
};

std::optional<std::int64_t> parse_i64(std::string_view text);
std::optional<bir::TypeKind> lower_integer_type(std::string_view text);
std::optional<std::pair<std::size_t, std::string_view>> parse_integer_array_layer(
    std::string_view text);
std::size_t type_size_bytes(bir::TypeKind type);
std::size_t type_size_bytes_from_text(std::string_view text);
std::optional<ParsedTypedOperand> parse_typed_operand(std::string_view text);
std::optional<std::int64_t> resolve_index_operand(const c4c::codegen::lir::LirOperand& operand,
                                                  const ValueMap& value_aliases);
std::optional<bir::Value> lower_global_initializer(std::string_view text,
                                                   bir::TypeKind type);
AggregateTypeLayout compute_aggregate_type_layout(std::string_view text,
                                                  const TypeDeclMap& type_decls);
std::vector<std::string_view> split_top_level_initializer_items(std::string_view text);

bool is_local_array_element_slot(std::string_view slot_name,
                                 const LocalArraySlotMap& local_array_slots) {
  for (const auto& [array_name, array_slots] : local_array_slots) {
    (void)array_name;
    if (std::find(array_slots.element_slots.begin(),
                  array_slots.element_slots.end(),
                  slot_name) != array_slots.element_slots.end()) {
      return true;
    }
  }
  return false;
}

bool is_known_function_symbol(std::string_view symbol_name,
                              const FunctionSymbolSet& function_symbols) {
  return function_symbols.find(std::string(symbol_name)) != function_symbols.end();
}

std::optional<std::string> parse_global_symbol_initializer(std::string_view text) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.size() < 2 || trimmed.front() != '@') {
    return std::nullopt;
  }
  return std::string(trimmed.substr(1));
}

std::optional<GlobalAddress> parse_global_gep_initializer(std::string_view text,
                                                          const TypeDeclMap& type_decls) {
  constexpr std::string_view kPrefix = "getelementptr inbounds (";

  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.size() <= kPrefix.size() || trimmed.substr(0, kPrefix.size()) != kPrefix ||
      trimmed.back() != ')') {
    return std::nullopt;
  }

  const auto body = trimmed.substr(kPrefix.size(), trimmed.size() - kPrefix.size() - 1);
  const auto ptr_pos = body.find(", ptr @");
  if (ptr_pos == std::string_view::npos) {
    return std::nullopt;
  }

  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(body.substr(0, ptr_pos));
  auto remainder = body.substr(ptr_pos + 7);
  const auto global_end = remainder.find(',');

  std::string global_name;
  if (global_end == std::string_view::npos) {
    global_name = std::string(c4c::codegen::lir::trim_lir_arg_text(remainder));
    remainder = std::string_view();
  } else {
    global_name =
        std::string(c4c::codegen::lir::trim_lir_arg_text(remainder.substr(0, global_end)));
    remainder = remainder.substr(global_end + 1);
  }
  if (global_name.empty()) {
    return std::nullopt;
  }

  std::size_t byte_offset = 0;
  bool saw_index = false;
  while (!remainder.empty()) {
    remainder = c4c::codegen::lir::trim_lir_arg_text(remainder);
    if (remainder.empty()) {
      break;
    }

    const auto comma = remainder.find(',');
    const auto index_text =
        c4c::codegen::lir::trim_lir_arg_text(comma == std::string_view::npos
                                                 ? remainder
                                                 : remainder.substr(0, comma));
    const auto index = parse_typed_operand(index_text);
    if (!index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(index->operand, ValueMap{});
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_index) {
      saw_index = true;
      if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
        byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
        if (comma != std::string_view::npos) {
          return std::nullopt;
        }
        return GlobalAddress{
            .global_name = std::move(global_name),
            .value_type = layout.scalar_type,
            .byte_offset = byte_offset,
        };
      }
      if (*index_value != 0) {
        return std::nullopt;
      }
      if (comma == std::string_view::npos) {
        break;
      }
      remainder = remainder.substr(comma + 1);
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      case AggregateTypeLayout::Kind::Scalar:
        if (comma != std::string_view::npos) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
        return GlobalAddress{
            .global_name = std::move(global_name),
            .value_type = layout.scalar_type,
            .byte_offset = byte_offset,
        };
      default:
        return std::nullopt;
    }
    if (comma == std::string_view::npos) {
      break;
    }
    remainder = remainder.substr(comma + 1);
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  if (leaf_layout.kind == AggregateTypeLayout::Kind::Scalar) {
    return GlobalAddress{
        .global_name = std::move(global_name),
        .value_type = leaf_layout.scalar_type,
        .byte_offset = byte_offset,
    };
  }

  return std::nullopt;
}

std::optional<GlobalAddress> parse_global_address_initializer(std::string_view text,
                                                              const TypeDeclMap& type_decls) {
  if (const auto symbol_name = parse_global_symbol_initializer(text); symbol_name.has_value()) {
    return GlobalAddress{
        .global_name = *symbol_name,
        .value_type = bir::TypeKind::Void,
        .byte_offset = 0,
    };
  }
  return parse_global_gep_initializer(text, type_decls);
}

std::optional<GlobalAddress> resolve_known_global_address(std::string_view global_name,
                                                          GlobalTypes& global_types,
                                                          const FunctionSymbolSet& function_symbols,
                                                          std::unordered_set<std::string>* active) {
  const auto it = global_types.find(std::string(global_name));
  if (it == global_types.end()) {
    return std::nullopt;
  }

  auto& info = it->second;
  if (info.known_global_address.has_value()) {
    return info.known_global_address;
  }
  if (info.initializer_symbol_name.empty()) {
    return std::nullopt;
  }
  if (is_known_function_symbol(info.initializer_symbol_name, function_symbols)) {
    if (info.initializer_offset_type != bir::TypeKind::Void || info.initializer_byte_offset != 0) {
      return std::nullopt;
    }
    info.known_global_address = GlobalAddress{
        .global_name = info.initializer_symbol_name,
        .value_type = bir::TypeKind::Ptr,
        .byte_offset = 0,
    };
    return info.known_global_address;
  }

  const std::string active_name(global_name);
  if (!active->insert(active_name).second) {
    return std::nullopt;
  }

  const auto erase_active = [&]() { active->erase(active_name); };
  const auto pointee_it = global_types.find(info.initializer_symbol_name);
  if (pointee_it == global_types.end()) {
    erase_active();
    return std::nullopt;
  }

  GlobalAddress resolved;
  if (pointee_it->second.supports_linear_addressing) {
    resolved = GlobalAddress{
        .global_name = info.initializer_symbol_name,
        .value_type = info.initializer_offset_type != bir::TypeKind::Void
                          ? info.initializer_offset_type
                          : pointee_it->second.value_type,
        .byte_offset = info.initializer_byte_offset,
    };
  } else {
    const auto pointee_address =
        resolve_known_global_address(
            info.initializer_symbol_name, global_types, function_symbols, active);
    if (!pointee_address.has_value()) {
      erase_active();
      return std::nullopt;
    }
    resolved = *pointee_address;
    auto nested_offset = info.initializer_byte_offset;
    if (info.initializer_offset_type == bir::TypeKind::I8 &&
        pointee_it->second.value_type == bir::TypeKind::Ptr &&
        resolved.value_type != bir::TypeKind::I8) {
      const auto pointee_stride = type_size_bytes(resolved.value_type);
      if (pointee_stride == 0) {
        erase_active();
        return std::nullopt;
      }
      nested_offset *= pointee_stride;
    }
    resolved.byte_offset += nested_offset;
  }

  erase_active();

  const auto base_it = global_types.find(resolved.global_name);
  if (base_it == global_types.end() || !base_it->second.supports_linear_addressing) {
    return std::nullopt;
  }
  if (resolved.byte_offset >= base_it->second.storage_size_bytes) {
    return std::nullopt;
  }

  info.known_global_address = resolved;
  return info.known_global_address;
}

bool resolve_pointer_initializer_offsets(GlobalTypes& global_types,
                                         const FunctionSymbolSet& function_symbols) {
  std::unordered_set<std::string> resolving_global_addresses;
  for (auto& [global_name, info] : global_types) {
    (void)global_name;
    for (auto& [byte_offset, address] : info.pointer_initializer_offsets) {
      if (address.value_type != bir::TypeKind::Void) {
        continue;
      }
      if (byte_offset >= info.storage_size_bytes) {
        return false;
      }

      const auto target_it = global_types.find(address.global_name);
      if (target_it == global_types.end()) {
        if (!is_known_function_symbol(address.global_name, function_symbols) ||
            address.byte_offset != 0) {
          return false;
        }
        address.value_type = bir::TypeKind::Ptr;
        continue;
      }

      if (target_it->second.supports_linear_addressing) {
        if (address.value_type == bir::TypeKind::Void) {
          address.value_type = target_it->second.value_type;
        }
        if (address.byte_offset >= target_it->second.storage_size_bytes) {
          return false;
        }
        continue;
      }

      if (target_it->second.supports_direct_value &&
          target_it->second.value_type == bir::TypeKind::Ptr &&
          address.byte_offset == 0) {
        address.value_type = bir::TypeKind::Ptr;
        continue;
      }

      const auto resolved_address =
          resolve_known_global_address(
              address.global_name, global_types, function_symbols, &resolving_global_addresses);
      if (!resolved_address.has_value()) {
        return false;
      }
      address = *resolved_address;
    }
  }
  return true;
}

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<bir::TypeKind> lower_integer_type(std::string_view text) {
  if (text == "ptr") {
    return bir::TypeKind::Ptr;
  }
  if (text == "i1") {
    return bir::TypeKind::I1;
  }
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  if (text == "void") {
    return bir::TypeKind::Void;
  }
  return std::nullopt;
}

std::optional<bir::CastOpcode> lower_cast_opcode(c4c::codegen::lir::LirCastKind kind) {
  switch (kind) {
    case c4c::codegen::lir::LirCastKind::SExt:
      return bir::CastOpcode::SExt;
    case c4c::codegen::lir::LirCastKind::ZExt:
      return bir::CastOpcode::ZExt;
    case c4c::codegen::lir::LirCastKind::Trunc:
      return bir::CastOpcode::Trunc;
    default:
      return std::nullopt;
  }
}

std::size_t type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return 1;
    case bir::TypeKind::I32:
      return 4;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 8;
    default:
      return 0;
  }
}

std::optional<std::pair<std::size_t, bir::TypeKind>> parse_local_array_type(std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  const auto element_type = lower_integer_type(text.substr(x_pos + 3, text.size() - x_pos - 4));
  if (!element_type.has_value()) {
    return std::nullopt;
  }

  return std::pair<std::size_t, bir::TypeKind>{static_cast<std::size_t>(*count), *element_type};
}

struct IntegerArrayType {
  std::vector<std::size_t> extents;
  bir::TypeKind element_type = bir::TypeKind::Void;
};

std::optional<IntegerArrayType> parse_integer_array_type(std::string_view text) {
  IntegerArrayType lowered;
  std::string_view remainder = text;
  while (true) {
    if (const auto scalar_type = lower_integer_type(remainder); scalar_type.has_value()) {
      if (lowered.extents.empty()) {
        return std::nullopt;
      }
      lowered.element_type = *scalar_type;
      return lowered;
    }

    if (remainder.size() < 6 || remainder.front() != '[' || remainder.back() != ']') {
      return std::nullopt;
    }

    const auto x_pos = remainder.find(" x ");
    if (x_pos == std::string_view::npos || x_pos <= 1) {
      return std::nullopt;
    }

    const auto count = parse_i64(remainder.substr(1, x_pos - 1));
    if (!count.has_value() || *count <= 0) {
      return std::nullopt;
    }

    lowered.extents.push_back(static_cast<std::size_t>(*count));
    remainder = remainder.substr(x_pos + 3, remainder.size() - x_pos - 4);
  }
}

std::optional<std::string_view> peel_integer_array_layer(std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  return text.substr(x_pos + 3, text.size() - x_pos - 4);
}

std::optional<std::pair<std::size_t, std::string_view>> parse_integer_array_layer(
    std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  return std::pair<std::size_t, std::string_view>{
      static_cast<std::size_t>(*count),
      text.substr(x_pos + 3, text.size() - x_pos - 4),
  };
}

std::optional<int> parse_hex_digit(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return std::nullopt;
}

std::optional<std::vector<bir::Value>> lower_llvm_byte_string_initializer(
    std::string_view init_text,
    std::string_view type_text) {
  const auto array_type = parse_integer_array_type(type_text);
  if (!array_type.has_value() || array_type->element_type != bir::TypeKind::I8 ||
      array_type->extents.size() != 1) {
    return std::nullopt;
  }

  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.size() < 3 || trimmed_init[0] != 'c' || trimmed_init[1] != '"' ||
      trimmed_init.back() != '"') {
    return std::nullopt;
  }

  std::vector<bir::Value> lowered;
  lowered.reserve(array_type->extents.front());
  for (std::size_t index = 2; index + 1 < trimmed_init.size(); ++index) {
    unsigned char byte = static_cast<unsigned char>(trimmed_init[index]);
    if (trimmed_init[index] == '\\') {
      if (index + 2 >= trimmed_init.size() - 1) {
        return std::nullopt;
      }
      const auto hi = parse_hex_digit(trimmed_init[index + 1]);
      const auto lo = parse_hex_digit(trimmed_init[index + 2]);
      if (!hi.has_value() || !lo.has_value()) {
        return std::nullopt;
      }
      byte = static_cast<unsigned char>((*hi << 4) | *lo);
      index += 2;
    }
    lowered.push_back(bir::Value::immediate_i8(static_cast<std::int8_t>(byte)));
  }

  if (lowered.size() > array_type->extents.front()) {
    return std::nullopt;
  }
  while (lowered.size() < array_type->extents.front()) {
    lowered.push_back(bir::Value::immediate_i8(0));
  }
  return lowered;
}

std::size_t type_size_bytes_from_text(std::string_view text) {
  if (const auto scalar_type = lower_integer_type(text); scalar_type.has_value()) {
    return type_size_bytes(*scalar_type);
  }

  const auto array_type = parse_integer_array_type(text);
  if (!array_type.has_value()) {
    return 0;
  }

  const auto element_size_bytes = type_size_bytes(array_type->element_type);
  if (element_size_bytes == 0) {
    return 0;
  }

  std::size_t total_elements = 1;
  for (const auto extent : array_type->extents) {
    total_elements *= extent;
  }
  return total_elements * element_size_bytes;
}

std::vector<std::string_view> split_top_level_initializer_items(std::string_view text) {
  std::vector<std::string_view> items;
  std::size_t item_start = 0;
  int depth = 0;
  for (std::size_t index = 0; index < text.size(); ++index) {
    const char ch = text[index];
    if (ch == '[' || ch == '{') {
      ++depth;
    } else if (ch == ']' || ch == '}') {
      --depth;
    } else if (ch == ',' && depth == 0) {
      items.push_back(text.substr(item_start, index - item_start));
      item_start = index + 1;
    }
  }
  items.push_back(text.substr(item_start));
  return items;
}

std::size_t align_up(std::size_t value, std::size_t align_bytes) {
  if (align_bytes <= 1) {
    return value;
  }
  const auto remainder = value % align_bytes;
  return remainder == 0 ? value : value + (align_bytes - remainder);
}

TypeDeclMap build_type_decl_map(const std::vector<std::string>& type_decls) {
  TypeDeclMap lowered;
  lowered.reserve(type_decls.size());
  for (const auto& decl : type_decls) {
    const auto eq = decl.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const auto type_kw = decl.find("type", eq);
    if (type_kw == std::string::npos) {
      continue;
    }
    const auto name = std::string(c4c::codegen::lir::trim_lir_arg_text(decl.substr(0, eq)));
    const auto body =
        std::string(c4c::codegen::lir::trim_lir_arg_text(decl.substr(type_kw + 4)));
    if (!name.empty() && !body.empty()) {
      lowered.emplace(name, body);
    }
  }
  return lowered;
}

std::optional<std::string_view> resolve_type_decl_body(std::string_view text,
                                                       const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty() || trimmed.front() != '%') {
    return std::nullopt;
  }
  const auto it = type_decls.find(std::string(trimmed));
  if (it == type_decls.end()) {
    return std::nullopt;
  }
  return std::string_view(it->second);
}

AggregateTypeLayout compute_aggregate_type_layout(std::string_view text,
                                                  const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty()) {
    return {};
  }

  if (const auto scalar_type = lower_integer_type(trimmed); scalar_type.has_value()) {
    return AggregateTypeLayout{
        .kind = AggregateTypeLayout::Kind::Scalar,
        .scalar_type = *scalar_type,
        .size_bytes = type_size_bytes(*scalar_type),
        .align_bytes = type_size_bytes(*scalar_type),
    };
  }

  if (const auto resolved = resolve_type_decl_body(trimmed, type_decls); resolved.has_value()) {
    return compute_aggregate_type_layout(*resolved, type_decls);
  }

  if (const auto layer = parse_integer_array_layer(trimmed); layer.has_value()) {
    const auto element_layout = compute_aggregate_type_layout(layer->second, type_decls);
    if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        element_layout.size_bytes == 0 || element_layout.align_bytes == 0) {
      return {};
    }
    return AggregateTypeLayout{
        .kind = AggregateTypeLayout::Kind::Array,
        .size_bytes = layer->first * element_layout.size_bytes,
        .align_bytes = element_layout.align_bytes,
        .array_count = layer->first,
        .element_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(layer->second)),
    };
  }

  if (trimmed.size() < 2 || trimmed.front() != '{' || trimmed.back() != '}') {
    return {};
  }

  const auto body = trimmed.substr(1, trimmed.size() - 2);
  const auto field_items = split_top_level_initializer_items(body);
  AggregateTypeLayout layout;
  layout.kind = AggregateTypeLayout::Kind::Struct;
  std::size_t current_offset = 0;
  std::size_t struct_align = 1;
  for (const auto item : field_items) {
    const auto field_type = c4c::codegen::lir::trim_lir_arg_text(item);
    if (field_type.empty()) {
      return {};
    }
    const auto field_layout = compute_aggregate_type_layout(field_type, type_decls);
    if (field_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        field_layout.size_bytes == 0 || field_layout.align_bytes == 0) {
      return {};
    }
    current_offset = align_up(current_offset, field_layout.align_bytes);
    layout.fields.push_back(AggregateField{
        .byte_offset = current_offset,
        .type_text = std::string(field_type),
    });
    current_offset += field_layout.size_bytes;
    struct_align = std::max(struct_align, field_layout.align_bytes);
  }

  if (layout.fields.empty()) {
    return {};
  }
  layout.align_bytes = struct_align;
  layout.size_bytes = align_up(current_offset, struct_align);
  return layout;
}

bool is_zero_integer_array_initializer(std::string_view init_text, std::string_view type_text) {
  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init == "zeroinitializer") {
    return true;
  }

  if (trimmed_init.size() < 2 || trimmed_init.front() != '[' || trimmed_init.back() != ']') {
    return false;
  }

  const auto element_type = peel_integer_array_layer(type_text);
  if (!element_type.has_value()) {
    return false;
  }

  const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
  for (const auto item : split_top_level_initializer_items(body)) {
    const auto trimmed_item = c4c::codegen::lir::trim_lir_arg_text(item);
    if (trimmed_item.empty()) {
      return false;
    }

    if (parse_integer_array_type(*element_type).has_value()) {
      if (!is_zero_integer_array_initializer(trimmed_item, *element_type)) {
        return false;
      }
      continue;
    }

    const auto space = trimmed_item.find(' ');
    if (space == std::string_view::npos || space == 0 || space + 1 >= trimmed_item.size() ||
        trimmed_item.substr(0, space) != *element_type) {
      return false;
    }
    const auto zero_value = parse_i64(trimmed_item.substr(space + 1));
    if (!zero_value.has_value() || *zero_value != 0) {
      return false;
    }
  }
  return true;
}

std::optional<bir::Value> lower_zero_initializer_value(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(false);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(0);
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(0);
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(0);
    case bir::TypeKind::Ptr:
      return bir::Value{
          .kind = bir::Value::Kind::Immediate,
          .type = bir::TypeKind::Ptr,
          .immediate = 0,
          .immediate_bits = 0,
      };
    default:
      return std::nullopt;
  }
}

bool append_zero_integer_array_initializer(std::string_view type_text,
                                           std::vector<bir::Value>* out) {
  const auto scalar_type = lower_integer_type(type_text);
  if (scalar_type.has_value()) {
    const auto zero_value = lower_zero_initializer_value(*scalar_type);
    if (!zero_value.has_value()) {
      return false;
    }
    out->push_back(*zero_value);
    return true;
  }

  const auto layer = parse_integer_array_layer(type_text);
  if (!layer.has_value()) {
    return false;
  }

  for (std::size_t index = 0; index < layer->first; ++index) {
    if (!append_zero_integer_array_initializer(layer->second, out)) {
      return false;
    }
  }
  return true;
}

bool lower_integer_array_initializer_recursive(std::string_view init_text,
                                               std::string_view type_text,
                                               std::vector<bir::Value>* out) {
  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.empty()) {
    return false;
  }

  if (trimmed_init == "zeroinitializer") {
    return append_zero_integer_array_initializer(type_text, out);
  }

  if (const auto scalar_type = lower_integer_type(type_text); scalar_type.has_value()) {
    std::string_view scalar_init = trimmed_init;
    const auto space = trimmed_init.find(' ');
    if (space != std::string_view::npos && trimmed_init.substr(0, space) == type_text) {
      scalar_init = trimmed_init.substr(space + 1);
    }
    const auto value = lower_global_initializer(scalar_init, *scalar_type);
    if (!value.has_value()) {
      return false;
    }
    out->push_back(*value);
    return true;
  }

  const auto layer = parse_integer_array_layer(type_text);
  if (!layer.has_value() || trimmed_init.front() != '[' || trimmed_init.back() != ']') {
    return false;
  }

  const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
  const auto items = split_top_level_initializer_items(body);
  if (items.size() > layer->first) {
    return false;
  }

  for (const auto item : items) {
    const auto trimmed_item = c4c::codegen::lir::trim_lir_arg_text(item);
    if (trimmed_item.empty()) {
      return false;
    }
    if (trimmed_item.size() <= layer->second.size() ||
        trimmed_item.substr(0, layer->second.size()) != layer->second ||
        trimmed_item[layer->second.size()] != ' ') {
      return false;
    }
    if (!lower_integer_array_initializer_recursive(
            trimmed_item.substr(layer->second.size() + 1), layer->second, out)) {
      return false;
    }
  }

  for (std::size_t index = items.size(); index < layer->first; ++index) {
    if (!append_zero_integer_array_initializer(layer->second, out)) {
      return false;
    }
  }
  return true;
}

std::optional<std::vector<bir::Value>> lower_integer_array_initializer(std::string_view init_text,
                                                                       std::string_view type_text) {
  if (const auto lowered_bytes = lower_llvm_byte_string_initializer(init_text, type_text);
      lowered_bytes.has_value()) {
    return lowered_bytes;
  }

  std::vector<bir::Value> lowered;
  if (!lower_integer_array_initializer_recursive(init_text, type_text, &lowered)) {
    return std::nullopt;
  }
  return lowered;
}

std::string_view strip_typed_initializer_prefix(std::string_view init_text,
                                                std::string_view expected_type_text) {
  const auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  const auto trimmed_type = c4c::codegen::lir::trim_lir_arg_text(expected_type_text);
  if (trimmed_type.empty() || trimmed_init.size() <= trimmed_type.size() ||
      trimmed_init.substr(0, trimmed_type.size()) != trimmed_type ||
      trimmed_init[trimmed_type.size()] != ' ') {
    return trimmed_init;
  }
  return c4c::codegen::lir::trim_lir_arg_text(trimmed_init.substr(trimmed_type.size() + 1));
}

bool append_zero_aggregate_initializer(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Scalar: {
      const auto zero_value = lower_zero_initializer_value(layout.scalar_type);
      if (!zero_value.has_value()) {
        return false;
      }
      out->push_back(*zero_value);
      if (layout.scalar_type == bir::TypeKind::Ptr) {
        pointer_offsets->erase(byte_offset);
      }
      return true;
    }
    case AggregateTypeLayout::Kind::Array:
      for (std::size_t index = 0; index < layout.array_count; ++index) {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            !append_zero_aggregate_initializer(layout.element_type_text,
                                              type_decls,
                                              out,
                                              pointer_offsets,
                                              byte_offset + index * element_layout.size_bytes)) {
          return false;
        }
      }
      return true;
    case AggregateTypeLayout::Kind::Struct:
      for (const auto& field : layout.fields) {
        if (!append_zero_aggregate_initializer(field.type_text,
                                              type_decls,
                                              out,
                                              pointer_offsets,
                                              byte_offset + field.byte_offset)) {
          return false;
        }
      }
      return true;
    default:
      return false;
  }
}

bool lower_aggregate_initializer_recursive(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::vector<bir::Value>* out,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets,
    std::size_t byte_offset) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid) {
    return false;
  }

  auto trimmed_init = c4c::codegen::lir::trim_lir_arg_text(init_text);
  if (trimmed_init.empty()) {
    return false;
  }
  trimmed_init = strip_typed_initializer_prefix(trimmed_init, type_text);

  if (trimmed_init == "zeroinitializer") {
    return append_zero_aggregate_initializer(
        type_text, type_decls, out, pointer_offsets, byte_offset);
  }

  if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
    if (layout.scalar_type == bir::TypeKind::Ptr) {
      if (const auto address = parse_global_address_initializer(trimmed_init, type_decls);
          address.has_value()) {
        pointer_offsets->emplace(byte_offset, *address);
        out->push_back(bir::Value::named(bir::TypeKind::Ptr, "@" + address->global_name));
        return true;
      }
    }
    const auto value = lower_global_initializer(trimmed_init, layout.scalar_type);
    if (!value.has_value()) {
      return false;
    }
    if (layout.scalar_type == bir::TypeKind::Ptr) {
      pointer_offsets->erase(byte_offset);
    }
    out->push_back(*value);
    return true;
  }

  if (layout.kind == AggregateTypeLayout::Kind::Array) {
    if (trimmed_init.front() != '[' || trimmed_init.back() != ']') {
      return false;
    }
    const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
    if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        element_layout.size_bytes == 0) {
      return false;
    }
    const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
    const auto items = split_top_level_initializer_items(body);
    if (items.size() > layout.array_count) {
      return false;
    }
    for (std::size_t index = 0; index < items.size(); ++index) {
      if (!lower_aggregate_initializer_recursive(items[index],
                                                 layout.element_type_text,
                                                 type_decls,
                                                 out,
                                                 pointer_offsets,
                                                 byte_offset + index * element_layout.size_bytes)) {
        return false;
      }
    }
    for (std::size_t index = items.size(); index < layout.array_count; ++index) {
      if (!append_zero_aggregate_initializer(layout.element_type_text,
                                             type_decls,
                                             out,
                                             pointer_offsets,
                                             byte_offset + index * element_layout.size_bytes)) {
        return false;
      }
    }
    return true;
  }

  if (trimmed_init.front() != '{' || trimmed_init.back() != '}') {
    return false;
  }
  const auto body = trimmed_init.substr(1, trimmed_init.size() - 2);
  const auto items = split_top_level_initializer_items(body);
  if (items.size() > layout.fields.size()) {
    return false;
  }
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (!lower_aggregate_initializer_recursive(items[index],
                                               layout.fields[index].type_text,
                                               type_decls,
                                               out,
                                               pointer_offsets,
                                               byte_offset + layout.fields[index].byte_offset)) {
      return false;
    }
  }
  for (std::size_t index = items.size(); index < layout.fields.size(); ++index) {
    if (!append_zero_aggregate_initializer(layout.fields[index].type_text,
                                           type_decls,
                                           out,
                                           pointer_offsets,
                                           byte_offset + layout.fields[index].byte_offset)) {
      return false;
    }
  }
  return true;
}

std::optional<std::vector<bir::Value>> lower_aggregate_initializer(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets) {
  std::vector<bir::Value> lowered;
  if (!lower_aggregate_initializer_recursive(
          init_text, type_text, type_decls, &lowered, pointer_offsets, 0)) {
    return std::nullopt;
  }
  return lowered;
}

std::optional<ParsedTypedOperand> parse_typed_operand(std::string_view text) {
  const auto space = text.find(' ');
  if (space == std::string_view::npos || space == 0 || space + 1 >= text.size()) {
    return std::nullopt;
  }
  return ParsedTypedOperand{
      .type_text = std::string(text.substr(0, space)),
      .operand = c4c::codegen::lir::LirOperand(std::string(text.substr(space + 1))),
  };
}

std::optional<std::int64_t> resolve_index_operand(const c4c::codegen::lir::LirOperand& operand,
                                                  const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Immediate ||
      operand.kind() == c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return parse_i64(operand.str());
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto alias = value_aliases.find(operand.str());
  if (alias == value_aliases.end() || alias->second.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  return alias->second.immediate;
}

std::optional<GlobalAddress> resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
  std::size_t byte_offset = 0;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0) {
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  return GlobalAddress{
      .global_name = std::string(global_name),
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<GlobalAddress> resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
  std::size_t byte_offset = base_address.byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
      if (index_pos + 1 != gep.indices.size()) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    if (index_pos == 0) {
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  return GlobalAddress{
      .global_name = base_address.global_name,
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<bir::BinaryOpcode> lower_scalar_binary_opcode(
    const c4c::codegen::lir::LirBinaryOpcodeRef& opcode) {
  using c4c::codegen::lir::LirBinaryOpcode;
  switch (opcode.typed().value_or(LirBinaryOpcode::FNeg)) {
    case LirBinaryOpcode::Add:
      return bir::BinaryOpcode::Add;
    case LirBinaryOpcode::Sub:
      return bir::BinaryOpcode::Sub;
    case LirBinaryOpcode::Mul:
      return bir::BinaryOpcode::Mul;
    case LirBinaryOpcode::SDiv:
      return bir::BinaryOpcode::SDiv;
    case LirBinaryOpcode::UDiv:
      return bir::BinaryOpcode::UDiv;
    case LirBinaryOpcode::SRem:
      return bir::BinaryOpcode::SRem;
    case LirBinaryOpcode::URem:
      return bir::BinaryOpcode::URem;
    case LirBinaryOpcode::And:
      return bir::BinaryOpcode::And;
    case LirBinaryOpcode::Or:
      return bir::BinaryOpcode::Or;
    case LirBinaryOpcode::Xor:
      return bir::BinaryOpcode::Xor;
    case LirBinaryOpcode::Shl:
      return bir::BinaryOpcode::Shl;
    case LirBinaryOpcode::LShr:
      return bir::BinaryOpcode::LShr;
    case LirBinaryOpcode::AShr:
      return bir::BinaryOpcode::AShr;
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> fold_i64_binary_immediates(bir::BinaryOpcode opcode,
                                                     std::int64_t lhs,
                                                     std::int64_t rhs) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return bir::Value::immediate_i64(lhs + rhs);
    case bir::BinaryOpcode::Sub:
      return bir::Value::immediate_i64(lhs - rhs);
    case bir::BinaryOpcode::Mul:
      return bir::Value::immediate_i64(lhs * rhs);
    case bir::BinaryOpcode::SDiv:
      if (rhs == 0) {
        return std::nullopt;
      }
      return bir::Value::immediate_i64(lhs / rhs);
    case bir::BinaryOpcode::And:
      return bir::Value::immediate_i64(lhs & rhs);
    case bir::BinaryOpcode::Or:
      return bir::Value::immediate_i64(lhs | rhs);
    case bir::BinaryOpcode::Xor:
      return bir::Value::immediate_i64(lhs ^ rhs);
    case bir::BinaryOpcode::Shl:
      return bir::Value::immediate_i64(lhs << rhs);
    case bir::BinaryOpcode::LShr:
      return bir::Value::immediate_i64(
          static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs) >> rhs));
    case bir::BinaryOpcode::AShr:
      return bir::Value::immediate_i64(lhs >> rhs);
    default:
      return std::nullopt;
  }
}

std::optional<bir::BinaryOpcode> lower_cmp_predicate(
    const c4c::codegen::lir::LirCmpPredicateRef& predicate) {
  using c4c::codegen::lir::LirCmpPredicate;
  switch (predicate.typed().value_or(LirCmpPredicate::Ord)) {
    case LirCmpPredicate::Eq:
      return bir::BinaryOpcode::Eq;
    case LirCmpPredicate::Ne:
      return bir::BinaryOpcode::Ne;
    case LirCmpPredicate::Slt:
      return bir::BinaryOpcode::Slt;
    case LirCmpPredicate::Sle:
      return bir::BinaryOpcode::Sle;
    case LirCmpPredicate::Sgt:
      return bir::BinaryOpcode::Sgt;
    case LirCmpPredicate::Sge:
      return bir::BinaryOpcode::Sge;
    case LirCmpPredicate::Ult:
      return bir::BinaryOpcode::Ult;
    case LirCmpPredicate::Ule:
      return bir::BinaryOpcode::Ule;
    case LirCmpPredicate::Ugt:
      return bir::BinaryOpcode::Ugt;
    case LirCmpPredicate::Uge:
      return bir::BinaryOpcode::Uge;
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> lower_value(const c4c::codegen::lir::LirOperand& operand,
                                      bir::TypeKind expected_type,
                                      const ValueMap& value_aliases) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    const auto alias = value_aliases.find(operand.str());
    if (alias != value_aliases.end()) {
      return alias->second;
    }
    return bir::Value::named(expected_type, operand.str());
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Immediate &&
      operand.kind() != c4c::codegen::lir::LirOperandKind::SpecialToken) {
    return std::nullopt;
  }

  if (expected_type == bir::TypeKind::I1) {
    if (operand == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (operand == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  const auto parsed = parse_i64(operand.str());
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (expected_type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(*parsed != 0);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*parsed));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*parsed));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*parsed);
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> lower_typed_index_value(const ParsedTypedOperand& index_operand,
                                                  const ValueMap& value_aliases) {
  const auto index_type = lower_integer_type(index_operand.type_text);
  if (!index_type.has_value() ||
      (*index_type != bir::TypeKind::I32 && *index_type != bir::TypeKind::I64)) {
    return std::nullopt;
  }
  return lower_value(index_operand.operand, *index_type, value_aliases);
}

std::optional<bir::Value> make_index_immediate(bir::TypeKind type, std::size_t value) {
  switch (type) {
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(value));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(static_cast<std::int64_t>(value));
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> synthesize_pointer_array_selects(
    std::string_view result_name,
    const std::vector<bir::Value>& element_values,
    const bir::Value& index_value,
    std::vector<bir::Inst>* lowered_insts) {
  if (element_values.empty() ||
      (index_value.type != bir::TypeKind::I32 && index_value.type != bir::TypeKind::I64)) {
    return std::nullopt;
  }
  if (element_values.size() == 1) {
    return element_values.front();
  }

  bir::Value current = element_values.back();
  for (std::size_t rev_index = element_values.size() - 1; rev_index-- > 0;) {
    const auto compare_rhs = make_index_immediate(index_value.type, rev_index);
    if (!compare_rhs.has_value()) {
      return std::nullopt;
    }

    const bool is_final = rev_index == 0;
    const std::string select_name =
        is_final ? std::string(result_name)
                 : std::string(result_name) + ".sel" + std::to_string(rev_index);
    lowered_insts->push_back(bir::SelectInst{
        .predicate = bir::BinaryOpcode::Eq,
        .result = bir::Value::named(bir::TypeKind::Ptr, select_name),
        .compare_type = index_value.type,
        .lhs = index_value,
        .rhs = *compare_rhs,
        .true_value = element_values[rev_index],
        .false_value = current,
    });
    current = bir::Value::named(bir::TypeKind::Ptr, select_name);
  }
  return current;
}

std::optional<std::vector<bir::Value>> collect_local_pointer_values(
    const std::vector<std::string>& element_slots,
    const LocalPointerValueAliasMap& local_pointer_value_aliases) {
  if (element_slots.empty()) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(element_slots.size());
  for (const auto& slot_name : element_slots) {
    const auto alias_it = local_pointer_value_aliases.find(slot_name);
    if (alias_it == local_pointer_value_aliases.end() || alias_it->second.type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    element_values.push_back(alias_it->second);
  }
  return element_values;
}

std::optional<std::vector<bir::Value>> collect_global_array_pointer_values(
    const DynamicGlobalPointerArrayAccess& access,
    const GlobalTypes& global_types) {
  const auto global_it = global_types.find(access.global_name);
  if (global_it == global_types.end()) {
    return std::nullopt;
  }
  if (access.element_count == 0) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(access.element_count);
  const auto element_stride =
      access.element_stride_bytes == 0 ? type_size_bytes(bir::TypeKind::Ptr)
                                       : access.element_stride_bytes;
  for (std::size_t index = 0; index < access.element_count; ++index) {
    const auto offset = access.byte_offset + index * element_stride;
    const auto init_it = global_it->second.pointer_initializer_offsets.find(offset);
    if (init_it == global_it->second.pointer_initializer_offsets.end() ||
        init_it->second.value_type != bir::TypeKind::Ptr || init_it->second.byte_offset != 0) {
      return std::nullopt;
    }
    element_values.push_back(
        bir::Value::named(bir::TypeKind::Ptr, "@" + init_it->second.global_name));
  }
  return element_values;
}

struct AggregateArrayExtent {
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
};

std::optional<AggregateArrayExtent> find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      if (nested_offset == 0 &&
          c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text) ==
              c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
        return AggregateArrayExtent{
            .element_count = layout.array_count - element_index,
            .element_stride_bytes = element_layout.size_bytes,
        };
      }
      return find_repeated_aggregate_extent_at_offset(
          layout.element_type_text, nested_offset, repeated_type_text, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_repeated_aggregate_extent_at_offset(layout.fields[index].type_text,
                                                        target_offset - field_begin,
                                                        repeated_type_text,
                                                        type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<std::size_t> find_pointer_array_length_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      if (target_offset == 0 &&
          element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
          element_layout.scalar_type == bir::TypeKind::Ptr) {
        return layout.array_count;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      return find_pointer_array_length_at_offset(
          layout.element_type_text, nested_offset, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_pointer_array_length_at_offset(
            layout.fields[index].type_text, target_offset - field_begin, type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<DynamicGlobalPointerArrayAccess> resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = initial_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0 && !relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    if (index_pos == 0 && relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value < 0) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 ||
              static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value() || index_pos + 1 != gep.indices.size() ||
            element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            element_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }
        return DynamicGlobalPointerArrayAccess{
            .global_name = std::string(global_name),
            .byte_offset = byte_offset,
            .element_count = layout.array_count,
            .index = *lowered_index,
        };
      }
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<DynamicGlobalAggregateArrayAccess> resolve_global_dynamic_aggregate_array_access(
    const GlobalAddress& base_address,
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls) {
  if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto base_layout =
      compute_aggregate_type_layout(c4c::codegen::lir::trim_lir_arg_text(base_type_text), type_decls);
  if (base_layout.kind != AggregateTypeLayout::Kind::Struct &&
      base_layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices.front());
  if (!parsed_index.has_value() ||
      resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
    return std::nullopt;
  }

  const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
  if (!lowered_index.has_value()) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(base_address.global_name);
  if (global_it == global_types.end()) {
    return std::nullopt;
  }

  const auto extent = find_repeated_aggregate_extent_at_offset(
      global_it->second.type_text, base_address.byte_offset, base_type_text, type_decls);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicGlobalAggregateArrayAccess{
      .global_name = base_address.global_name,
      .element_type_text =
          std::string(c4c::codegen::lir::trim_lir_arg_text(base_type_text)),
      .byte_offset = base_address.byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .index = *lowered_index,
  };
}

void record_pointer_global_object_alias(std::string_view result_name,
                                        const GlobalInfo& global_info,
                                        const GlobalTypes& global_types,
                                        GlobalObjectPointerMap& global_object_pointer_slots) {
  if (global_info.initializer_symbol_name.empty() ||
      global_info.initializer_offset_type != bir::TypeKind::Void ||
      global_info.initializer_byte_offset != 0) {
    return;
  }

  const auto pointee_it = global_types.find(global_info.initializer_symbol_name);
  if (pointee_it == global_types.end() || !pointee_it->second.supports_direct_value ||
      pointee_it->second.value_type != bir::TypeKind::Ptr) {
    return;
  }

  global_object_pointer_slots[std::string(result_name)] = GlobalAddress{
      .global_name = global_info.initializer_symbol_name,
      .value_type = bir::TypeKind::Ptr,
      .byte_offset = 0,
  };
}

std::optional<GlobalAddress> resolve_pointer_store_address(
    const c4c::codegen::lir::LirOperand& operand,
    const GlobalPointerMap& global_pointer_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = operand.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end()) {
      if (!is_known_function_symbol(global_name, function_symbols)) {
        return std::nullopt;
      }
      return GlobalAddress{
          .global_name = global_name,
          .value_type = bir::TypeKind::Ptr,
          .byte_offset = 0,
      };
    }
    if (!global_it->second.supports_linear_addressing) {
      return std::nullopt;
    }
    return GlobalAddress{
        .global_name = global_name,
        .value_type = global_it->second.value_type,
        .byte_offset = 0,
    };
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto global_ptr_it = global_pointer_slots.find(operand.str());
  if (global_ptr_it == global_pointer_slots.end()) {
    return std::nullopt;
  }
  return global_ptr_it->second;
}

bool append_local_aggregate_scalar_slots(std::string_view type_text,
                                         std::string_view slot_prefix,
                                         std::size_t byte_offset,
                                         std::size_t align_bytes,
                                         const TypeDeclMap& type_decls,
                                         LocalSlotTypes& local_slot_types,
                                         LocalPointerSlots& local_pointer_slots,
                                         LocalAggregateFieldSet& local_aggregate_field_slots,
                                         bir::Function* lowered_function,
                                         LocalAggregateSlots* aggregate_slots) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return false;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Scalar: {
      const std::string slot_name =
          std::string(slot_prefix) + "." + std::to_string(byte_offset);
      local_slot_types.emplace(slot_name, layout.scalar_type);
      local_pointer_slots.emplace(slot_name, slot_name);
      local_aggregate_field_slots.insert(slot_name);
      aggregate_slots->leaf_slots.emplace(byte_offset, slot_name);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = layout.scalar_type,
          .align_bytes = align_bytes > 0 ? align_bytes : layout.align_bytes,
      });
      return true;
    }
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return false;
      }
      for (std::size_t index = 0; index < layout.array_count; ++index) {
        if (!append_local_aggregate_scalar_slots(layout.element_type_text,
                                                 slot_prefix,
                                                 byte_offset + index * element_layout.size_bytes,
                                                 align_bytes,
                                                 type_decls,
                                                 local_slot_types,
                                                 local_pointer_slots,
                                                 local_aggregate_field_slots,
                                                 lowered_function,
                                                 aggregate_slots)) {
          return false;
        }
      }
      return true;
    }
    case AggregateTypeLayout::Kind::Struct:
      for (const auto& field : layout.fields) {
        if (!append_local_aggregate_scalar_slots(field.type_text,
                                                 slot_prefix,
                                                 byte_offset + field.byte_offset,
                                                 align_bytes,
                                                 type_decls,
                                                 local_slot_types,
                                                 local_pointer_slots,
                                                 local_aggregate_field_slots,
                                                 lowered_function,
                                                 aggregate_slots)) {
          return false;
        }
      }
      return true;
    default:
      return false;
  }
}

std::optional<std::string> resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = 0;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
        if (*index_value != 0) {
          return std::nullopt;
        }
        continue;
      }
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  if (leaf_layout.kind != AggregateTypeLayout::Kind::Scalar) {
    return std::nullopt;
  }

  const auto slot_it = aggregate_slots.leaf_slots.find(byte_offset);
  if (slot_it == aggregate_slots.leaf_slots.end()) {
    return std::nullopt;
  }
  return slot_it->second;
}

std::optional<std::vector<std::string>> resolve_local_aggregate_pointer_array_slots(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = 0;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto layout = compute_aggregate_type_layout(current_type, type_decls);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
      element_layout.scalar_type != bir::TypeKind::Ptr || element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  element_slots.reserve(layout.array_count);
  for (std::size_t index = 0; index < layout.array_count; ++index) {
    const auto slot_it = aggregate_slots.leaf_slots.find(byte_offset + index * element_layout.size_bytes);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    element_slots.push_back(slot_it->second);
  }
  return element_slots;
}

std::optional<DynamicLocalPointerArrayAccess> resolve_local_aggregate_dynamic_pointer_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = 0;
  bool saw_base_index = false;

  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      saw_base_index = true;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 ||
              static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value() || index_pos + 1 != gep.indices.size() ||
            element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            element_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }

        std::vector<std::string> element_slots;
        element_slots.reserve(layout.array_count);
        for (std::size_t element_index = 0; element_index < layout.array_count; ++element_index) {
          const auto slot_it =
              aggregate_slots.leaf_slots.find(byte_offset + element_index * element_layout.size_bytes);
          if (slot_it == aggregate_slots.leaf_slots.end()) {
            return std::nullopt;
          }
          element_slots.push_back(slot_it->second);
        }
        return DynamicLocalPointerArrayAccess{
            .element_slots = std::move(element_slots),
            .index = *lowered_index,
        };
      }
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<bir::Value> resolve_local_aggregate_pointer_value_alias(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    return lower_value(operand, bir::TypeKind::Ptr, value_aliases);
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return std::nullopt;
  }

  const std::string symbol_name = operand.str().substr(1);
  if (!is_known_function_symbol(symbol_name, function_symbols)) {
    return std::nullopt;
  }
  return bir::Value::named(bir::TypeKind::Ptr, "@" + symbol_name);
}

std::optional<GlobalAddress> resolve_honest_pointer_base(const GlobalAddress& address,
                                                         const GlobalTypes& global_types) {
  if (address.value_type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(address.global_name);
  if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
      global_it->second.value_type != bir::TypeKind::Ptr ||
      !global_it->second.known_global_address.has_value()) {
    return std::nullopt;
  }

  auto resolved = *global_it->second.known_global_address;
  resolved.byte_offset += address.byte_offset;

  const auto base_it = global_types.find(resolved.global_name);
  if (base_it == global_types.end() || !base_it->second.supports_linear_addressing ||
      resolved.byte_offset >= base_it->second.storage_size_bytes) {
    return std::nullopt;
  }

  return resolved;
}

std::optional<GlobalAddress> resolve_honest_addressed_global_access(
    const GlobalAddress& address,
    bir::TypeKind accessed_type,
    const GlobalTypes& global_types) {
  if (accessed_type == bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  auto resolved = resolve_honest_pointer_base(address, global_types);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  resolved->value_type = accessed_type;

  return resolved;
}

GlobalPointerSlotKey make_global_pointer_slot_key(const GlobalAddress& address) {
  return GlobalPointerSlotKey{
      .global_name = address.global_name,
      .byte_offset = address.byte_offset,
  };
}

std::optional<bir::TypeKind> lower_param_type(const c4c::TypeSpec& type) {
  if (type.base == TB_BOOL && type.ptr_level == 0 && type.array_rank == 0) {
    return bir::TypeKind::I1;
  }
  return backend_lir_lower_minimal_scalar_type(type);
}

std::optional<bir::Value> lower_global_initializer(std::string_view text,
                                                   bir::TypeKind type) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  if (trimmed == "zeroinitializer") {
    switch (type) {
      case bir::TypeKind::I1:
        return bir::Value::immediate_i1(false);
      case bir::TypeKind::I8:
        return bir::Value::immediate_i8(0);
      case bir::TypeKind::I32:
        return bir::Value::immediate_i32(0);
      case bir::TypeKind::I64:
        return bir::Value::immediate_i64(0);
      default:
        return std::nullopt;
    }
  }

  if (type == bir::TypeKind::I1) {
    if (trimmed == "true") {
      return bir::Value::immediate_i1(true);
    }
    if (trimmed == "false") {
      return bir::Value::immediate_i1(false);
    }
  }

  const auto parsed = parse_i64(trimmed);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(*parsed != 0);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*parsed));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*parsed));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*parsed);
    default:
      return std::nullopt;
  }
}

std::optional<bir::Global> lower_scalar_global(const c4c::codegen::lir::LirGlobal& global,
                                               const TypeDeclMap& type_decls) {
  if (backend_lir_global_uses_nonminimal_types(global)) {
    return std::nullopt;
  }

  const auto lowered_type = lower_integer_type(global.llvm_type);
  if (!lowered_type.has_value()) {
    return std::nullopt;
  }

  bir::Global lowered;
  lowered.name = global.name;
  lowered.type = *lowered_type;
  lowered.is_extern = global.is_extern_decl;
  lowered.is_constant = global.is_const;
  lowered.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;
  if (!global.is_extern_decl) {
    if (*lowered_type == bir::TypeKind::Ptr) {
      const auto initializer_address = parse_global_address_initializer(global.init_text, type_decls);
      if (!initializer_address.has_value()) {
        return std::nullopt;
      }
      lowered.initializer_symbol_name = initializer_address->global_name;
    } else {
      const auto initializer = lower_global_initializer(global.init_text, *lowered_type);
      if (!initializer.has_value()) {
        return std::nullopt;
      }
      lowered.initializer = *initializer;
    }
  }
  return lowered;
}

std::optional<bir::Global> lower_minimal_global(const c4c::codegen::lir::LirGlobal& global,
                                                const TypeDeclMap& type_decls,
                                                GlobalInfo* info) {
  if (auto lowered = lower_scalar_global(global, type_decls); lowered.has_value()) {
    info->value_type = lowered->type;
    info->element_size_bytes = type_size_bytes(lowered->type);
    info->element_count = 1;
    info->storage_size_bytes = info->element_size_bytes;
    info->supports_direct_value = true;
    info->supports_linear_addressing = true;
    if (lowered->initializer_symbol_name.has_value()) {
      const auto initializer_address = parse_global_address_initializer(global.init_text, type_decls);
      if (!initializer_address.has_value()) {
        return std::nullopt;
      }
      info->initializer_symbol_name = initializer_address->global_name;
      info->initializer_offset_type = initializer_address->value_type;
      info->initializer_byte_offset = initializer_address->byte_offset;
      info->supports_linear_addressing = false;
    }
    if (lowered->size_bytes == 0) {
      lowered->size_bytes = info->element_size_bytes;
    }
    return lowered;
  }

  if (const auto integer_array = parse_integer_array_type(global.llvm_type);
      integer_array.has_value() && integer_array->element_type != bir::TypeKind::Ptr) {
    const auto element_size_bytes = type_size_bytes(integer_array->element_type);
    if (element_size_bytes == 0) {
      return std::nullopt;
    }

    std::size_t total_elements = 1;
    for (const auto extent : integer_array->extents) {
      total_elements *= extent;
    }

    bir::Global lowered;
    lowered.name = global.name;
    lowered.type = integer_array->element_type;
    lowered.is_extern = global.is_extern_decl;
    lowered.is_constant = global.is_const;
    lowered.size_bytes = total_elements * element_size_bytes;
    lowered.align_bytes =
        global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes) : 0;
    if (!global.is_extern_decl) {
      const auto initializer_elements =
          lower_integer_array_initializer(global.init_text, global.llvm_type);
      if (!initializer_elements.has_value()) {
        return std::nullopt;
      }
      lowered.initializer_elements = *initializer_elements;
    }

    info->value_type = integer_array->element_type;
    info->element_size_bytes = element_size_bytes;
    info->element_count = total_elements;
    info->storage_size_bytes = lowered.size_bytes;
    info->supports_direct_value = false;
    info->supports_linear_addressing = true;
    info->type_text = global.llvm_type;
    return lowered;
  }

  const auto layout = compute_aggregate_type_layout(global.llvm_type, type_decls);
  if ((layout.kind != AggregateTypeLayout::Kind::Struct &&
       layout.kind != AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0) {
    return std::nullopt;
  }

  bir::Global aggregate;
  aggregate.name = global.name;
  aggregate.type = bir::TypeKind::I8;
  aggregate.is_extern = global.is_extern_decl;
  aggregate.is_constant = global.is_const;
  aggregate.size_bytes = layout.size_bytes;
  aggregate.align_bytes = global.align_bytes > 0 ? static_cast<std::size_t>(global.align_bytes)
                                                 : layout.align_bytes;
  if (!global.is_extern_decl) {
    std::unordered_map<std::size_t, GlobalAddress> pointer_offsets;
    const auto initializer_elements =
        lower_aggregate_initializer(global.init_text, global.llvm_type, type_decls, &pointer_offsets);
    if (!initializer_elements.has_value()) {
      return std::nullopt;
    }
    aggregate.initializer_elements = *initializer_elements;
    info->pointer_initializer_offsets = std::move(pointer_offsets);
  }

  info->value_type = bir::TypeKind::I8;
  info->element_size_bytes = 1;
  info->element_count = layout.size_bytes;
  info->storage_size_bytes = layout.size_bytes;
  info->supports_direct_value = false;
  info->supports_linear_addressing = true;
  info->type_text = global.llvm_type;
  return aggregate;
}

std::optional<bir::Global> lower_string_constant_global(
    const c4c::codegen::lir::LirStringConst& string_constant,
    GlobalInfo* info) {
  if (string_constant.pool_name.empty() || string_constant.byte_length <= 0) {
    return std::nullopt;
  }

  bir::Global lowered;
  lowered.name = string_constant.pool_name.front() == '@'
                     ? string_constant.pool_name.substr(1)
                     : string_constant.pool_name;
  lowered.type = bir::TypeKind::I8;
  lowered.is_extern = true;
  lowered.is_constant = true;
  lowered.size_bytes = static_cast<std::size_t>(string_constant.byte_length);
  lowered.align_bytes = 1;

  info->value_type = bir::TypeKind::I8;
  info->element_size_bytes = 1;
  info->element_count = static_cast<std::size_t>(string_constant.byte_length);
  info->storage_size_bytes = static_cast<std::size_t>(string_constant.byte_length);
  info->supports_direct_value = false;
  info->supports_linear_addressing = true;
  info->type_text = "[" + std::to_string(string_constant.byte_length) + " x i8]";
  return lowered;
}

bool is_void_param_sentinel(const c4c::TypeSpec& type) {
  return type.base == TB_VOID && type.ptr_level == 0 && type.array_rank == 0;
}

std::optional<bir::TypeKind> infer_function_return_type(
    const c4c::codegen::lir::LirFunction& function) {
  std::optional<bir::TypeKind> return_type;
  for (const auto& block : function.blocks) {
    const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator);
    if (ret == nullptr) {
      continue;
    }
    const auto lowered_type = lower_integer_type(ret->type_str);
    if (!lowered_type.has_value()) {
      return std::nullopt;
    }
    if (!return_type.has_value()) {
      return_type = lowered_type;
      continue;
    }
    if (*return_type != *lowered_type) {
      return std::nullopt;
    }
  }
  return return_type;
}

bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                           bir::Function* lowered);

std::optional<bir::TypeKind> lower_signature_return_type(std::string_view signature_text) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return std::nullopt;
  }
  return lower_integer_type(
      c4c::codegen::lir::trim_lir_arg_text(line.substr(first_space + 1, at_pos - first_space - 1)));
}

std::optional<bir::Function> lower_extern_decl(const c4c::codegen::lir::LirExternDecl& decl) {
  auto return_type = lower_integer_type(decl.return_type_str);
  if (!return_type.has_value()) {
    return_type = lower_integer_type(decl.return_type.str());
  }
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = decl.name;
  lowered.return_type = *return_type;
  lowered.is_declaration = true;
  return lowered;
}

std::optional<bir::Function> lower_decl_function(const c4c::codegen::lir::LirFunction& function) {
  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = lower_param_type(function.return_type)
                            .value_or(bir::TypeKind::Void);
  if (lowered.return_type == bir::TypeKind::Void) {
    const auto signature_return_type = lower_signature_return_type(function.signature_text);
    if (!signature_return_type.has_value()) {
      return std::nullopt;
    }
    lowered.return_type = *signature_return_type;
  }
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }
  lowered.is_declaration = true;
  return lowered;
}

bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                           bir::Function* lowered) {
  if (function.params.size() == 1 && is_void_param_sentinel(function.params.front().second)) {
    return true;
  }

  for (const auto& param : function.params) {
    const auto lowered_type = lower_param_type(param.second);
    if (!lowered_type.has_value()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.first,
    });
  }

  if (!lowered->params.empty()) {
    return true;
  }

  const auto parsed_params = parse_backend_function_signature_params(function.signature_text);
  if (!parsed_params.has_value()) {
    return true;
  }

  if (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
      c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void") {
    return true;
  }

  for (const auto& param : *parsed_params) {
    if (param.is_varargs) {
      return false;
    }
    const auto lowered_type = lower_integer_type(param.type);
    if (!lowered_type.has_value() || param.operand.empty()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.operand,
    });
  }
  return true;
}

bool lower_scalar_compare_inst(const c4c::codegen::lir::LirInst& inst,
                               ValueMap& value_aliases,
                               CompareMap& compare_exprs,
                               std::vector<bir::Inst>* lowered_insts) {
  if (const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&inst)) {
    if (cmp->is_float) {
      return false;
    }
    const auto operand_type = lower_integer_type(cmp->type_str.str());
    const auto opcode = lower_cmp_predicate(cmp->predicate);
    if (!operand_type.has_value() || !opcode.has_value()) {
      return false;
    }
    const auto lhs = lower_value(cmp->lhs, *operand_type, value_aliases);
    const auto rhs = lower_value(cmp->rhs, *operand_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*opcode == bir::BinaryOpcode::Ne && *operand_type == bir::TypeKind::I32 &&
        cmp->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        cmp->rhs.kind() == c4c::codegen::lir::LirOperandKind::Immediate) {
      const auto alias = value_aliases.find(cmp->lhs.str());
      const auto compare_alias = compare_exprs.find(cmp->lhs.str());
      const auto imm = parse_i64(cmp->rhs.str());
      if (alias != value_aliases.end() && compare_alias != compare_exprs.end() &&
          imm.has_value() && *imm == 0) {
        value_aliases[cmp->result.str()] = alias->second;
        compare_exprs[cmp->result.str()] = compare_alias->second;
        return true;
      }
    }

    const auto result = bir::Value::named(bir::TypeKind::I1, cmp->result.str());
    const CompareExpr expr{
        .opcode = *opcode,
        .operand_type = *operand_type,
        .lhs = *lhs,
        .rhs = *rhs,
    };
    value_aliases[cmp->result.str()] = result;
    compare_exprs[cmp->result.str()] = expr;

    if (lowered_insts != nullptr) {
      lowered_insts->push_back(bir::BinaryInst{
          .opcode = *opcode,
          .result = result,
          .operand_type = *operand_type,
          .lhs = *lhs,
          .rhs = *rhs,
      });
    }
    return true;
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt &&
        from_type == bir::TypeKind::I1 && to_type == bir::TypeKind::I32 &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto alias = value_aliases.find(cast->operand.str());
      if (alias != value_aliases.end()) {
        value_aliases[cast->result.str()] = alias->second;
        const auto compare_alias = compare_exprs.find(cast->operand.str());
        if (compare_alias != compare_exprs.end()) {
          compare_exprs[cast->result.str()] = compare_alias->second;
        }
        return true;
      }
    }

    if ((cast->kind == c4c::codegen::lir::LirCastKind::SExt ||
         cast->kind == c4c::codegen::lir::LirCastKind::ZExt) &&
        cast->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        from_type.has_value() && to_type.has_value()) {
      const auto value = lower_value(cast->operand, *from_type, value_aliases);
      if (!value.has_value()) {
        return false;
      }

      if (value->kind == bir::Value::Kind::Immediate) {
        const auto imm = value->immediate;
        switch (*to_type) {
          case bir::TypeKind::I32:
            value_aliases[cast->result.str()] = bir::Value::immediate_i32(
                static_cast<std::int32_t>(imm));
            return true;
          case bir::TypeKind::I64:
            value_aliases[cast->result.str()] = bir::Value::immediate_i64(imm);
            return true;
          default:
            break;
        }
      }
    }
  }

  return false;
}

bool lower_scalar_or_local_memory_inst(const c4c::codegen::lir::LirInst& inst,
                                       ValueMap& value_aliases,
                                       CompareMap& compare_exprs,
                                       LocalSlotTypes& local_slot_types,
                                       LocalPointerSlots& local_pointer_slots,
                                       LocalArraySlotMap& local_array_slots,
                                       LocalPointerArrayBaseMap& local_pointer_array_bases,
                                       DynamicLocalPointerArrayMap& dynamic_local_pointer_arrays,
                                       LocalAggregateSlotMap& local_aggregate_slots,
                                       LocalAggregateFieldSet& local_aggregate_field_slots,
                                       LocalPointerValueAliasMap& local_pointer_value_aliases,
                                       LocalAddressSlots& local_address_slots,
                                       GlobalAddressSlots& global_address_slots,
                                       AddressedGlobalPointerSlots& addressed_global_pointer_slots,
                                       GlobalPointerMap& global_pointer_slots,
                                       DynamicGlobalPointerArrayMap& dynamic_global_pointer_arrays,
                                       DynamicGlobalAggregateArrayMap& dynamic_global_aggregate_arrays,
                                       GlobalObjectPointerMap& global_object_pointer_slots,
                                       GlobalAddressIntMap& global_address_ints,
                                       GlobalObjectAddressIntMap& global_object_address_ints,
                                       const GlobalTypes& global_types,
                                       const FunctionSymbolSet& function_symbols,
                                       const TypeDeclMap& type_decls,
                                       bir::Function* lowered_function,
                                       std::vector<bir::Inst>* lowered_insts) {
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, lowered_insts)) {
    return true;
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    if (cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::PtrToInt &&
        cast->to_type.str() == "i64") {
      if (cast->operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = cast->operand.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end()) {
          return false;
        }
        if (global_it->second.supports_linear_addressing) {
          global_address_ints[cast->result.str()] = GlobalAddress{
              .global_name = global_name,
              .value_type = global_it->second.value_type,
              .byte_offset = 0,
          };
          return true;
        }
        if (!global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        global_object_address_ints[cast->result.str()] = GlobalAddress{
            .global_name = global_name,
            .value_type = bir::TypeKind::Ptr,
            .byte_offset = 0,
        };
        return true;
      }

      if (cast->operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }

      const auto global_object_it = global_object_pointer_slots.find(cast->operand.str());
      if (global_object_it != global_object_pointer_slots.end()) {
        global_object_address_ints[cast->result.str()] = global_object_it->second;
        return true;
      }

      const auto global_ptr_it = global_pointer_slots.find(cast->operand.str());
      if (global_ptr_it == global_pointer_slots.end()) {
        return false;
      }
      global_address_ints[cast->result.str()] = global_ptr_it->second;
      return true;
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::IntToPtr &&
        cast->from_type.str() == "i64" && cast->to_type.str() == "ptr" &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto global_object_addr_it = global_object_address_ints.find(cast->operand.str());
      if (global_object_addr_it != global_object_address_ints.end()) {
        global_object_pointer_slots[cast->result.str()] = global_object_addr_it->second;
        return true;
      }

      const auto global_addr_it = global_address_ints.find(cast->operand.str());
      if (global_addr_it == global_address_ints.end()) {
        return false;
      }
      global_pointer_slots[cast->result.str()] = global_addr_it->second;
      return true;
    }

    const auto opcode = lower_cast_opcode(cast->kind);
    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (!opcode.has_value() || !from_type.has_value() || !to_type.has_value()) {
      return false;
    }

    const auto operand = lower_value(cast->operand, *from_type, value_aliases);
    if (!operand.has_value()) {
      return false;
    }

    lowered_insts->push_back(bir::CastInst{
        .opcode = *opcode,
        .result = bir::Value::named(*to_type, cast->result.str()),
        .operand = *operand,
    });
    return true;
  }

  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto opcode = lower_scalar_binary_opcode(bin->opcode);
    const auto value_type = lower_integer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value()) {
      return false;
    }

    if (*opcode == bir::BinaryOpcode::Sub && *value_type == bir::TypeKind::I64 &&
        bin->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        bin->rhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto lhs_addr_it = global_address_ints.find(bin->lhs.str());
      const auto rhs_addr_it = global_address_ints.find(bin->rhs.str());
      if (lhs_addr_it != global_address_ints.end() && rhs_addr_it != global_address_ints.end() &&
          lhs_addr_it->second.global_name == rhs_addr_it->second.global_name) {
        const auto lhs_offset = static_cast<std::int64_t>(lhs_addr_it->second.byte_offset);
        const auto rhs_offset = static_cast<std::int64_t>(rhs_addr_it->second.byte_offset);
        value_aliases[bin->result.str()] = bir::Value::immediate_i64(lhs_offset - rhs_offset);
        return true;
      }
    }

    const auto lhs = lower_value(bin->lhs, *value_type, value_aliases);
    const auto rhs = lower_value(bin->rhs, *value_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*value_type == bir::TypeKind::I64 && lhs->kind == bir::Value::Kind::Immediate &&
        rhs->kind == bir::Value::Kind::Immediate) {
      const auto folded = fold_i64_binary_immediates(*opcode, lhs->immediate, rhs->immediate);
      if (folded.has_value()) {
        value_aliases[bin->result.str()] = *folded;
        return true;
      }
    }

    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = *lhs,
        .rhs = *rhs,
    });
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    if (alloca->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        !alloca->count.str().empty()) {
      return false;
    }

    const auto slot_type = lower_integer_type(alloca->type_str.str());
    const std::string slot_name = alloca->result.str();
    if (local_slot_types.find(slot_name) != local_slot_types.end() ||
        local_array_slots.find(slot_name) != local_array_slots.end()) {
      return false;
    }

    if (slot_type.has_value()) {
      local_slot_types.emplace(slot_name, *slot_type);
      local_pointer_slots.emplace(slot_name, slot_name);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = *slot_type,
          .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
      });
      return true;
    }

    const auto array_type = parse_local_array_type(alloca->type_str.str());
    if (array_type.has_value()) {
      LocalArraySlots array_slots{.element_type = array_type->second};
      array_slots.element_slots.reserve(array_type->first);
      for (std::size_t index = 0; index < array_type->first; ++index) {
        const std::string element_slot = slot_name + "." + std::to_string(index);
        local_slot_types.emplace(element_slot, array_type->second);
        local_pointer_slots.emplace(element_slot, element_slot);
        array_slots.element_slots.push_back(element_slot);
        lowered_function->local_slots.push_back(bir::LocalSlot{
            .name = element_slot,
            .type = array_type->second,
            .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
        });
      }
      local_array_slots.emplace(slot_name, std::move(array_slots));
      return true;
    }

    const auto aggregate_layout = compute_aggregate_type_layout(alloca->type_str.str(), type_decls);
    if (aggregate_layout.kind != AggregateTypeLayout::Kind::Struct) {
      return false;
    }

    LocalAggregateSlots aggregate_slots{
        .type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(alloca->type_str.str())),
    };
    if (!append_local_aggregate_scalar_slots(alloca->type_str.str(),
                                             slot_name,
                                             0,
                                             alloca->align > 0
                                                 ? static_cast<std::size_t>(alloca->align)
                                                 : 0,
                                             type_decls,
                                             local_slot_types,
                                             local_pointer_slots,
                                             local_aggregate_field_slots,
                                             lowered_function,
                                             &aggregate_slots)) {
      return false;
    }
    local_aggregate_slots.emplace(slot_name, std::move(aggregate_slots));
    return true;
  }

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    if (gep->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    std::string resolved_slot;
    if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = gep->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
        return false;
      }
      const auto resolved_address = resolve_global_gep_address(
          global_name, global_it->second.type_text, *gep, value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          global_name, global_it->second.type_text, 0, false, *gep, value_aliases, type_decls);
      if (!dynamic_array.has_value()) {
        return false;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    }

    if (const auto aggregate_it = local_aggregate_slots.find(gep->ptr.str());
        aggregate_it != local_aggregate_slots.end()) {
      const auto resolved_slot = resolve_local_aggregate_gep_slot(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (resolved_slot.has_value()) {
        local_pointer_slots[gep->result.str()] = *resolved_slot;
        return true;
      }
      const auto pointer_array_slots = resolve_local_aggregate_pointer_array_slots(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (pointer_array_slots.has_value()) {
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = std::move(*pointer_array_slots),
            .base_index = 0,
        };
        return true;
      }
      const auto dynamic_array = resolve_local_aggregate_dynamic_pointer_array_access(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (!dynamic_array.has_value()) {
        return false;
      }
      dynamic_local_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    }

    if (const auto array_it = local_array_slots.find(gep->ptr.str()); array_it != local_array_slots.end()) {
      if (gep->indices.size() != 2) {
        return false;
      }
      const auto base_index = parse_typed_operand(gep->indices[0]);
      const auto elem_index = parse_typed_operand(gep->indices[1]);
      if (!base_index.has_value() || !elem_index.has_value()) {
        return false;
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
      if (!base_imm.has_value() || *base_imm != 0) {
        return false;
      }
      if (elem_imm.has_value()) {
        if (*elem_imm < 0 ||
            static_cast<std::size_t>(*elem_imm) >= array_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot = array_it->second.element_slots[static_cast<std::size_t>(*elem_imm)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_it->second.element_slots,
            .base_index = static_cast<std::size_t>(*elem_imm),
        };
      } else {
        const auto elem_value = lower_typed_index_value(*elem_index, value_aliases);
        if (!elem_value.has_value() || array_it->second.element_type != bir::TypeKind::Ptr) {
          return false;
        }
        dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
            .element_slots = array_it->second.element_slots,
            .index = *elem_value,
        };
        return true;
      }
    } else if (const auto array_base_it = local_pointer_array_bases.find(gep->ptr.str());
               array_base_it != local_pointer_array_bases.end()) {
      if (gep->indices.empty() || gep->indices.size() > 2) {
        return false;
      }
      std::size_t index_pos = 0;
      if (gep->indices.size() == 2) {
        const auto base_index = parse_typed_operand(gep->indices[0]);
        if (!base_index.has_value()) {
          return false;
        }
        const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
        if (!base_imm.has_value() || *base_imm != 0) {
          return false;
        }
        index_pos = 1;
      }
      const auto parsed_index = parse_typed_operand(gep->indices[index_pos]);
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
      if (index_imm.has_value()) {
        const auto final_index = static_cast<std::int64_t>(array_base_it->second.base_index) + *index_imm;
        if (final_index < 0 ||
            static_cast<std::size_t>(final_index) >= array_base_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot =
            array_base_it->second.element_slots[static_cast<std::size_t>(final_index)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_base_it->second.element_slots,
            .base_index = static_cast<std::size_t>(final_index),
        };
      } else {
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value() || array_base_it->second.base_index != 0) {
          return false;
        }
        dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
            .element_slots = array_base_it->second.element_slots,
            .index = *index_value,
        };
        return true;
      }
    } else if (const auto global_ptr_it = global_pointer_slots.find(gep->ptr.str());
               global_ptr_it != global_pointer_slots.end()) {
      auto base_address = global_ptr_it->second;
      if (const auto honest_base = resolve_honest_pointer_base(base_address, global_types);
          honest_base.has_value()) {
        base_address = *honest_base;
      }
      const auto resolved_address = resolve_relative_global_gep_address(
          base_address, gep->element_type.str(), *gep, value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          base_address.global_name,
          gep->element_type.str(),
          base_address.byte_offset,
          true,
          *gep,
          value_aliases,
          type_decls);
      if (!dynamic_array.has_value()) {
        const auto dynamic_aggregate = resolve_global_dynamic_aggregate_array_access(
            base_address, gep->element_type.str(), *gep, value_aliases, global_types, type_decls);
        if (dynamic_aggregate.has_value()) {
          dynamic_global_aggregate_arrays[gep->result.str()] = std::move(*dynamic_aggregate);
          return true;
        }
        if (gep->indices.size() != 1 || base_address.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        const auto parsed_index = parse_typed_operand(gep->indices.front());
        if (!parsed_index.has_value()) {
          return false;
        }
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value()) {
          return false;
        }
        const auto global_it = global_types.find(base_address.global_name);
        if (global_it == global_types.end()) {
          return false;
        }
        const auto array_length = find_pointer_array_length_at_offset(
            global_it->second.type_text, base_address.byte_offset, type_decls);
        if (!array_length.has_value()) {
          return false;
        }
        dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
            .global_name = base_address.global_name,
            .byte_offset = base_address.byte_offset,
            .element_count = *array_length,
            .index = *index_value,
        };
        return true;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    } else if (const auto global_aggregate_it = dynamic_global_aggregate_arrays.find(gep->ptr.str());
               global_aggregate_it != dynamic_global_aggregate_arrays.end()) {
      const auto element_leaf = resolve_relative_global_gep_address(
          GlobalAddress{},
          global_aggregate_it->second.element_type_text,
          *gep,
          value_aliases,
          type_decls);
      if (!element_leaf.has_value() || element_leaf->value_type != bir::TypeKind::Ptr) {
        return false;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
          .global_name = global_aggregate_it->second.global_name,
          .byte_offset = global_aggregate_it->second.byte_offset + element_leaf->byte_offset,
          .element_count = global_aggregate_it->second.element_count,
          .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
          .index = global_aggregate_it->second.index,
      };
      return true;
    } else {
      const auto ptr_it = local_pointer_slots.find(gep->ptr.str());
      if (ptr_it == local_pointer_slots.end() || gep->indices.size() != 1) {
        return false;
      }
      const auto slot_it = local_slot_types.find(ptr_it->second);
      if (slot_it == local_slot_types.end()) {
        return false;
      }

      const auto parsed_index = parse_typed_operand(gep->indices.front());
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);

      const auto dot = ptr_it->second.rfind('.');
      if (dot == std::string::npos) {
        if (!index_imm.has_value() || *index_imm != 0) {
          return false;
        }
        resolved_slot = ptr_it->second;
      } else {
        const auto base_name = ptr_it->second.substr(0, dot);
        const auto base_array_it = local_array_slots.find(base_name);
        if (base_array_it == local_array_slots.end()) {
          return false;
        }
        const auto base_offset = parse_i64(std::string_view(ptr_it->second).substr(dot + 1));
        if (!base_offset.has_value()) {
          return false;
        }
        if (index_imm.has_value()) {
          const auto final_index = *base_offset + *index_imm;
          if (final_index < 0 ||
              static_cast<std::size_t>(final_index) >= base_array_it->second.element_slots.size()) {
            return false;
          }
          resolved_slot =
              base_array_it->second.element_slots[static_cast<std::size_t>(final_index)];
        } else {
          const auto parsed_index = parse_typed_operand(gep->indices.front());
          if (!parsed_index.has_value()) {
            return false;
          }
          const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
          if (!index_value.has_value() || *base_offset != 0 ||
              base_array_it->second.element_type != bir::TypeKind::Ptr) {
            return false;
          }
          dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
              .element_slots = base_array_it->second.element_slots,
              .index = *index_value,
          };
          return true;
        }
      }
    }

    local_pointer_slots[gep->result.str()] = std::move(resolved_slot);
    return true;
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    if (store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
        store->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
      return false;
    }

    const auto value_type = lower_integer_type(store->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    const auto value = lower_value(store->val, *value_type, value_aliases);
    if (!value.has_value()) {
      return false;
    }

    if (store->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = store->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        global_address_slots[global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_name,
          .value = *value,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(store->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != *value_type) {
          return false;
        }
        global_address_slots[global_object_it->second.global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
        lowered_insts->push_back(bir::StoreGlobalInst{
            .global_name = global_object_it->second.global_name,
            .value = *value,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(store->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (global_ptr_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        addressed_global_pointer_slots[make_global_pointer_slot_key(global_ptr_it->second)] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_ptr_it->second.global_name,
          .value = *value,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(store->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (tracks_pointer_value_slot) {
      if (*value_type == bir::TypeKind::Ptr) {
        const auto pointer_alias = resolve_local_aggregate_pointer_value_alias(
            store->val, value_aliases, function_symbols);
        if (pointer_alias.has_value()) {
          local_pointer_value_aliases[ptr_it->second] = *pointer_alias;
        } else {
          local_pointer_value_aliases.erase(ptr_it->second);
        }
      } else {
        local_pointer_value_aliases.erase(ptr_it->second);
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto global_addr_it = global_address_ints.find(store->val.str());
        if (global_addr_it != global_address_ints.end()) {
          local_address_slots[ptr_it->second] = global_addr_it->second;
          return true;
        }
      }
      local_address_slots.erase(ptr_it->second);
    } else if (*value_type == bir::TypeKind::Ptr) {
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = store->val.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end()) {
          if (!is_known_function_symbol(global_name, function_symbols)) {
            return false;
          }
          local_address_slots[ptr_it->second] = GlobalAddress{
              .global_name = global_name,
              .value_type = bir::TypeKind::Ptr,
              .byte_offset = 0,
          };
          return true;
        }
        if (!global_it->second.supports_linear_addressing) {
          return false;
        }
        local_address_slots[ptr_it->second] = GlobalAddress{
            .global_name = global_name,
            .value_type = global_it->second.value_type,
            .byte_offset = 0,
        };
        return true;
      }
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto global_ptr_it = global_pointer_slots.find(store->val.str());
        if (global_ptr_it != global_pointer_slots.end()) {
          local_address_slots[ptr_it->second] = global_ptr_it->second;
          return true;
        }
      }
      local_address_slots.erase(ptr_it->second);
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = ptr_it->second,
        .value = *value,
    });
    return true;
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    if (load->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         load->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    const auto value_type = lower_integer_type(load->type_str.str());
    if (!value_type.has_value()) {
      return false;
    }

    if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = load->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto runtime_it = global_address_slots.find(global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_name,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(load->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        if (const auto runtime_it = global_address_slots.find(global_object_it->second.global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_object_it->second.global_name,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(load->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (*value_type != bir::TypeKind::Ptr) {
        if (const auto honest_address = resolve_honest_addressed_global_access(
                global_ptr_it->second, *value_type, global_types);
            honest_address.has_value()) {
          lowered_insts->push_back(bir::LoadGlobalInst{
              .result = bir::Value::named(*value_type, load->result.str()),
              .global_name = honest_address->global_name,
              .byte_offset = honest_address->byte_offset,
          });
          return true;
        }
      }

      if (global_ptr_it->second.value_type != *value_type) {
        if (global_ptr_it->second.value_type != bir::TypeKind::Ptr ||
            global_ptr_it->second.byte_offset != 0) {
          return false;
        }
        const auto global_it = global_types.find(global_ptr_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_ptr_it->second.global_name,
        });
        return true;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        const auto addressed_it =
            addressed_global_pointer_slots.find(make_global_pointer_slot_key(global_ptr_it->second));
        if (addressed_it != addressed_global_pointer_slots.end()) {
          if (addressed_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *addressed_it->second;
          }
        } else {
          const auto global_it = global_types.find(global_ptr_it->second.global_name);
          if (global_it == global_types.end()) {
            return false;
          }
          if (global_ptr_it->second.byte_offset == 0 &&
              global_it->second.supports_direct_value &&
              global_it->second.value_type == bir::TypeKind::Ptr) {
            if (const auto runtime_it = global_address_slots.find(global_ptr_it->second.global_name);
                runtime_it != global_address_slots.end()) {
              if (runtime_it->second.has_value()) {
                global_pointer_slots[load->result.str()] = *runtime_it->second;
              }
            } else if (global_it->second.known_global_address.has_value()) {
              global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
              record_pointer_global_object_alias(
                  load->result.str(), global_it->second, global_types, global_object_pointer_slots);
            }
          }
          const auto pointer_init_it =
              global_it->second.pointer_initializer_offsets.find(global_ptr_it->second.byte_offset);
          if (pointer_init_it != global_it->second.pointer_initializer_offsets.end()) {
            global_pointer_slots[load->result.str()] = pointer_init_it->second;
          }
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_ptr_it->second.global_name,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(load->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto local_array_it = dynamic_local_pointer_arrays.find(load->ptr.str());
            local_array_it != dynamic_local_pointer_arrays.end()) {
          const auto element_values = collect_local_pointer_values(
              local_array_it->second.element_slots, local_pointer_value_aliases);
          if (!element_values.has_value()) {
            return false;
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, local_array_it->second.index, lowered_insts);
          if (!selected_value.has_value()) {
            return false;
          }
          if (selected_value->kind == bir::Value::Kind::Named &&
              selected_value->name == load->result.str()) {
            return true;
          }
          value_aliases[load->result.str()] = *selected_value;
          return true;
        }
        if (const auto global_array_it = dynamic_global_pointer_arrays.find(load->ptr.str());
            global_array_it != dynamic_global_pointer_arrays.end()) {
          const auto element_values =
              collect_global_array_pointer_values(global_array_it->second, global_types);
          if (!element_values.has_value()) {
            return false;
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, global_array_it->second.index, lowered_insts);
          if (!selected_value.has_value()) {
            return false;
          }
          if (selected_value->kind == bir::Value::Kind::Named &&
              selected_value->name == load->result.str()) {
            return true;
          }
          value_aliases[load->result.str()] = *selected_value;
          return true;
        }
      }
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (*value_type == bir::TypeKind::Ptr && tracks_pointer_value_slot) {
      const auto pointer_alias = local_pointer_value_aliases.find(ptr_it->second);
      if (pointer_alias != local_pointer_value_aliases.end()) {
        value_aliases[load->result.str()] = pointer_alias->second;
        if (const auto addr_it = local_address_slots.find(ptr_it->second);
            addr_it != local_address_slots.end()) {
          global_pointer_slots[load->result.str()] = addr_it->second;
        }
        return true;
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        global_address_ints[load->result.str()] = addr_it->second;
        return true;
      }
    } else if (*value_type == bir::TypeKind::Ptr) {
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        global_pointer_slots[load->result.str()] = addr_it->second;
        return true;
      }
    }

    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(*value_type, load->result.str()),
        .slot_name = ptr_it->second,
    });
    return true;
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    const auto return_type = lower_integer_type(call->return_type.str());
    if (!return_type.has_value()) {
      return false;
    }

    std::vector<bir::Value> lowered_args;
    std::vector<bir::TypeKind> lowered_arg_types;
    std::optional<std::string> callee_name;
    std::optional<bir::Value> callee_value;
    bool is_indirect_call = false;

    if (const auto parsed_call = parse_backend_direct_global_typed_call(*call);
        parsed_call.has_value()) {
      callee_name = std::string(parsed_call->symbol_name);
      lowered_args.reserve(parsed_call->typed_call.args.size());
      lowered_arg_types.reserve(parsed_call->typed_call.param_types.size());
      for (std::size_t index = 0; index < parsed_call->typed_call.args.size(); ++index) {
        const auto arg_type = lower_integer_type(parsed_call->typed_call.param_types[index]);
        if (!arg_type.has_value()) {
          return false;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(
                            std::string(parsed_call->typed_call.args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return false;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
      }
    } else if (const auto parsed_call = parse_backend_typed_call(*call);
               parsed_call.has_value() &&
               call->callee.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      callee_value = lower_value(call->callee, bir::TypeKind::Ptr, value_aliases);
      if (!callee_value.has_value()) {
        return false;
      }
      lowered_args.reserve(parsed_call->args.size());
      lowered_arg_types.reserve(parsed_call->param_types.size());
      for (std::size_t index = 0; index < parsed_call->args.size(); ++index) {
        const auto arg_type = lower_integer_type(parsed_call->param_types[index]);
        if (!arg_type.has_value()) {
          return false;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return false;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
      }
      is_indirect_call = true;
    } else if (const auto zero_arg_callee =
                   c4c::codegen::lir::parse_lir_direct_global_callee(call->callee);
               zero_arg_callee.has_value() &&
               c4c::codegen::lir::trim_lir_arg_text(call->args_str).empty()) {
      callee_name = std::string(*zero_arg_callee);
    } else {
      return false;
    }

    bir::CallInst lowered_call;
    if (*return_type != bir::TypeKind::Void) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }
      lowered_call.result = bir::Value::named(*return_type, call->result.str());
    } else if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }
    if (is_indirect_call) {
      lowered_call.callee_value = std::move(*callee_value);
    } else {
      lowered_call.callee = std::move(*callee_name);
    }
    lowered_call.args = std::move(lowered_args);
    lowered_call.arg_types = std::move(lowered_arg_types);
    lowered_call.return_type_name = std::string(call->return_type.str());
    lowered_call.return_type = *return_type;
    lowered_call.is_indirect = is_indirect_call;
    lowered_insts->push_back(std::move(lowered_call));
    return true;
  }

  return false;
}

bool canonicalize_compare_return_alias(const c4c::codegen::lir::LirOperand& ret_value,
                                       const bir::Value& lowered_value,
                                       bir::TypeKind return_type,
                                       const CompareMap& compare_exprs,
                                       std::vector<bir::Inst>* lowered_insts,
                                       bir::ReturnTerminator* lowered_ret) {
  if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      lowered_value.kind != bir::Value::Kind::Named ||
      lowered_value.type != bir::TypeKind::I1 ||
      return_type != bir::TypeKind::I32) {
    return false;
  }

  const auto compare_it = compare_exprs.find(ret_value.str());
  if (compare_it == compare_exprs.end()) {
    return false;
  }

  for (auto inst_it = lowered_insts->rbegin(); inst_it != lowered_insts->rend(); ++inst_it) {
    auto* binary = std::get_if<bir::BinaryInst>(&*inst_it);
    if (binary == nullptr) {
      continue;
    }
    if (binary->result.name != lowered_value.name) {
      continue;
    }

    binary->result.name = ret_value.str();
    lowered_ret->value = bir::Value::named(return_type, ret_value.str());
    return true;
  }

  return false;
}

BlockLookup make_block_lookup(const c4c::codegen::lir::LirFunction& function) {
  BlockLookup blocks;
  for (const auto& block : function.blocks) {
    blocks.emplace(block.label, &block);
  }
  return blocks;
}

std::optional<BranchChain> follow_empty_branch_chain(const BlockLookup& blocks,
                                                     const std::string& start_label) {
  std::unordered_set<std::string> seen;
  const auto* current = [&]() -> const c4c::codegen::lir::LirBlock* {
    const auto it = blocks.find(start_label);
    return it == blocks.end() ? nullptr : it->second;
  }();
  if (current == nullptr) {
    return std::nullopt;
  }

  BranchChain chain;
  while (current != nullptr) {
    if (!seen.emplace(current->label).second || !current->insts.empty()) {
      return std::nullopt;
    }

    const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&current->terminator);
    if (br == nullptr) {
      return std::nullopt;
    }

    chain.labels.push_back(current->label);
    const auto next_it = blocks.find(br->target_label);
    if (next_it == blocks.end()) {
      return std::nullopt;
    }

    const auto* next = next_it->second;
    if (!next->insts.empty() ||
        !std::holds_alternative<c4c::codegen::lir::LirBr>(next->terminator)) {
      chain.leaf_label = current->label;
      chain.join_label = br->target_label;
      return chain;
    }

    current = next;
  }

  return std::nullopt;
}

std::optional<bir::Function> lower_select_family_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_type = infer_function_return_type(function);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* cond_br = std::get_if<c4c::codegen::lir::LirCondBr>(&entry.terminator);
  if (cond_br == nullptr) {
    return std::nullopt;
  }

  ValueMap value_aliases;
  CompareMap compare_exprs;
  for (const auto& inst : entry.insts) {
    if (!lower_scalar_compare_inst(inst, value_aliases, compare_exprs, nullptr)) {
      return std::nullopt;
    }
  }

  const auto compare_it = compare_exprs.find(cond_br->cond_name);
  if (compare_it == compare_exprs.end()) {
    return std::nullopt;
  }

  const auto blocks = make_block_lookup(function);
  const auto true_chain = follow_empty_branch_chain(blocks, cond_br->true_label);
  const auto false_chain = follow_empty_branch_chain(blocks, cond_br->false_label);
  if (!true_chain.has_value() || !false_chain.has_value() ||
      true_chain->join_label != false_chain->join_label) {
    return std::nullopt;
  }

  const auto join_it = blocks.find(true_chain->join_label);
  if (join_it == blocks.end()) {
    return std::nullopt;
  }
  const auto& join_block = *join_it->second;

  if (join_block.insts.empty() || join_block.insts.size() > 2) {
    return std::nullopt;
  }
  const auto* phi = std::get_if<c4c::codegen::lir::LirPhiOp>(&join_block.insts.front());
  if (phi == nullptr || phi->incoming.size() != 2) {
    return std::nullopt;
  }
  const auto phi_type = lower_integer_type(phi->type_str.str());
  if (!phi_type.has_value()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&join_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto ret_value = c4c::codegen::lir::LirOperand(*ret->value_str);

  std::optional<c4c::codegen::lir::LirOperand> true_incoming;
  std::optional<c4c::codegen::lir::LirOperand> false_incoming;
  for (const auto& [value, label] : phi->incoming) {
    if (label == true_chain->leaf_label) {
      true_incoming = c4c::codegen::lir::LirOperand(value);
    } else if (label == false_chain->leaf_label) {
      false_incoming = c4c::codegen::lir::LirOperand(value);
    }
  }
  if (!true_incoming.has_value() || !false_incoming.has_value()) {
    return std::nullopt;
  }

  std::unordered_set<std::string> covered_labels;
  covered_labels.insert(entry.label);
  covered_labels.insert(join_block.label);
  covered_labels.insert(true_chain->labels.begin(), true_chain->labels.end());
  covered_labels.insert(false_chain->labels.begin(), false_chain->labels.end());
  if (covered_labels.size() != function.blocks.size()) {
    return std::nullopt;
  }

  const auto true_value = lower_value(*true_incoming, *phi_type, value_aliases);
  const auto false_value = lower_value(*false_incoming, *phi_type, value_aliases);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = *return_type;
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }

  bir::Block lowered_block;
  lowered_block.label = entry.label;
  lowered_block.insts.push_back(bir::SelectInst{
      .predicate = compare_it->second.opcode,
      .result = bir::Value::named(*phi_type, phi->result.str()),
      .compare_type = compare_it->second.operand_type,
      .lhs = compare_it->second.lhs,
      .rhs = compare_it->second.rhs,
      .true_value = *true_value,
      .false_value = *false_value,
  });

  if (join_block.insts.size() == 1) {
    if (ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        ret_value.str() != phi->result.str()) {
      return std::nullopt;
    }
    lowered_block.terminator = bir::ReturnTerminator{
        .value = bir::Value::named(*phi_type, phi->result.str()),
    };
    lowered.blocks.push_back(std::move(lowered_block));
    return lowered;
  }

  const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&join_block.insts[1]);
  if (call == nullptr || *phi_type != bir::TypeKind::Ptr ||
      call->callee.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      call->callee.str() != phi->result.str()) {
    return std::nullopt;
  }

  const auto parsed_call = parse_backend_typed_call(*call);
  const auto call_return_type = lower_integer_type(call->return_type.str());
  if (!parsed_call.has_value() || !call_return_type.has_value() || *call_return_type != *return_type ||
      parsed_call->args.size() != parsed_call->param_types.size() ||
      call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      ret_value.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
      ret_value.str() != call->result.str()) {
    return std::nullopt;
  }

  bir::CallInst lowered_call;
  lowered_call.result = bir::Value::named(*call_return_type, call->result.str());
  lowered_call.callee_value = bir::Value::named(*phi_type, phi->result.str());
  lowered_call.return_type_name = std::string(call->return_type.str());
  lowered_call.return_type = *call_return_type;
  lowered_call.is_indirect = true;
  lowered_call.args.reserve(parsed_call->args.size());
  lowered_call.arg_types.reserve(parsed_call->param_types.size());
  for (std::size_t index = 0; index < parsed_call->args.size(); ++index) {
    const auto arg_type = lower_integer_type(parsed_call->param_types[index]);
    if (!arg_type.has_value()) {
      return std::nullopt;
    }
    const auto arg =
        lower_value(c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
                    *arg_type,
                    value_aliases);
    if (!arg.has_value()) {
      return std::nullopt;
    }
    lowered_call.arg_types.push_back(*arg_type);
    lowered_call.args.push_back(*arg);
  }
  lowered_block.insts.push_back(std::move(lowered_call));
  lowered_block.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(*call_return_type, call->result.str()),
  };
  lowered.blocks.push_back(std::move(lowered_block));
  return lowered;
}

std::optional<bir::Function> lower_branch_family_function(
    BirLoweringContext& context,
    const c4c::codegen::lir::LirFunction& function,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    const TypeDeclMap& type_decls) {
  (void)context;
  if (function.is_declaration || function.blocks.empty()) {
    return std::nullopt;
  }

  const auto return_type = infer_function_return_type(function);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = function.name;
  lowered.return_type = *return_type;
  if (!lower_function_params(function, &lowered)) {
    return std::nullopt;
  }

  ValueMap value_aliases;
  CompareMap compare_exprs;
  LocalSlotTypes local_slot_types;
  LocalPointerSlots local_pointer_slots;
  LocalArraySlotMap local_array_slots;
  LocalPointerArrayBaseMap local_pointer_array_bases;
  DynamicLocalPointerArrayMap dynamic_local_pointer_arrays;
  LocalAggregateSlotMap local_aggregate_slots;
  LocalAggregateFieldSet local_aggregate_field_slots;
  LocalPointerValueAliasMap local_pointer_value_aliases;
  LocalAddressSlots local_address_slots;
  GlobalAddressSlots global_address_slots;
  AddressedGlobalPointerSlots addressed_global_pointer_slots;
  GlobalPointerMap global_pointer_slots;
  DynamicGlobalPointerArrayMap dynamic_global_pointer_arrays;
  DynamicGlobalAggregateArrayMap dynamic_global_aggregate_arrays;
  GlobalObjectPointerMap global_object_pointer_slots;
  GlobalAddressIntMap global_address_ints;
  GlobalObjectAddressIntMap global_object_address_ints;
  std::vector<bir::Inst> hoisted_alloca_scratch;

  for (const auto& inst : function.alloca_insts) {
    if (!lower_scalar_or_local_memory_inst(
            inst,
            value_aliases,
            compare_exprs,
            local_slot_types,
            local_pointer_slots,
            local_array_slots,
            local_pointer_array_bases,
            dynamic_local_pointer_arrays,
            local_aggregate_slots,
            local_aggregate_field_slots,
            local_pointer_value_aliases,
            local_address_slots,
            global_address_slots,
            addressed_global_pointer_slots,
            global_pointer_slots,
            dynamic_global_pointer_arrays,
            dynamic_global_aggregate_arrays,
            global_object_pointer_slots,
            global_address_ints,
            global_object_address_ints,
            global_types,
            function_symbols,
            type_decls,
            &lowered,
            &hoisted_alloca_scratch)) {
      return std::nullopt;
    }
  }
  if (!hoisted_alloca_scratch.empty()) {
    return std::nullopt;
  }

  for (const auto& block : function.blocks) {
    bir::Block lowered_block;
    lowered_block.label = block.label;

    for (const auto& inst : block.insts) {
      if (!lower_scalar_or_local_memory_inst(
              inst,
              value_aliases,
              compare_exprs,
              local_slot_types,
              local_pointer_slots,
              local_array_slots,
              local_pointer_array_bases,
              dynamic_local_pointer_arrays,
              local_aggregate_slots,
              local_aggregate_field_slots,
              local_pointer_value_aliases,
              local_address_slots,
              global_address_slots,
              addressed_global_pointer_slots,
              global_pointer_slots,
              dynamic_global_pointer_arrays,
              dynamic_global_aggregate_arrays,
              global_object_pointer_slots,
              global_address_ints,
              global_object_address_ints,
              global_types,
              function_symbols,
              type_decls,
              &lowered,
              &lowered_block.insts)) {
        return std::nullopt;
      }
    }

    if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator)) {
      bir::ReturnTerminator lowered_ret;
      if (ret->value_str.has_value()) {
        const c4c::codegen::lir::LirOperand ret_value(*ret->value_str);
        const auto value = lower_value(ret_value, *return_type, value_aliases);
        if (!value.has_value()) {
          return std::nullopt;
        }
        if (!canonicalize_compare_return_alias(
                ret_value, *value, *return_type, compare_exprs, &lowered_block.insts, &lowered_ret)) {
          lowered_ret.value = *value;
        }
      }
      lowered_block.terminator = lowered_ret;
    } else if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&block.terminator)) {
      lowered_block.terminator = bir::BranchTerminator{.target_label = br->target_label};
    } else if (const auto* cond_br =
                   std::get_if<c4c::codegen::lir::LirCondBr>(&block.terminator)) {
      const auto condition = lower_value(cond_br->cond_name, bir::TypeKind::I1, value_aliases);
      if (!condition.has_value()) {
        return std::nullopt;
      }
      lowered_block.terminator = bir::CondBranchTerminator{
          .condition = *condition,
          .true_label = cond_br->true_label,
          .false_label = cond_br->false_label,
      };
    } else {
      return std::nullopt;
    }

    lowered.blocks.push_back(std::move(lowered_block));
  }

  return lowered;
}

}  // namespace

std::optional<bir::Module> lower_module(BirLoweringContext& context,
                                        const BirModuleAnalysis& analysis) {
  bir::Module module;
  module.target_triple = context.lir_module.target_triple;
  module.data_layout = context.lir_module.data_layout;

  if (analysis.function_count == 0 && analysis.global_count == 0 &&
      analysis.string_constant_count == 0 && analysis.extern_decl_count == 0) {
    context.note("module", "lowered empty module shell to BIR");
    return module;
  }

  if (analysis.global_count != 0 || analysis.string_constant_count != 0 ||
      analysis.extern_decl_count != 0) {
    context.note(
        "module",
        "bootstrap lir_to_bir only supports minimal scalar globals, linear integer-array globals, aggregate-backed globals with honest byte-address semantics, string-backed byte data, and extern integer-array globals right now");
  }

  GlobalTypes global_types;
  FunctionSymbolSet function_symbols;
  global_types.reserve(context.lir_module.globals.size() + context.lir_module.string_pool.size());
  function_symbols.reserve(context.lir_module.extern_decls.size() +
                           context.lir_module.functions.size());
  for (const auto& decl : context.lir_module.extern_decls) {
    function_symbols.insert(decl.name);
  }
  for (const auto& function : context.lir_module.functions) {
    function_symbols.insert(function.name);
  }
  const auto type_decls = build_type_decl_map(context.lir_module.type_decls);
  for (const auto& global : context.lir_module.globals) {
    GlobalInfo info;
    auto lowered_global = lower_minimal_global(global, type_decls, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports minimal scalar globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics right now");
      return std::nullopt;
    }
    global_types.emplace(lowered_global->name, info);
    module.globals.push_back(std::move(*lowered_global));
  }

  for (const auto& string_constant : context.lir_module.string_pool) {
    GlobalInfo info;
    auto lowered_global = lower_string_constant_global(string_constant, &info);
    if (!lowered_global.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports byte-addressable string-pool constants right now");
      return std::nullopt;
    }
    global_types.emplace(lowered_global->name, info);
    module.globals.push_back(std::move(*lowered_global));
  }

  if (!resolve_pointer_initializer_offsets(global_types, function_symbols)) {
    context.note(
        "module",
        "bootstrap lir_to_bir only supports aggregate pointer fields initialized from addressable globals right now");
    return std::nullopt;
  }

  std::unordered_set<std::string> resolving_global_addresses;
  for (auto& [global_name, info] : global_types) {
    if (info.initializer_symbol_name.empty()) {
      continue;
    }
    if (!resolve_known_global_address(
             global_name, global_types, function_symbols, &resolving_global_addresses)
             .has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports pointer globals initialized from addressable globals or pointer-global aliases that resolve to in-bounds global data right now");
      return std::nullopt;
    }
  }

  for (const auto& decl : context.lir_module.extern_decls) {
    auto lowered_decl = lower_extern_decl(decl);
    if (!lowered_decl.has_value()) {
      continue;
    }
    module.functions.push_back(std::move(*lowered_decl));
  }

  for (const auto& function : context.lir_module.functions) {
    if (function.is_declaration) {
      auto lowered_decl = lower_decl_function(function);
      if (!lowered_decl.has_value()) {
        continue;
      }
      module.functions.push_back(std::move(*lowered_decl));
      continue;
    }

    auto lowered_function = lower_select_family_function(context, function);
    if (!lowered_function.has_value()) {
      lowered_function = lower_branch_family_function(
          context, function, global_types, function_symbols, type_decls);
    }
    if (!lowered_function.has_value()) {
      context.note(
          "module",
          "bootstrap lir_to_bir only supports scalar compare/branch/select/return functions right now");
      return std::nullopt;
    }
    module.functions.push_back(std::move(*lowered_function));
  }

  context.note(
      "module",
      "lowered scalar compare/select/local-memory/global/call/return module to BIR");
  return module;
}

}  // namespace c4c::backend
