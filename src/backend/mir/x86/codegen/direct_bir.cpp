#include "x86_codegen.hpp"

#include "../../../bir/bir.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

struct MinimalDirectReturnSlice {
  std::string function_name;
  c4c::backend::bir::TypeKind return_type = c4c::backend::bir::TypeKind::Void;
  std::optional<std::int64_t> immediate = std::nullopt;
};

std::string local_symbol_name(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string_view::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string local_function_prelude(std::string_view target_triple, std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string_view::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::optional<MinimalDirectReturnSlice> parse_minimal_direct_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (!module.globals.empty() || !module.string_constants.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !function.params.empty() || !function.local_slots.empty() ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (!entry.insts.empty() || entry.terminator.kind != TerminatorKind::Return) {
    return std::nullopt;
  }

  if (!entry.terminator.value.has_value()) {
    if (function.return_type != TypeKind::Void) {
      return std::nullopt;
    }
    return MinimalDirectReturnSlice{
        .function_name = function.name,
        .return_type = TypeKind::Void,
    };
  }

  const auto& returned = *entry.terminator.value;
  if (returned.kind != c4c::backend::bir::Value::Kind::Immediate ||
      returned.type != function.return_type) {
    return std::nullopt;
  }

  switch (function.return_type) {
    case TypeKind::I8:
    case TypeKind::I32:
    case TypeKind::I64:
      return MinimalDirectReturnSlice{
          .function_name = function.name,
          .return_type = function.return_type,
          .immediate = returned.immediate,
      };
    default:
      return std::nullopt;
  }
}

std::string emit_minimal_direct_return_asm(std::string_view target_triple,
                                           const MinimalDirectReturnSlice& slice) {
  std::ostringstream out;
  const auto symbol_name = local_symbol_name(target_triple, slice.function_name);
  out << local_function_prelude(target_triple, symbol_name);
  if (slice.immediate.has_value()) {
    switch (slice.return_type) {
      case c4c::backend::bir::TypeKind::I64:
        out << "    movq $" << *slice.immediate << ", %rax\n";
        break;
      case c4c::backend::bir::TypeKind::I8:
      case c4c::backend::bir::TypeKind::I32:
        out << "    movl $" << *slice.immediate << ", %eax\n";
        break;
      default:
        break;
    }
  }
  out << "    ret\n";
  return out.str();
}

}  // namespace

std::string emit_module(const c4c::backend::bir::Module& module) {
  if (const auto slice = parse_minimal_direct_return_slice(module); slice.has_value()) {
    return emit_minimal_direct_return_asm(module.target_triple, *slice);
  }
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the simple direct-return subset lowers natively");
}

}  // namespace c4c::backend::x86
