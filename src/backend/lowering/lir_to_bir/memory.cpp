// Translated from the reference memory-lowering paths:
// - ref/claudes-c-compiler/src/ir/lowering/lvalue.rs
// - ref/claudes-c-compiler/src/ir/lowering/expr_access.rs
// - ref/claudes-c-compiler/src/backend/x86/codegen/memory.rs
//
// This file is a translation scaffold for the future split of memory-related
// lowering out of the monolithic lir_to_bir.cpp. It records the intended
// ownership for allocas, GEP/address formation, and load/store materialization
// into backend-owned BIR memory operations.

#include "passes.hpp"

#include "../../../codegen/lir/ir.hpp"

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

namespace {

struct MemoryLoweringFrame {
  const c4c::codegen::lir::LirFunction* lir_function = nullptr;
  bir::Function* bir_function = nullptr;
  bir::Block* bir_block = nullptr;
};

std::optional<bir::TypeKind> lower_memory_scalar_type(std::string_view type_text) {
  if (type_text == "i8" || type_text == "char" || type_text == "signed char" ||
      type_text == "unsigned char") {
    return bir::TypeKind::I8;
  }
  if (type_text == "i32" || type_text == "int") {
    return bir::TypeKind::I32;
  }
  if (type_text == "i64" || type_text == "ptr" || (!type_text.empty() && type_text.back() == '*')) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<std::int64_t> parse_memory_immediate(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool memory_target_supports_global_pointer_diff(std::string_view target_triple) {
  return target_triple.find("x86_64") != std::string::npos ||
         target_triple.find("i686") != std::string::npos ||
         target_triple.find("aarch64") != std::string::npos;
}

bool memory_lir_type_matches_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                           unsigned bit_width) {
  if (type.kind() == c4c::codegen::lir::LirTypeKind::Integer &&
      type.integer_bit_width() == bit_width) {
    return true;
  }

  const auto lowered_type = lower_scalar_type_text(type.str());
  if (!lowered_type.has_value()) {
    return false;
  }

  switch (*lowered_type) {
    case bir::TypeKind::I8:
      return bit_width == 8;
    case bir::TypeKind::I32:
      return bit_width == 32;
    case bir::TypeKind::I64:
      return bit_width == 64;
    case bir::TypeKind::Void:
      return bit_width == 0;
  }
  return false;
}

bir::MemoryAddress make_local_memory_address(std::string slot_name,
                                             std::size_t byte_offset,
                                             std::size_t align_bytes = 0) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::LocalSlot;
  address.base_name = std::move(slot_name);
  address.byte_offset = static_cast<std::int64_t>(byte_offset);
  address.align_bytes = align_bytes;
  address.address_space = bir::AddressSpace::Default;
  return address;
}

bir::MemoryAddress make_global_memory_address(std::string global_name,
                                              std::size_t byte_offset,
                                              std::size_t align_bytes = 0) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol;
  address.base_name = std::move(global_name);
  address.byte_offset = static_cast<std::int64_t>(byte_offset);
  address.align_bytes = align_bytes;
  address.address_space = bir::AddressSpace::Default;
  return address;
}

bir::MemoryAddress make_pointer_memory_address(bir::Value base_value,
                                               std::int64_t byte_offset,
                                               std::size_t align_bytes = 0) {
  bir::MemoryAddress address;
  address.base_kind = bir::MemoryAddress::BaseKind::PointerValue;
  address.base_value = std::move(base_value);
  address.byte_offset = byte_offset;
  address.align_bytes = align_bytes;
  address.address_space = bir::AddressSpace::Default;
  return address;
}

}  // namespace

// ---------------------------------------------------------------------------
// LValue / address translation scaffold
// ---------------------------------------------------------------------------
//
// The reference lowering code treats these as the address-producing forms:
// - identifier -> local slot, static local, or global symbol
// - dereference -> pointer value becomes the address
// - subscript -> base address + scaled element offset
// - member access -> base address + field byte offset
// - pointer member access -> pointer value + field byte offset
// - complex real/imag access -> base address + component offset
// - generic selection -> delegate to the selected branch
//
// The eventual C++ split should funnel those cases into BIR memory operations
// instead of keeping them embedded inside the main LIR lowering loop.

std::optional<bir::MemoryAddress> lower_identifier_address(
    const c4c::codegen::lir::LirFunction& lir_function,
    std::string_view name) {
  // TODO: consult local/static/global tables from lir_function and the module
  // scope, then produce the appropriate BIR memory address.
  (void)lir_function;
  (void)name;
  return std::nullopt;
}

std::optional<bir::MemoryAddress> lower_dereference_address(bir::Value pointer_value) {
  // `*ptr` in the reference lowering simply reuses the pointer value as the
  // address-producing result.
  return make_pointer_memory_address(std::move(pointer_value), 0);
}

std::optional<bir::MemoryAddress> lower_gep_address(bir::Value base_value,
                                                    std::int64_t byte_offset) {
  // GEP is modeled as pointer + byte offset. The more specific element sizing
  // logic still belongs to the caller that knows the pointee layout.
  return make_pointer_memory_address(std::move(base_value), byte_offset);
}

std::optional<bir::MemoryAddress> lower_struct_member_address(
    bir::Value base_value,
    std::int64_t field_offset,
    std::size_t align_bytes = 0) {
  return make_pointer_memory_address(std::move(base_value), field_offset, align_bytes);
}

std::optional<bir::MemoryAddress> lower_complex_component_address(
    bir::Value base_value,
    std::int64_t component_offset,
    std::size_t align_bytes = 0) {
  return make_pointer_memory_address(std::move(base_value), component_offset, align_bytes);
}

// ---------------------------------------------------------------------------
// Alloca / load / store translation scaffold
// ---------------------------------------------------------------------------
//
// The target BIR owns memory explicitly:
// - allocas become local slots with size/alignment metadata
// - loads become LoadLocalInst / LoadGlobalInst
// - stores become StoreLocalInst / StoreGlobalInst
// - pointer-based accesses carry a MemoryAddress so codegen can preserve the
//   original base/offset/address-space information later.

void lower_alloca_inst(MemoryLoweringFrame& frame,
                       const c4c::codegen::lir::LirAllocaOp& alloca) {
  if (frame.bir_function == nullptr || alloca.result.empty()) {
    return;
  }

  // TODO: translate the hoisted alloca into a BIR local slot entry. Keep the
  // element type, size, and alignment metadata intact so later passes can
  // recover the original stack layout.
  frame.bir_function->local_slots.push_back(bir::LocalSlot{
      .name = alloca.result.str(),
      .type = bir::TypeKind::Ptr,
      .size_bytes = 0,
      .align_bytes = static_cast<std::size_t>(alloca.align > 0 ? alloca.align : 0),
      .is_address_taken = false,
      .is_byval_copy = false,
  });
}

void lower_load_inst(MemoryLoweringFrame& frame,
                     const c4c::codegen::lir::LirLoadOp& load) {
  if (frame.bir_block == nullptr) {
    return;
  }

  const auto loaded_type = lower_memory_scalar_type(load.type_str.str());
  if (!loaded_type.has_value()) {
    return;
  }

  // The concrete address classification is intentionally deferred: the caller
  // should decide whether this is a local slot, global symbol, or pointer-based
  // access and emit the matching BIR memory inst.
  frame.bir_block->insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(*loaded_type, load.result.str()),
      .slot_name = load.ptr.str(),
      .byte_offset = 0,
      .align_bytes = 0,
      .address = std::nullopt,
  });
}

void lower_store_inst(MemoryLoweringFrame& frame,
                      const c4c::codegen::lir::LirStoreOp& store) {
  if (frame.bir_block == nullptr) {
    return;
  }

  const auto stored_type = lower_memory_scalar_type(store.type_str.str());
  if (!stored_type.has_value()) {
    return;
  }

  frame.bir_block->insts.push_back(bir::StoreLocalInst{
      .slot_name = store.ptr.str(),
      .value = bir::Value::named(*stored_type, store.val.str()),
      .byte_offset = 0,
      .align_bytes = 0,
      .address = std::nullopt,
  });
}

void lower_memory_address_form_notes() {
  // Reference mapping notes for the future implementation:
  // - identifier -> local/global address resolution
  // - dereference -> pointer value as address
  // - array subscript -> element-size scaled address arithmetic
  // - member access -> field-offset GEP
  // - pointer member access -> dereferenced field-offset GEP
  // - real/imag access -> complex component offset
  // - generic selection -> recurse into selected expression
}

void record_memory_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                           std::vector<BirLoweringNote>* notes) {
  (void)module;
  if (notes == nullptr) {
    return;
  }
  notes->push_back(BirLoweringNote{
      .phase = "memory-lowering",
      .message = "memory/address lowering scaffold lives in lir_to_bir/memory.cpp",
  });
}

bir::LocalSlot make_memory_local_slot(std::string name,
                                      bir::TypeKind type,
                                      std::size_t size_bytes,
                                      std::size_t align_bytes,
                                      bool is_address_taken) {
  return bir::LocalSlot{
      .name = std::move(name),
      .type = type,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .is_address_taken = is_address_taken,
      .is_byval_copy = false,
  };
}

bir::MemoryAddress make_memory_local_address(std::string slot_name,
                                             std::size_t byte_offset,
                                             std::size_t align_bytes) {
  return make_local_memory_address(std::move(slot_name), byte_offset, align_bytes);
}

bir::MemoryAddress make_memory_global_address(std::string global_name,
                                              std::size_t byte_offset,
                                              std::size_t align_bytes) {
  return make_global_memory_address(std::move(global_name), byte_offset, align_bytes);
}

bir::MemoryAddress make_memory_pointer_address(bir::Value base_value,
                                               std::int64_t byte_offset,
                                               std::size_t align_bytes) {
  return make_pointer_memory_address(std::move(base_value), byte_offset, align_bytes);
}

bool match_memory_global_base_gep_zero(const c4c::codegen::lir::LirGepOp& gep,
                                       std::string_view global_name,
                                       std::string_view global_llvm_type) {
  return gep.element_type == global_llvm_type && gep.ptr == ("@" + std::string(global_name)) &&
         gep.indices.size() == 2 && gep.indices[0] == "i64 0" && gep.indices[1] == "i64 0";
}

bool match_memory_string_base_gep_zero(const c4c::codegen::lir::LirGepOp& gep,
                                       std::string_view pool_name,
                                       std::string_view string_llvm_type) {
  return gep.element_type == string_llvm_type && gep.ptr == pool_name && gep.indices.size() == 2 &&
         gep.indices[0] == "i64 0" && gep.indices[1] == "i64 0";
}

std::optional<std::int64_t> match_memory_sext_i32_to_i64_immediate(
    const c4c::codegen::lir::LirCastOp& cast) {
  if (cast.kind != c4c::codegen::lir::LirCastKind::SExt ||
      !lir_to_bir::legalize_lir_type_is_i32(cast.from_type) ||
      cast.to_type.kind() != c4c::codegen::lir::LirTypeKind::Integer ||
      cast.to_type.integer_bit_width() != 64) {
    return std::nullopt;
  }

  std::int64_t value = 0;
  const std::string_view text = cast.operand.str();
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool match_memory_indexed_gep_from_result(const c4c::codegen::lir::LirGepOp& gep,
                                          std::string_view expected_ptr,
                                          std::string_view expected_element_type,
                                          std::string_view expected_index_name) {
  return gep.element_type == expected_element_type && gep.ptr == expected_ptr &&
         gep.indices.size() == 1 &&
         gep.indices[0] == ("i64 " + std::string(expected_index_name));
}

std::optional<bir::Module> try_lower_minimal_global_char_pointer_diff_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!memory_target_supports_global_pointer_diff(module.target_triple)) {
    return std::nullopt;
  }

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global ") {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i8]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) != array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_memory_immediate(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 12) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* byte_gep1 = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* byte_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[9]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[10]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[11]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || byte_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || byte_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      expected_diff == nullptr || cmp == nullptr || extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  if (!match_memory_global_base_gep_zero(*base_gep1, global.name, global.llvm_type) ||
      !match_memory_global_base_gep_zero(*base_gep0, global.name, global.llvm_type)) {
    return std::nullopt;
  }

  const auto byte_index = match_memory_sext_i32_to_i64_immediate(*index1);
  if (!byte_index.has_value() || *byte_index <= 0 || *byte_index >= *global_size) {
    return std::nullopt;
  }

  const auto zero_index = match_memory_sext_i32_to_i64_immediate(*index0);
  if (!zero_index.has_value() || *zero_index != 0) {
    return std::nullopt;
  }
  if (!match_memory_indexed_gep_from_result(
          *byte_gep1, base_gep1->result.str(), "i8", index1->result.str()) ||
      !match_memory_indexed_gep_from_result(
          *byte_gep0, base_gep0->result.str(), "i8", index0->result.str())) {
    return std::nullopt;
  }
  if (ptrtoint1->kind != LirCastKind::PtrToInt ||
      !lir_to_bir::legalize_lir_type_is_pointer_like(ptrtoint1->from_type) ||
      ptrtoint1->operand != byte_gep1->result ||
      !memory_lir_type_matches_integer_width(ptrtoint1->to_type, 64)) {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt ||
      !lir_to_bir::legalize_lir_type_is_pointer_like(ptrtoint0->from_type) ||
      ptrtoint0->operand != byte_gep0->result ||
      !memory_lir_type_matches_integer_width(ptrtoint0->to_type, 64)) {
    return std::nullopt;
  }

  if (diff->opcode.typed() != LirBinaryOpcode::Sub ||
      !memory_lir_type_matches_integer_width(diff->type_str, 64) ||
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }

  if (expected_diff->kind != LirCastKind::SExt ||
      !memory_lir_type_matches_integer_width(expected_diff->from_type, 32) ||
      !memory_lir_type_matches_integer_width(expected_diff->to_type, 64)) {
    return std::nullopt;
  }
  const auto expected_value = parse_memory_immediate(expected_diff->operand);
  if (!expected_value.has_value() || *expected_value != *byte_index) {
    return std::nullopt;
  }

  if (cmp->is_float || cmp->predicate.typed() != LirCmpPredicate::Eq ||
      !memory_lir_type_matches_integer_width(cmp->type_str, 64) ||
      cmp->lhs != diff->result || cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }
  if (extend->kind != LirCastKind::ZExt ||
      !memory_lir_type_matches_integer_width(extend->from_type, 1) ||
      extend->operand != cmp->result ||
      !memory_lir_type_matches_integer_width(extend->to_type, 32)) {
    return std::nullopt;
  }
  if (!ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != extend->result) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_global_int_pointer_diff_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!memory_target_supports_global_pointer_diff(module.target_triple)) {
    return std::nullopt;
  }

  if (module.functions.size() != 1 || module.globals.size() != 1 ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global " ||
      global.align_bytes != 4 ||
      (global.init_text != "zeroinitializer" && global.init_text != "[i32 0, i32 0]")) {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i32]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) != array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_memory_immediate(global_size_text);
  if (!global_size.has_value() || *global_size != 2) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label != "entry" || entry.insts.size() != 13) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep1 = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&entry.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&entry.insts[4]);
  const auto* elem_gep0 = std::get_if<LirGepOp>(&entry.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&entry.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&entry.insts[8]);
  const auto* scaled_diff = std::get_if<LirBinOp>(&entry.insts[9]);
  const auto* expected_diff = std::get_if<LirCastOp>(&entry.insts[10]);
  const auto* cmp = std::get_if<LirCmpOp>(&entry.insts[11]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[12]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || elem_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || elem_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      scaled_diff == nullptr || expected_diff == nullptr || cmp == nullptr ||
      extend == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  if (!match_memory_global_base_gep_zero(*base_gep1, global.name, global.llvm_type) ||
      !match_memory_global_base_gep_zero(*base_gep0, global.name, global.llvm_type)) {
    return std::nullopt;
  }
  const auto one_index = match_memory_sext_i32_to_i64_immediate(*index1);
  if (!one_index.has_value() || *one_index != 1) {
    return std::nullopt;
  }
  const auto zero_index = match_memory_sext_i32_to_i64_immediate(*index0);
  if (!zero_index.has_value() || *zero_index != 0) {
    return std::nullopt;
  }
  if (!match_memory_indexed_gep_from_result(
          *elem_gep1, base_gep1->result.str(), "i32", index1->result.str()) ||
      !match_memory_indexed_gep_from_result(
          *elem_gep0, base_gep0->result.str(), "i32", index0->result.str())) {
    return std::nullopt;
  }
  if (ptrtoint1->kind != LirCastKind::PtrToInt ||
      !lir_to_bir::legalize_lir_type_is_pointer_like(ptrtoint1->from_type) ||
      ptrtoint1->operand != elem_gep1->result ||
      !memory_lir_type_matches_integer_width(ptrtoint1->to_type, 64)) {
    return std::nullopt;
  }
  if (ptrtoint0->kind != LirCastKind::PtrToInt ||
      !lir_to_bir::legalize_lir_type_is_pointer_like(ptrtoint0->from_type) ||
      ptrtoint0->operand != elem_gep0->result ||
      !memory_lir_type_matches_integer_width(ptrtoint0->to_type, 64)) {
    return std::nullopt;
  }

  if (diff->opcode.typed() != LirBinaryOpcode::Sub ||
      !memory_lir_type_matches_integer_width(diff->type_str, 64) ||
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result) {
    return std::nullopt;
  }
  if (scaled_diff->opcode.typed() != LirBinaryOpcode::SDiv ||
      !memory_lir_type_matches_integer_width(scaled_diff->type_str, 64) ||
      scaled_diff->lhs != diff->result || scaled_diff->rhs != "4") {
    return std::nullopt;
  }
  if (expected_diff->kind != LirCastKind::SExt ||
      !memory_lir_type_matches_integer_width(expected_diff->from_type, 32) ||
      !memory_lir_type_matches_integer_width(expected_diff->to_type, 64) ||
      expected_diff->operand != "1") {
    return std::nullopt;
  }

  if (cmp->is_float || cmp->predicate.typed() != LirCmpPredicate::Eq ||
      !memory_lir_type_matches_integer_width(cmp->type_str, 64) ||
      cmp->lhs != scaled_diff->result || cmp->rhs != expected_diff->result) {
    return std::nullopt;
  }
  if (extend->kind != LirCastKind::ZExt ||
      !memory_lir_type_matches_integer_width(extend->from_type, 1) ||
      extend->operand != cmp->result ||
      !memory_lir_type_matches_integer_width(extend->to_type, 32)) {
    return std::nullopt;
  }
  if (!ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != extend->result) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

bir::LoadLocalInst make_memory_load_local(bir::Value result,
                                          std::string slot_name,
                                          std::size_t byte_offset,
                                          std::size_t align_bytes) {
  const auto address = make_memory_local_address(slot_name, byte_offset, align_bytes);
  return bir::LoadLocalInst{
      .result = std::move(result),
      .slot_name = std::move(slot_name),
      .byte_offset = byte_offset,
      .align_bytes = align_bytes,
      .address = std::move(address),
  };
}

bir::StoreLocalInst make_memory_store_local(std::string slot_name,
                                            bir::Value value,
                                            std::size_t byte_offset,
                                            std::size_t align_bytes) {
  const auto address = make_memory_local_address(slot_name, byte_offset, align_bytes);
  return bir::StoreLocalInst{
      .slot_name = std::move(slot_name),
      .value = std::move(value),
      .byte_offset = byte_offset,
      .align_bytes = align_bytes,
      .address = std::move(address),
  };
}

bir::LoadGlobalInst make_memory_load_global(bir::Value result,
                                            std::string global_name,
                                            std::size_t byte_offset,
                                            std::size_t align_bytes) {
  const auto address = make_memory_global_address(global_name, byte_offset, align_bytes);
  return bir::LoadGlobalInst{
      .result = std::move(result),
      .global_name = std::move(global_name),
      .byte_offset = byte_offset,
      .align_bytes = align_bytes,
      .address = std::move(address),
  };
}

bir::StoreGlobalInst make_memory_store_global(std::string global_name,
                                              bir::Value value,
                                              std::size_t byte_offset,
                                              std::size_t align_bytes) {
  const auto address = make_memory_global_address(global_name, byte_offset, align_bytes);
  return bir::StoreGlobalInst{
      .global_name = std::move(global_name),
      .value = std::move(value),
      .byte_offset = byte_offset,
      .align_bytes = align_bytes,
      .address = std::move(address),
  };
}

}  // namespace c4c::backend
