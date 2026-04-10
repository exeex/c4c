#include "x86_codegen.hpp"

#include "../../bir.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

std::string symbol_name_for_target(std::string_view target_triple, std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string::npos) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name) {
  std::ostringstream out;
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @function\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool zero_initializer) {
  std::ostringstream out;
  out << (zero_initializer ? ".bss\n" : ".data\n");
  out << ".globl " << symbol_name << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".type " << symbol_name << ", @object\n";
  }
  if (align_bytes > 1) {
    out << ".p2align " << (align_bytes == 2 ? 1 : 2) << "\n";
  }
  out << symbol_name << ":\n";
  return out.str();
}

}  // namespace

struct MinimalGlobalTwoFieldStructStoreSubSubSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t field0_init_imm = 0;
  std::int64_t field1_init_imm = 0;
  std::int64_t field0_store_imm = 0;
  std::int64_t field1_store_imm = 0;
  std::size_t align_bytes = 4;
};

struct MinimalScalarGlobalStoreReloadSlice {
  std::string function_name;
  std::string global_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  std::size_t align_bytes = 4;
};

std::string emit_minimal_scalar_global_load_slice_asm(std::string_view target_triple,
                                                      std::string_view function_name,
                                                      std::string_view global_name,
                                                      std::int64_t init_imm,
                                                      std::size_t align_bytes,
                                                      bool zero_initializer) {
  if (init_imm < std::numeric_limits<std::int32_t>::min() ||
      init_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, align_bytes, zero_initializer);
  if (zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << init_imm << "\n";
  }
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_extern_scalar_global_load_slice_asm(std::string_view target_triple,
                                                             std::string_view function_name,
                                                             std::string_view global_name) {
  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".extern " << global_symbol << "\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_scalar_global_store_reload_slice_asm(std::string_view target_triple,
                                                              std::string_view function_name,
                                                              std::string_view global_name,
                                                              std::int64_t init_imm,
                                                              std::int64_t store_imm,
                                                              std::size_t align_bytes) {
  if (init_imm < std::numeric_limits<std::int32_t>::min() ||
      init_imm > std::numeric_limits<std::int32_t>::max() ||
      store_imm < std::numeric_limits<std::int32_t>::min() ||
      store_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, align_bytes, false)
      << "  .long " << init_imm << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << store_imm << "\n"
      << "  mov eax, dword ptr [rax]\n"
      << "  ret\n";
  return out.str();
}

std::string emit_minimal_global_store_return_and_entry_return_asm(
    std::string_view target_triple,
    const MinimalGlobalStoreReturnAndEntryReturnSlice& slice) {
  if (slice.init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.init_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.store_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.store_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.helper_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.helper_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.entry_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.entry_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, slice.global_name);
  const auto helper_symbol = symbol_name_for_target(target_triple, slice.helper_name);
  const auto entry_symbol = symbol_name_for_target(target_triple, slice.entry_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, slice.align_bytes,
                                    slice.zero_initializer);
  if (slice.zero_initializer) {
    out << "  .zero 4\n";
  } else {
    out << "  .long " << slice.init_imm << "\n";
  }
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 4\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, helper_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.store_imm << "\n"
      << "  mov eax, " << slice.helper_imm << "\n"
      << "  ret\n"
      << emit_function_prelude(target_triple, entry_symbol)
      << "  mov eax, " << slice.entry_imm << "\n"
      << "  ret\n";
  return out.str();
}

std::optional<MinimalScalarGlobalStoreReloadSlice> parse_minimal_scalar_global_store_reload_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* store =
      entry.insts.size() == 2 ? std::get_if<StoreGlobalInst>(&entry.insts.front()) : nullptr;
  const auto* load =
      entry.insts.size() == 2 ? std::get_if<LoadGlobalInst>(&entry.insts.back()) : nullptr;
  if (entry.label != "entry" || store == nullptr || load == nullptr ||
      store->global_name != global.name || store->byte_offset != 0 ||
      store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store->value.type != TypeKind::I32 ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load->result.type != TypeKind::I32 ||
      load->global_name != global.name || load->byte_offset != 0 ||
      entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() || *entry.terminator.value != load->result) {
    return std::nullopt;
  }

  return MinimalScalarGlobalStoreReloadSlice{
      .function_name = function.name,
      .global_name = global.name,
      .init_imm = global.initializer->immediate,
      .store_imm = store->value.immediate,
      .align_bytes = load->align_bytes > 0 ? load->align_bytes : 4,
  };
}

std::optional<MinimalGlobalStoreReturnAndEntryReturnSlice>
parse_minimal_global_store_return_and_entry_return_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 2 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32) {
    return std::nullopt;
  }

  auto try_match = [&](const Function& helper,
                       const Function& entry)
      -> std::optional<MinimalGlobalStoreReturnAndEntryReturnSlice> {
    if (helper.is_declaration || entry.is_declaration || helper.return_type != TypeKind::I32 ||
        entry.return_type != TypeKind::I32 || !helper.params.empty() || !entry.params.empty() ||
        !helper.local_slots.empty() || !entry.local_slots.empty() ||
        helper.blocks.size() != 1 || entry.blocks.size() != 1) {
      return std::nullopt;
    }

    const auto& helper_entry = helper.blocks.front();
    const auto& entry_entry = entry.blocks.front();
    const auto* store = helper_entry.insts.size() == 1
                            ? std::get_if<StoreGlobalInst>(&helper_entry.insts.front())
                            : nullptr;
    if (helper_entry.label != "entry" || store == nullptr ||
        store->global_name != global.name || store->byte_offset != 0 ||
        store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        store->value.type != TypeKind::I32 ||
        helper_entry.terminator.kind != TerminatorKind::Return ||
        !helper_entry.terminator.value.has_value() ||
        helper_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
        helper_entry.terminator.value->type != TypeKind::I32 ||
        entry_entry.label != "entry" || !entry_entry.insts.empty() ||
        entry_entry.terminator.kind != TerminatorKind::Return ||
        !entry_entry.terminator.value.has_value() ||
        entry_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
        entry_entry.terminator.value->type != TypeKind::I32) {
      return std::nullopt;
    }

    return MinimalGlobalStoreReturnAndEntryReturnSlice{
        .global_name = global.name,
        .helper_name = helper.name,
        .entry_name = entry.name,
        .init_imm = global.initializer->immediate,
        .store_imm = store->value.immediate,
        .helper_imm = helper_entry.terminator.value->immediate,
        .entry_imm = entry_entry.terminator.value->immediate,
        .align_bytes = store->align_bytes > 0 ? store->align_bytes : 4,
        .zero_initializer = global.initializer->immediate == 0,
    };
  };

  if (const auto parsed = try_match(module.functions.front(), module.functions.back());
      parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions.back(), module.functions.front());
}

std::optional<MinimalGlobalTwoFieldStructStoreSubSubSlice>
parse_minimal_global_two_field_struct_store_sub_sub_slice(
    const c4c::backend::bir::Module& module) {
  using namespace c4c::backend::bir;

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_extern || global.type != TypeKind::I32 || !global.initializer.has_value() ||
      global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate ||
      global.initializer->type != TypeKind::I32 || global.size_bytes != 8) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* store_field0 =
      entry.insts.size() == 6 ? std::get_if<StoreGlobalInst>(&entry.insts[0]) : nullptr;
  const auto* store_field1 =
      entry.insts.size() == 6 ? std::get_if<StoreGlobalInst>(&entry.insts[1]) : nullptr;
  const auto* load_field0 =
      entry.insts.size() == 6 ? std::get_if<LoadGlobalInst>(&entry.insts[2]) : nullptr;
  const auto* first_sub =
      entry.insts.size() == 6 ? std::get_if<BinaryInst>(&entry.insts[3]) : nullptr;
  const auto* load_field1 =
      entry.insts.size() == 6 ? std::get_if<LoadGlobalInst>(&entry.insts[4]) : nullptr;
  const auto* second_sub =
      entry.insts.size() == 6 ? std::get_if<BinaryInst>(&entry.insts[5]) : nullptr;
  if (entry.label != "entry" || store_field0 == nullptr || store_field1 == nullptr ||
      load_field0 == nullptr || first_sub == nullptr || load_field1 == nullptr ||
      second_sub == nullptr || entry.terminator.kind != TerminatorKind::Return ||
      !entry.terminator.value.has_value() ||
      entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      *entry.terminator.value != second_sub->result ||
      store_field0->global_name != global.name || store_field0->byte_offset != 0 ||
      store_field0->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_field0->value.type != TypeKind::I32 ||
      store_field1->global_name != global.name || store_field1->byte_offset != 4 ||
      store_field1->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_field1->value.type != TypeKind::I32 ||
      load_field0->global_name != global.name || load_field0->byte_offset != 0 ||
      load_field0->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load_field0->result.type != TypeKind::I32 ||
      first_sub->opcode != BinaryOpcode::Sub ||
      first_sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      first_sub->result.type != TypeKind::I32 ||
      first_sub->lhs != c4c::backend::bir::Value::immediate_i32(3) ||
      first_sub->rhs != load_field0->result || load_field1->global_name != global.name ||
      load_field1->byte_offset != 4 ||
      load_field1->result.kind != c4c::backend::bir::Value::Kind::Named ||
      load_field1->result.type != TypeKind::I32 ||
      second_sub->opcode != BinaryOpcode::Sub ||
      second_sub->result.kind != c4c::backend::bir::Value::Kind::Named ||
      second_sub->result.type != TypeKind::I32 ||
      second_sub->lhs != first_sub->result || second_sub->rhs != load_field1->result) {
    return std::nullopt;
  }

  return MinimalGlobalTwoFieldStructStoreSubSubSlice{
      .function_name = function.name,
      .global_name = global.name,
      .field0_init_imm = global.initializer->immediate,
      .field1_init_imm = global.initializer->immediate,
      .field0_store_imm = store_field0->value.immediate,
      .field1_store_imm = store_field1->value.immediate,
      .align_bytes = store_field0->align_bytes > 0 ? store_field0->align_bytes : 4,
  };
}

std::string emit_minimal_global_two_field_struct_store_sub_sub_asm(
    std::string_view target_triple,
    const MinimalGlobalTwoFieldStructStoreSubSubSlice& slice) {
  if (slice.field0_init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.field0_init_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.field1_init_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.field1_init_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.field0_store_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.field0_store_imm > std::numeric_limits<std::int32_t>::max() ||
      slice.field1_store_imm < std::numeric_limits<std::int32_t>::min() ||
      slice.field1_store_imm > std::numeric_limits<std::int32_t>::max()) {
    throw std::invalid_argument(
        "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
  }

  const auto global_symbol = symbol_name_for_target(target_triple, slice.global_name);
  const auto function_symbol = symbol_name_for_target(target_triple, slice.function_name);
  std::ostringstream out;
  out << ".intel_syntax noprefix\n"
      << emit_global_symbol_prelude(target_triple, global_symbol, slice.align_bytes, false)
      << "  .long " << slice.field0_init_imm << "\n"
      << "  .long " << slice.field1_init_imm << "\n";
  if (target_triple.find("apple-darwin") == std::string::npos) {
    out << ".size " << global_symbol << ", 8\n";
  }
  out << ".text\n"
      << emit_function_prelude(target_triple, function_symbol)
      << "  lea rax, " << global_symbol << "[rip]\n"
      << "  mov dword ptr [rax], " << slice.field0_store_imm << "\n"
      << "  mov dword ptr [rax + 4], " << slice.field1_store_imm << "\n"
      << "  mov ecx, 3\n"
      << "  sub ecx, dword ptr [rax]\n"
      << "  sub ecx, dword ptr [rax + 4]\n"
      << "  mov eax, ecx\n"
      << "  ret\n";
  return out.str();
}

std::optional<std::string> try_emit_minimal_global_two_field_struct_store_sub_sub_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_global_two_field_struct_store_sub_sub_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_global_two_field_struct_store_sub_sub_asm(module.target_triple, *slice);
}

std::optional<std::string> try_emit_minimal_scalar_global_store_reload_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_scalar_global_store_reload_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_scalar_global_store_reload_slice_asm(module.target_triple,
                                                           slice->function_name,
                                                           slice->global_name,
                                                           slice->init_imm,
                                                           slice->store_imm,
                                                           slice->align_bytes);
}

std::optional<std::string> try_emit_minimal_global_store_return_and_entry_return_module(
    const c4c::backend::bir::Module& module) {
  const auto slice = parse_minimal_global_store_return_and_entry_return_slice(module);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  return emit_minimal_global_store_return_and_entry_return_asm(module.target_triple, *slice);
}

}  // namespace c4c::backend::x86
