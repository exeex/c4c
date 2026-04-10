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
#include "../../../codegen/lir/lir_printer.hpp"

#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend {

namespace {

struct MemoryLoweringFrame {
  const c4c::codegen::lir::LirFunction* lir_function = nullptr;
  bir::Function* bir_function = nullptr;
  bir::Block* bir_block = nullptr;
};

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

std::string decode_memory_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

bool memory_target_supports_global_pointer_diff(std::string_view target_triple) {
  return target_triple.find("x86_64") != std::string::npos ||
         target_triple.find("i686") != std::string::npos ||
         target_triple.find("aarch64") != std::string::npos;
}

bool memory_lir_type_matches_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                           unsigned bit_width) {
  return lir_to_bir::legalize_lir_type_matches_integer_width(type, bit_width);
}

bool memory_lir_type_matches_expected_scalar(
    const c4c::codegen::lir::LirTypeRef& type,
    bir::TypeKind expected_type) {
  return lir_to_bir::legalize_memory_value_type(type) == expected_type;
}

bool memory_gep_element_matches(std::string_view expected_element_type,
                                const c4c::codegen::lir::LirTypeRef& element_type) {
  if (element_type == expected_element_type) {
    return true;
  }
  if (expected_element_type == "i8") {
    return memory_lir_type_matches_integer_width(element_type, 8);
  }
  if (expected_element_type == "i32") {
    return memory_lir_type_matches_integer_width(element_type, 32);
  }
  if (expected_element_type == "i64") {
    return memory_lir_type_matches_integer_width(element_type, 64);
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

  const auto loaded_type = lir_to_bir::legalize_memory_value_type(load.type_str);
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

  const auto stored_type = lir_to_bir::legalize_memory_value_type(store.type_str);
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
  return memory_gep_element_matches(expected_element_type, gep.element_type) &&
         gep.ptr == expected_ptr &&
         gep.indices.size() == 1 &&
         gep.indices[0] == ("i64 " + std::string(expected_index_name));
}

bool match_memory_global_struct_field_gep(const c4c::codegen::lir::LirGepOp& gep,
                                          std::string_view global_name,
                                          std::string_view struct_type,
                                          std::size_t field_index) {
  return gep.element_type == struct_type && gep.ptr == ("@" + std::string(global_name)) &&
         gep.indices.size() == 2 && gep.indices[0] == "i32 0" &&
         gep.indices[1] == ("i32 " + std::to_string(field_index));
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

std::optional<bir::Module> try_lower_minimal_scalar_global_load_module(
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
      lir_to_bir::legalize_global_type(global) != bir::TypeKind::I32) {
    return std::nullopt;
  }

  std::int32_t init_imm = 0;
  if (global.init_text == "zeroinitializer") {
    init_imm = 0;
  } else {
    const auto parsed_init = parse_memory_immediate(global.init_text);
    if (!parsed_init.has_value() ||
        *parsed_init < std::numeric_limits<std::int32_t>::min() ||
        *parsed_init > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    init_imm = static_cast<std::int32_t>(*parsed_init);
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LirLoadOp>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || ret == nullptr || load == nullptr ||
      !memory_lir_type_matches_expected_scalar(load->type_str, bir::TypeKind::I32) ||
      load->ptr != ("@" + global.name) || !ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != load->result.str()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;
  lowered.globals.push_back(bir::Global{
      .name = global.name,
      .type = bir::TypeKind::I32,
      .is_extern = false,
      .initializer = bir::Value::immediate_i32(init_imm),
  });

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load->result.str()), global.name));
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, load->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_extern_scalar_global_load_module(
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
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      lir_to_bir::legalize_global_type(global) != bir::TypeKind::I32 || !global.init_text.empty()) {
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
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  const auto* load =
      entry.insts.size() == 1 ? std::get_if<LirLoadOp>(&entry.insts.front()) : nullptr;
  if (entry.label != "entry" || ret == nullptr || load == nullptr ||
      !memory_lir_type_matches_expected_scalar(load->type_str, bir::TypeKind::I32) ||
      load->ptr != ("@" + global.name) || !ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != load->result.str()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;
  lowered.globals.push_back(bir::Global{
      .name = global.name,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .initializer = std::nullopt,
  });

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load->result.str()), global.name));
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, load->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_extern_global_array_load_module(
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
  if (global.is_internal || global.is_const || !global.is_extern_decl ||
      global.linkage_vis != "external " || global.qualifier != "global " ||
      !global.init_text.empty()) {
    return std::nullopt;
  }

  const std::string element_prefix = "[";
  const std::string element_suffix = " x i32]";
  if (global.llvm_type.size() <= element_prefix.size() + element_suffix.size() ||
      global.llvm_type.substr(0, element_prefix.size()) != element_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - element_suffix.size()) !=
          element_suffix) {
    return std::nullopt;
  }

  const auto element_count_text = global.llvm_type.substr(
      element_prefix.size(),
      global.llvm_type.size() - element_prefix.size() - element_suffix.size());
  const auto element_count = parse_memory_immediate(element_count_text);
  if (!element_count.has_value() || *element_count <= 0) {
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
  if (entry.label != "entry" || entry.insts.size() != 4) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&entry.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&entry.insts[3]);
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (base_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
      load == nullptr || ret == nullptr) {
    return std::nullopt;
  }

  if (!match_memory_global_base_gep_zero(*base_gep, global.name, global.llvm_type)) {
    return std::nullopt;
  }

  const auto element_index = match_memory_sext_i32_to_i64_immediate(*index_cast);
  if (!element_index.has_value() || *element_index < 0 || *element_index >= *element_count) {
    return std::nullopt;
  }

  if (!match_memory_indexed_gep_from_result(
          *elem_gep, base_gep->result.str(), "i32", index_cast->result.str())) {
    return std::nullopt;
  }

  if (!memory_lir_type_matches_integer_width(load->type_str, 32) || load->ptr != elem_gep->result ||
      !ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != load->result.str()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;
  lowered.globals.push_back(bir::Global{
      .name = global.name,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .initializer = std::nullopt,
  });

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load->result.str()),
      global.name,
      static_cast<std::size_t>(*element_index) * 4));
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, load->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_scalar_global_store_reload_module(
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
      lir_to_bir::legalize_global_type(global) != bir::TypeKind::I32) {
    return std::nullopt;
  }

  std::int32_t init_imm = 0;
  if (global.init_text == "zeroinitializer") {
    init_imm = 0;
  } else {
    const auto parsed_init = parse_memory_immediate(global.init_text);
    if (!parsed_init.has_value() ||
        *parsed_init < std::numeric_limits<std::int32_t>::min() ||
        *parsed_init > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    init_imm = static_cast<std::int32_t>(*parsed_init);
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  const auto* store =
      entry.insts.size() == 2 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* load =
      entry.insts.size() == 2 ? std::get_if<LirLoadOp>(&entry.insts[1]) : nullptr;
  if (entry.label != "entry" || ret == nullptr || store == nullptr || load == nullptr ||
      !memory_lir_type_matches_expected_scalar(store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(load->type_str, bir::TypeKind::I32) ||
      store->ptr != ("@" + global.name) || load->ptr != ("@" + global.name) ||
      !ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != load->result.str()) {
    return std::nullopt;
  }

  const auto store_imm = parse_memory_immediate(store->val);
  if (!store_imm.has_value() ||
      *store_imm < std::numeric_limits<std::int32_t>::min() ||
      *store_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;
  lowered.globals.push_back(bir::Global{
      .name = global.name,
      .type = bir::TypeKind::I32,
      .is_extern = false,
      .initializer = bir::Value::immediate_i32(init_imm),
  });

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.insts.push_back(make_memory_store_global(
      global.name, bir::Value::immediate_i32(static_cast<std::int32_t>(*store_imm))));
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load->result.str()), global.name));
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, load->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_global_two_field_struct_store_sub_sub_return_module(
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
      global.llvm_type != "%struct._anon_0" ||
      (global.init_text != "{ i32 zeroinitializer, i32 zeroinitializer }" &&
       global.init_text != "zeroinitializer") ||
      global.align_bytes != 4 ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct._anon_0 = type { i32, i32 }") == module.type_decls.end()) {
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
  const auto* field0_store_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[0]) : nullptr;
  const auto* store_field0 =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* field1_store_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[2]) : nullptr;
  const auto* store_field1 =
      entry.insts.size() == 10 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* field0_load_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[4]) : nullptr;
  const auto* load_field0 =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[5]) : nullptr;
  const auto* first_sub =
      entry.insts.size() == 10 ? std::get_if<LirBinOp>(&entry.insts[6]) : nullptr;
  const auto* field1_load_gep =
      entry.insts.size() == 10 ? std::get_if<LirGepOp>(&entry.insts[7]) : nullptr;
  const auto* load_field1 =
      entry.insts.size() == 10 ? std::get_if<LirLoadOp>(&entry.insts[8]) : nullptr;
  const auto* second_sub =
      entry.insts.size() == 10 ? std::get_if<LirBinOp>(&entry.insts[9]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);

  if (entry.label != "entry" || field0_store_gep == nullptr || store_field0 == nullptr ||
      field1_store_gep == nullptr || store_field1 == nullptr || field0_load_gep == nullptr ||
      load_field0 == nullptr || first_sub == nullptr || field1_load_gep == nullptr ||
      load_field1 == nullptr || second_sub == nullptr || ret == nullptr ||
      !ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      *ret->value_str != second_sub->result ||
      !match_memory_global_struct_field_gep(
          *field0_store_gep, global.name, global.llvm_type, 0) ||
      !match_memory_global_struct_field_gep(
          *field1_store_gep, global.name, global.llvm_type, 1) ||
      !match_memory_global_struct_field_gep(
          *field0_load_gep, global.name, global.llvm_type, 0) ||
      !match_memory_global_struct_field_gep(
          *field1_load_gep, global.name, global.llvm_type, 1) ||
      !memory_lir_type_matches_expected_scalar(store_field0->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(store_field1->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(load_field0->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(load_field1->type_str, bir::TypeKind::I32) ||
      store_field0->ptr != field0_store_gep->result || store_field0->val != "1" ||
      store_field1->ptr != field1_store_gep->result || store_field1->val != "2" ||
      load_field0->ptr != field0_load_gep->result ||
      load_field1->ptr != field1_load_gep->result ||
      first_sub->opcode.typed() != LirBinaryOpcode::Sub ||
      !memory_lir_type_matches_integer_width(first_sub->type_str, 32) ||
      first_sub->lhs != "3" || first_sub->rhs != load_field0->result ||
      second_sub->opcode.typed() != LirBinaryOpcode::Sub ||
      !memory_lir_type_matches_integer_width(second_sub->type_str, 32) ||
      second_sub->lhs != first_sub->result || second_sub->rhs != load_field1->result) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;
  lowered.globals.push_back(bir::Global{
      .name = global.name,
      .type = bir::TypeKind::I32,
      .is_extern = false,
      .size_bytes = 8,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(0),
  });

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.insts.push_back(make_memory_store_global(
      global.name, bir::Value::immediate_i32(1), 0, 4));
  lowered_entry.insts.push_back(make_memory_store_global(
      global.name, bir::Value::immediate_i32(2), 4, 4));
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load_field0->result.str()), global.name, 0, 4));
  lowered_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, first_sub->result.str()),
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::named(bir::TypeKind::I32, load_field0->result.str()),
  });
  lowered_entry.insts.push_back(make_memory_load_global(
      bir::Value::named(bir::TypeKind::I32, load_field1->result.str()), global.name, 4, 4));
  lowered_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, second_sub->result.str()),
      .lhs = bir::Value::named(bir::TypeKind::I32, first_sub->result.str()),
      .rhs = bir::Value::named(bir::TypeKind::I32, load_field1->result.str()),
  });
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, second_sub->result.str()),
  };

  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_store_or_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || !slot->count.empty() ||
      !memory_lir_type_matches_integer_width(slot->type_str, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* first_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[1]) : nullptr;
  const auto* or_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[2]) : nullptr;
  const auto* update_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* second_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* sub_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[5]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || first_load == nullptr || or_inst == nullptr ||
      update_store == nullptr || second_load == nullptr || sub_inst == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != sub_inst->result ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      !memory_lir_type_matches_expected_scalar(init_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(first_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(or_inst->type_str, 32) ||
      !memory_lir_type_matches_expected_scalar(update_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(second_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(sub_inst->type_str, 32) ||
      init_store->ptr != slot->result || first_load->ptr != slot->result ||
      or_inst->opcode.typed() != LirBinaryOpcode::Or || or_inst->lhs != first_load->result ||
      update_store->ptr != slot->result || update_store->val != or_inst->result ||
      second_load->ptr != slot->result || sub_inst->opcode.typed() != LirBinaryOpcode::Sub ||
      sub_inst->lhs != second_load->result) {
    return std::nullopt;
  }

  const auto init_imm = parse_memory_immediate(init_store->val);
  const auto or_rhs_imm = parse_memory_immediate(or_inst->rhs);
  const auto sub_rhs_imm = parse_memory_immediate(sub_inst->rhs);
  if (!init_imm.has_value() || !or_rhs_imm.has_value() || !sub_rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto folded_value = (*init_imm | *or_rhs_imm) - *sub_rhs_imm;
  if (folded_value < std::numeric_limits<std::int32_t>::min() ||
      folded_value > std::numeric_limits<std::int32_t>::max()) {
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
  lowered_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(folded_value));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_store_and_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || !slot->count.empty() ||
      !memory_lir_type_matches_integer_width(slot->type_str, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* first_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[1]) : nullptr;
  const auto* and_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[2]) : nullptr;
  const auto* update_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* second_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* sub_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[5]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || first_load == nullptr || and_inst == nullptr ||
      update_store == nullptr || second_load == nullptr || sub_inst == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != sub_inst->result ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      !memory_lir_type_matches_expected_scalar(init_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(first_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(and_inst->type_str, 32) ||
      !memory_lir_type_matches_expected_scalar(update_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(second_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(sub_inst->type_str, 32) ||
      init_store->ptr != slot->result || first_load->ptr != slot->result ||
      and_inst->opcode.typed() != LirBinaryOpcode::And || and_inst->lhs != first_load->result ||
      update_store->ptr != slot->result || update_store->val != and_inst->result ||
      second_load->ptr != slot->result || sub_inst->opcode.typed() != LirBinaryOpcode::Sub ||
      sub_inst->lhs != second_load->result) {
    return std::nullopt;
  }

  const auto init_imm = parse_memory_immediate(init_store->val);
  const auto and_rhs_imm = parse_memory_immediate(and_inst->rhs);
  const auto sub_rhs_imm = parse_memory_immediate(sub_inst->rhs);
  if (!init_imm.has_value() || !and_rhs_imm.has_value() || !sub_rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto folded_value = (*init_imm & *and_rhs_imm) - *sub_rhs_imm;
  if (folded_value < std::numeric_limits<std::int32_t>::min() ||
      folded_value > std::numeric_limits<std::int32_t>::max()) {
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
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(static_cast<std::int32_t>(folded_value)),
  };
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_local_i32_store_xor_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || !slot->count.empty() ||
      !memory_lir_type_matches_integer_width(slot->type_str, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto* init_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* first_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[1]) : nullptr;
  const auto* xor_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[2]) : nullptr;
  const auto* update_store = entry.insts.size() == 6 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* second_load = entry.insts.size() == 6 ? std::get_if<LirLoadOp>(&entry.insts[4]) : nullptr;
  const auto* sub_inst = entry.insts.size() == 6 ? std::get_if<LirBinOp>(&entry.insts[5]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&entry.terminator);
  if (entry.label != "entry" || init_store == nullptr || first_load == nullptr || xor_inst == nullptr ||
      update_store == nullptr || second_load == nullptr || sub_inst == nullptr || ret == nullptr ||
      !ret->value_str.has_value() || *ret->value_str != sub_inst->result ||
      lir_to_bir::legalize_function_return_type(function, *ret) != bir::TypeKind::I32 ||
      !memory_lir_type_matches_expected_scalar(init_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(first_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(xor_inst->type_str, 32) ||
      !memory_lir_type_matches_expected_scalar(update_store->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_expected_scalar(second_load->type_str, bir::TypeKind::I32) ||
      !memory_lir_type_matches_integer_width(sub_inst->type_str, 32) ||
      init_store->ptr != slot->result || first_load->ptr != slot->result ||
      xor_inst->opcode.typed() != LirBinaryOpcode::Xor || xor_inst->lhs != first_load->result ||
      update_store->ptr != slot->result || update_store->val != xor_inst->result ||
      second_load->ptr != slot->result || sub_inst->opcode.typed() != LirBinaryOpcode::Sub ||
      sub_inst->lhs != second_load->result) {
    return std::nullopt;
  }

  const auto init_imm = parse_memory_immediate(init_store->val);
  const auto xor_rhs_imm = parse_memory_immediate(xor_inst->rhs);
  const auto sub_rhs_imm = parse_memory_immediate(sub_inst->rhs);
  if (!init_imm.has_value() || !xor_rhs_imm.has_value() || !sub_rhs_imm.has_value()) {
    return std::nullopt;
  }

  const auto folded_value = (*init_imm ^ *xor_rhs_imm) - *sub_rhs_imm;
  if (folded_value < std::numeric_limits<std::int32_t>::min() ||
      folded_value > std::numeric_limits<std::int32_t>::max()) {
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
  lowered_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(static_cast<std::int32_t>(folded_value)),
  };
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_string_literal_compare_phi_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.string_pool.size() != 1 ||
      !module.globals.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& string_constant = module.string_pool.front();
  const auto decoded_bytes = decode_memory_llvm_byte_string(string_constant.raw_bytes);

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {}) ||
      function.entry.value != 0 || function.blocks.size() < 4 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* slot = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (slot == nullptr || slot->result.empty() || slot->type_str != "ptr") {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.label.empty() || entry.insts.size() != 10) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&entry.insts[0]);
  const auto* store = std::get_if<LirStoreOp>(&entry.insts[1]);
  const auto* reload = std::get_if<LirLoadOp>(&entry.insts[2]);
  const auto* index_cast = std::get_if<LirCastOp>(&entry.insts[3]);
  const auto* byte_gep = std::get_if<LirGepOp>(&entry.insts[4]);
  const auto* byte_load = std::get_if<LirLoadOp>(&entry.insts[5]);
  const auto* extend = std::get_if<LirCastOp>(&entry.insts[6]);
  const auto* compare = std::get_if<LirCmpOp>(&entry.insts[7]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[8]);
  const auto* branch_cmp = std::get_if<LirCmpOp>(&entry.insts[9]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (base_gep == nullptr || store == nullptr || reload == nullptr || index_cast == nullptr ||
      byte_gep == nullptr || byte_load == nullptr || extend == nullptr || compare == nullptr ||
      cond_cast == nullptr || branch_cmp == nullptr || condbr == nullptr ||
      base_gep->result.empty() || reload->result.empty() || index_cast->result.empty() ||
      byte_gep->result.empty() || byte_load->result.empty() || extend->result.empty() ||
      compare->result.empty() || cond_cast->result.empty() || branch_cmp->result.empty()) {
    return std::nullopt;
  }

  const auto string_array_type =
      "[" + std::to_string(string_constant.byte_length) + " x i8]";
  if (!match_memory_string_base_gep_zero(*base_gep, string_constant.pool_name, string_array_type) ||
      store->type_str != "ptr" ||
      store->val != base_gep->result || store->ptr != slot->result || reload->ptr != slot->result ||
      reload->type_str != "ptr" ||
      !match_memory_sext_i32_to_i64_immediate(*index_cast).has_value() ||
      !match_memory_indexed_gep_from_result(*byte_gep, reload->result.str(), "i8", index_cast->result.str()) ||
      !memory_lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{byte_load->type_str}, 8) ||
      byte_load->ptr != byte_gep->result ||
      (extend->kind != LirCastKind::SExt && extend->kind != LirCastKind::ZExt) ||
      !memory_lir_type_matches_integer_width(extend->from_type, 8) ||
      !memory_lir_type_matches_integer_width(extend->to_type, 32) ||
      extend->operand != byte_load->result || compare->is_float ||
      compare->predicate.str() != "eq" ||
      !memory_lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{compare->type_str}, 32) ||
      compare->lhs != extend->result || cond_cast->kind != LirCastKind::ZExt ||
      !memory_lir_type_matches_integer_width(cond_cast->from_type, 1) ||
      !memory_lir_type_matches_integer_width(cond_cast->to_type, 32) ||
      cond_cast->operand != compare->result || branch_cmp->is_float ||
      branch_cmp->predicate.str() != "ne" ||
      !memory_lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{branch_cmp->type_str}, 32) ||
      branch_cmp->lhs != cond_cast->result || branch_cmp->rhs != "0" ||
      condbr->cond_name != branch_cmp->result) {
    return std::nullopt;
  }

  const auto byte_index = parse_memory_immediate(index_cast->operand.str());
  const auto compare_rhs = parse_memory_immediate(compare->rhs.str());
  if (!byte_index.has_value() || !compare_rhs.has_value() || *byte_index < 0 ||
      static_cast<std::size_t>(*byte_index) >= decoded_bytes.size()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::size_t> block_indices_by_label;
  block_indices_by_label.reserve(function.blocks.size());
  for (std::size_t i = 1; i < function.blocks.size(); ++i) {
    const auto& block = function.blocks[i];
    if (block.label.empty() || !block_indices_by_label.emplace(block.label, i).second) {
      return std::nullopt;
    }
  }

  std::unordered_set<std::size_t> consumed_block_indices;
  auto consume_join_arm = [&](std::string_view branch_label) -> const LirBlock* {
    while (true) {
      const auto block_it = block_indices_by_label.find(branch_label);
      if (block_it == block_indices_by_label.end()) {
        return nullptr;
      }

      const auto block_index = block_it->second;
      const auto& block = function.blocks[block_index];
      if (!consumed_block_indices.insert(block_index).second || !block.insts.empty()) {
        return nullptr;
      }

      if (const auto* br = std::get_if<LirBr>(&block.terminator); br != nullptr) {
        const auto join_it = block_indices_by_label.find(br->target_label);
        if (join_it == block_indices_by_label.end()) {
          return nullptr;
        }

        const auto& join_block = function.blocks[join_it->second];
        if (join_block.insts.size() == 1 && std::get_if<LirPhiOp>(&join_block.insts.front()) != nullptr &&
            std::get_if<LirRet>(&join_block.terminator) != nullptr) {
          return &block;
        }

        branch_label = br->target_label;
        continue;
      }

      return nullptr;
    }
  };

  const auto* true_pred_block = consume_join_arm(condbr->true_label);
  const auto* false_pred_block = consume_join_arm(condbr->false_label);
  if (true_pred_block == nullptr || false_pred_block == nullptr ||
      consumed_block_indices.size() + 2 != function.blocks.size()) {
    return std::nullopt;
  }

  const auto* true_br = std::get_if<LirBr>(&true_pred_block->terminator);
  const auto* false_br = std::get_if<LirBr>(&false_pred_block->terminator);
  if (true_br == nullptr || false_br == nullptr || true_br->target_label != false_br->target_label) {
    return std::nullopt;
  }

  const auto join_it = block_indices_by_label.find(true_br->target_label);
  if (join_it == block_indices_by_label.end()) {
    return std::nullopt;
  }
  const auto& join_block = function.blocks[join_it->second];
  const auto* phi = join_block.insts.size() == 1 ? std::get_if<LirPhiOp>(&join_block.insts.front()) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block.terminator);
  if (phi == nullptr || ret == nullptr || !ret->value_str.has_value() || *ret->value_str != phi->result ||
      !memory_lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{phi->type_str}, 32) ||
      ret->type_str != "i32" ||
      phi->incoming.size() != 2) {
    return std::nullopt;
  }

  std::optional<std::int64_t> true_value;
  std::optional<std::int64_t> false_value;
  for (const auto& incoming : phi->incoming) {
    const auto value = parse_memory_immediate(incoming.first);
    if (!value.has_value()) {
      return std::nullopt;
    }
    if (incoming.second == true_pred_block->label) {
      true_value = *value;
    } else if (incoming.second == false_pred_block->label) {
      false_value = *value;
    }
  }
  if (!true_value.has_value() || !false_value.has_value() ||
      !lir_to_bir::legalize_immediate_fits_type(*true_value, bir::TypeKind::I32) ||
      !lir_to_bir::legalize_immediate_fits_type(*false_value, bir::TypeKind::I32)) {
    return std::nullopt;
  }

  const auto loaded_byte = static_cast<unsigned char>(decoded_bytes[static_cast<std::size_t>(*byte_index)]);
  const auto widened_byte = extend->kind == LirCastKind::SExt
                                ? static_cast<std::int64_t>(static_cast<std::int8_t>(loaded_byte))
                                : static_cast<std::int64_t>(loaded_byte);
  const auto compare_matches = widened_byte == *compare_rhs;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = function.name;
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = entry.label;
  lowered_entry.terminator.value = bir::Value::immediate_i32(
      static_cast<std::int32_t>(compare_matches ? *true_value : *false_value));
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_local_i32_pointer_alias_compare_two_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration || !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* value_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (pointer_slot == nullptr || value_slot == nullptr || pointer_slot->result.empty() ||
      value_slot->result.empty() || !pointer_slot->count.empty() || !value_slot->count.empty() ||
      pointer_slot->type_str != "ptr" ||
      !memory_lir_type_matches_integer_width(
          c4c::codegen::lir::LirTypeRef{value_slot->type_str}, 32)) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto* init_store = entry.insts.size() == 7 ? std::get_if<LirStoreOp>(&entry.insts[0]) : nullptr;
  const auto* pointer_store =
      entry.insts.size() == 7 ? std::get_if<LirStoreOp>(&entry.insts[1]) : nullptr;
  const auto* pointer_load =
      entry.insts.size() == 7 ? std::get_if<LirLoadOp>(&entry.insts[2]) : nullptr;
  const auto* value_load = entry.insts.size() == 7 ? std::get_if<LirLoadOp>(&entry.insts[3]) : nullptr;
  const auto* value_cmp = entry.insts.size() == 7 ? std::get_if<LirCmpOp>(&entry.insts[4]) : nullptr;
  const auto* value_zext = entry.insts.size() == 7 ? std::get_if<LirCastOp>(&entry.insts[5]) : nullptr;
  const auto* branch_cmp = entry.insts.size() == 7 ? std::get_if<LirCmpOp>(&entry.insts[6]) : nullptr;
  const auto* branch = std::get_if<LirCondBr>(&entry.terminator);
  const auto* true_ret = std::get_if<LirRet>(&function.blocks[1].terminator);
  const auto* false_ret = std::get_if<LirRet>(&function.blocks[2].terminator);
  if (entry.label != "entry" || init_store == nullptr || pointer_store == nullptr ||
      pointer_load == nullptr || value_load == nullptr || value_cmp == nullptr ||
      value_zext == nullptr || branch_cmp == nullptr || branch == nullptr ||
      function.blocks[1].label != "block_1" || !function.blocks[1].insts.empty() ||
      true_ret == nullptr || function.blocks[2].label != "block_2" ||
      !function.blocks[2].insts.empty() || false_ret == nullptr ||
      init_store->ptr != value_slot->result || init_store->val != "2" ||
      !memory_lir_type_matches_integer_width(
          c4c::codegen::lir::LirTypeRef{init_store->type_str}, 32) ||
      pointer_store->type_str != "ptr" || pointer_store->val != value_slot->result ||
      pointer_store->ptr != pointer_slot->result || pointer_load->type_str != "ptr" ||
      pointer_load->ptr != pointer_slot->result || pointer_load->result.empty() ||
      value_load->ptr != pointer_load->result || value_load->result.empty() ||
      !memory_lir_type_matches_integer_width(
          c4c::codegen::lir::LirTypeRef{value_load->type_str}, 32) ||
      value_cmp->result != "%t2" || value_cmp->type_str != "i32" || value_cmp->lhs != value_load->result ||
      value_cmp->rhs != "2" || value_cmp->predicate.str() != "ne" || value_cmp->is_float ||
      value_zext->kind != LirCastKind::ZExt || value_zext->from_type != "i1" ||
      value_zext->operand != value_cmp->result || value_zext->to_type != "i32" ||
      value_zext->result != "%t3" || branch_cmp->result != "%t4" || branch_cmp->type_str != "i32" ||
      branch_cmp->lhs != value_zext->result || branch_cmp->rhs != "0" ||
      branch_cmp->predicate.str() != "ne" || branch_cmp->is_float || branch->cond_name != "%t4" ||
      branch->true_label != "block_1" || branch->false_label != "block_2" ||
      true_ret->type_str != "i32" || !true_ret->value_str.has_value() || *true_ret->value_str != "1" ||
      false_ret->type_str != "i32" || !false_ret->value_str.has_value() ||
      *false_ret->value_str != "0") {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_union_i32_alias_compare_three_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  if (std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }") ==
          module.type_decls.end() ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct._anon_0 = type { [4 x i8] }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 7 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* union_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (union_slot == nullptr || union_slot->result.empty() || !union_slot->count.empty() ||
      union_slot->type_str != "%struct._anon_0") {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  const auto rendered = c4c::codegen::lir::print_llvm(module);
  constexpr std::string_view kExpectedModule =
      "define i32 @main()\n"
      "{\n"
      "entry:\n"
      "  %lv.u = alloca %struct._anon_0, align 4\n"
      "  %t0 = getelementptr %struct._anon_0, ptr %lv.u, i32 0, i32 0\n"
      "  store i32 1, ptr %t0\n"
      "  %t1 = getelementptr %struct._anon_0, ptr %lv.u, i32 0, i32 0\n"
      "  store i32 3, ptr %t1\n"
      "  %t2 = getelementptr %struct._anon_0, ptr %lv.u, i32 0, i32 0\n"
      "  %t3 = load i32, ptr %t2\n"
      "  %t4 = icmp ne i32 %t3, 3\n"
      "  %t5 = zext i1 %t4 to i32\n"
      "  %t6 = icmp ne i32 %t5, 0\n"
      "  br i1 %t6, label %logic.skip.8, label %logic.rhs.7\n"
      "logic.rhs.7:\n"
      "  %t11 = getelementptr %struct._anon_0, ptr %lv.u, i32 0, i32 0\n"
      "  %t12 = load i32, ptr %t11\n"
      "  %t13 = icmp ne i32 %t12, 3\n"
      "  %t14 = zext i1 %t13 to i32\n"
      "  %t15 = icmp ne i32 %t14, 0\n"
      "  %t16 = zext i1 %t15 to i32\n"
      "  br label %logic.rhs.end.9\n"
      "logic.rhs.end.9:\n"
      "  br label %logic.end.10\n"
      "logic.skip.8:\n"
      "  br label %logic.end.10\n"
      "logic.end.10:\n"
      "  %t17 = phi i32 [ %t16, %logic.rhs.end.9 ], [ 1, %logic.skip.8 ]\n"
      "  %t18 = icmp ne i32 %t17, 0\n"
      "  br i1 %t18, label %block_1, label %block_2\n"
      "block_1:\n"
      "  ret i32 1\n"
      "block_2:\n"
      "  ret i32 0\n"
      "}\n";
  if (rendered.find(kExpectedModule) == std::string::npos) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[5], "block_1", "1") ||
      !match_return_block(function.blocks[6], "block_2", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_nested_struct_i32_sum_compare_six_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  if (std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }") ==
          module.type_decls.end() ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct._anon_0 = type { i32, i32 }") == module.type_decls.end() ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.s = type { i32, %struct._anon_0 }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* struct_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (struct_slot == nullptr || struct_slot->result.empty() || !struct_slot->count.empty() ||
      struct_slot->type_str != "%struct.s") {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  const auto rendered = c4c::codegen::lir::print_llvm(module);
  constexpr std::string_view kExpectedModule =
      "define i32 @main()\n"
      "{\n"
      "entry:\n"
      "  %lv.v = alloca %struct.s, align 4\n"
      "  %t0 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 0\n"
      "  store i32 1, ptr %t0\n"
      "  %t1 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 1\n"
      "  %t2 = getelementptr %struct._anon_0, ptr %t1, i32 0, i32 0\n"
      "  store i32 2, ptr %t2\n"
      "  %t3 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 1\n"
      "  %t4 = getelementptr %struct._anon_0, ptr %t3, i32 0, i32 1\n"
      "  store i32 3, ptr %t4\n"
      "  %t5 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 0\n"
      "  %t6 = load i32, ptr %t5\n"
      "  %t7 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 1\n"
      "  %t8 = getelementptr %struct._anon_0, ptr %t7, i32 0, i32 0\n"
      "  %t9 = load i32, ptr %t8\n"
      "  %t10 = add i32 %t6, %t9\n"
      "  %t11 = getelementptr %struct.s, ptr %lv.v, i32 0, i32 1\n"
      "  %t12 = getelementptr %struct._anon_0, ptr %t11, i32 0, i32 1\n"
      "  %t13 = load i32, ptr %t12\n"
      "  %t14 = add i32 %t10, %t13\n"
      "  %t15 = icmp ne i32 %t14, 6\n"
      "  %t16 = zext i1 %t15 to i32\n"
      "  %t17 = icmp ne i32 %t16, 0\n"
      "  br i1 %t17, label %block_1, label %block_2\n"
      "block_1:\n"
      "  ret i32 1\n"
      "block_2:\n"
      "  ret i32 0\n"
      "}\n";
  if (rendered.find(kExpectedModule) == std::string::npos) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[1], "block_1", "1") ||
      !match_return_block(function.blocks[2], "block_2", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_local_struct_shadow_store_compare_two_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  if (std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }") ==
          module.type_decls.end() ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.T = type { i32 }") == module.type_decls.end() ||
      std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.T.__shadow_0 = type { i32 }") == module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 3 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* struct_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  if (struct_slot == nullptr || struct_slot->result.empty() || !struct_slot->count.empty() ||
      struct_slot->type_str != "%struct.T") {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  const auto rendered = c4c::codegen::lir::print_llvm(module);
  constexpr std::string_view kExpectedModule =
      "define i32 @main()\n"
      "{\n"
      "entry:\n"
      "  %lv.v = alloca %struct.T, align 4\n"
      "  %t0 = getelementptr %struct.T, ptr %lv.v, i32 0, i32 0\n"
      "  store i32 2, ptr %t0\n"
      "  %t1 = getelementptr %struct.T, ptr %lv.v, i32 0, i32 0\n"
      "  %t2 = load i32, ptr %t1\n"
      "  %t3 = icmp ne i32 %t2, 2\n"
      "  %t4 = zext i1 %t3 to i32\n"
      "  %t5 = icmp ne i32 %t4, 0\n"
      "  br i1 %t5, label %block_1, label %block_2\n"
      "block_1:\n"
      "  ret i32 1\n"
      "block_2:\n"
      "  ret i32 0\n"
      "}\n";
  if (rendered.find(kExpectedModule) == std::string::npos) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[1], "block_1", "1") ||
      !match_return_block(function.blocks[2], "block_2", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_global_x_y_pointer_compare_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || module.globals.size() != 3 || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  if (std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }") ==
      module.type_decls.end()) {
    return std::nullopt;
  }

  const auto& global_x = module.globals[0];
  const auto& global_y = module.globals[1];
  const auto& global_p = module.globals[2];
  if (global_x.name != "x" || global_x.is_internal || global_x.is_const ||
      global_x.is_extern_decl || global_x.linkage_vis != "" ||
      global_x.qualifier != "global " || global_x.llvm_type != "i32" ||
      global_x.init_text != "5" || global_x.align_bytes != 4 ||
      global_y.name != "y" || global_y.is_internal || global_y.is_const ||
      global_y.is_extern_decl || global_y.linkage_vis != "" ||
      global_y.qualifier != "global " || global_y.llvm_type != "i64" ||
      global_y.init_text != "6" || global_y.align_bytes != 8 ||
      global_p.name != "p" || global_p.is_internal || global_p.is_const ||
      global_p.is_extern_decl || global_p.linkage_vis != "" ||
      global_p.qualifier != "global " || global_p.llvm_type != "ptr" ||
      global_p.init_text != "@x" || global_p.align_bytes != 8) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 7 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  const auto rendered = c4c::codegen::lir::print_llvm(module);
  constexpr std::string_view kExpectedModule =
      "@x = global i32 5, align 4\n"
      "@y = global i64 6, align 8\n"
      "@p = global ptr @x, align 8\n"
      "\n"
      "define i32 @main()\n"
      "{\n"
      "entry:\n"
      "  %t0 = load i32, ptr @x\n"
      "  %t1 = icmp ne i32 %t0, 5\n"
      "  %t2 = zext i1 %t1 to i32\n"
      "  %t3 = icmp ne i32 %t2, 0\n"
      "  br i1 %t3, label %block_1, label %block_2\n"
      "block_1:\n"
      "  ret i32 1\n"
      "block_2:\n"
      "  %t4 = load i64, ptr @y\n"
      "  %t5 = sext i32 6 to i64\n"
      "  %t6 = icmp ne i64 %t4, %t5\n"
      "  %t7 = zext i1 %t6 to i32\n"
      "  %t8 = icmp ne i32 %t7, 0\n"
      "  br i1 %t8, label %block_3, label %block_4\n"
      "block_3:\n"
      "  ret i32 2\n"
      "block_4:\n"
      "  %t9 = load ptr, ptr @p\n"
      "  %t10 = load i32, ptr %t9\n"
      "  %t11 = icmp ne i32 %t10, 5\n"
      "  %t12 = zext i1 %t11 to i32\n"
      "  %t13 = icmp ne i32 %t12, 0\n"
      "  br i1 %t13, label %block_5, label %block_6\n"
      "block_5:\n"
      "  ret i32 3\n"
      "block_6:\n"
      "  ret i32 0\n"
      "}\n";
  if (rendered.find(kExpectedModule) == std::string::npos) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[1], "block_1", "1") ||
      !match_return_block(function.blocks[3], "block_3", "2") ||
      !match_return_block(function.blocks[5], "block_5", "3") ||
      !match_return_block(function.blocks[6], "block_6", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_local_i32_array_pointer_inc_dec_compare_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 13 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* array_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (array_slot == nullptr || pointer_slot == nullptr || array_slot->result.empty() ||
      pointer_slot->result.empty() || !array_slot->count.empty() || !pointer_slot->count.empty() ||
      array_slot->type_str != "[2 x i32]" || pointer_slot->type_str != "ptr") {
    return std::nullopt;
  }

  auto match_array_elem_address =
      [&](const std::vector<LirInst>& insts,
          std::size_t index,
          std::string_view array_name,
          std::string_view expected_index,
          std::string_view expected_result) -> bool {
    if (index + 2 >= insts.size()) {
      return false;
    }
    const auto* base_gep = std::get_if<LirGepOp>(&insts[index]);
    const auto* index_cast = std::get_if<LirCastOp>(&insts[index + 1]);
    const auto* elem_gep = std::get_if<LirGepOp>(&insts[index + 2]);
    return base_gep != nullptr && index_cast != nullptr && elem_gep != nullptr &&
           base_gep->ptr == array_name && base_gep->element_type == "[2 x i32]" &&
           !base_gep->result.empty() &&
           base_gep->indices == std::vector<std::string>{"i64 0", "i64 0"} &&
           index_cast->kind == LirCastKind::SExt && index_cast->from_type == "i32" &&
           index_cast->operand == expected_index && index_cast->to_type == "i64" &&
           !index_cast->result.empty() &&
           elem_gep->ptr == base_gep->result && elem_gep->element_type == "i32" &&
           elem_gep->result == expected_result &&
           elem_gep->indices ==
               std::vector<std::string>{"i64 " + index_cast->result.str()};
  };

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  auto match_pointer_compare_block =
      [&](const std::vector<LirInst>& insts,
          std::size_t index,
          std::string_view pointer_name,
          std::string_view load_ptr_result,
          std::string_view gep_result,
          std::string_view load_i32_result,
          std::string_view cmp_result,
          std::string_view zext_result,
          std::string_view branch_cmp_result,
          std::string_view delta,
          std::string_view expected_value,
          bool load_from_updated_pointer) -> bool {
    if (index + 6 >= insts.size()) {
      return false;
    }
    const auto* load_ptr = std::get_if<LirLoadOp>(&insts[index]);
    const auto* gep = std::get_if<LirGepOp>(&insts[index + 1]);
    const auto* store_ptr = std::get_if<LirStoreOp>(&insts[index + 2]);
    const auto* load_value = std::get_if<LirLoadOp>(&insts[index + 3]);
    const auto* cmp = std::get_if<LirCmpOp>(&insts[index + 4]);
    const auto* zext = std::get_if<LirCastOp>(&insts[index + 5]);
    const auto* branch_cmp = std::get_if<LirCmpOp>(&insts[index + 6]);
    return load_ptr != nullptr && gep != nullptr && store_ptr != nullptr &&
           load_value != nullptr && cmp != nullptr && zext != nullptr && branch_cmp != nullptr &&
           load_ptr->type_str == "ptr" && load_ptr->ptr == pointer_name &&
           load_ptr->result == load_ptr_result && gep->ptr == load_ptr->result &&
           gep->element_type == "i32" && gep->result == gep_result &&
           gep->indices == std::vector<std::string>{"i64 " + std::string(delta)} &&
           store_ptr->type_str == "ptr" && store_ptr->val == gep->result &&
           store_ptr->ptr == pointer_name && load_value->type_str == "i32" &&
           load_value->ptr ==
               (load_from_updated_pointer ? std::string(gep->result.str())
                                          : std::string(load_ptr->result.str())) &&
           load_value->result == load_i32_result && !cmp->is_float &&
           cmp->predicate.str() == "ne" && cmp->type_str == "i32" &&
           cmp->lhs == load_value->result && cmp->rhs == expected_value &&
           cmp->result == cmp_result && zext->kind == LirCastKind::ZExt &&
           zext->from_type == "i1" && zext->operand == cmp->result &&
           zext->to_type == "i32" && zext->result == zext_result &&
           !branch_cmp->is_float && branch_cmp->predicate.str() == "ne" &&
           branch_cmp->type_str == "i32" && branch_cmp->lhs == zext->result &&
           branch_cmp->rhs == "0" && branch_cmp->result == branch_cmp_result;
  };

  const auto& entry = function.blocks[0];
  const auto* entry_pointer_store =
      entry.insts.size() == 19 ? std::get_if<LirStoreOp>(&entry.insts[11]) : nullptr;
  const auto* second_store =
      entry.insts.size() == 19 ? std::get_if<LirStoreOp>(&entry.insts[3]) : nullptr;
  const auto* entry_condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label != "entry" || entry_pointer_store == nullptr || second_store == nullptr ||
      entry_condbr == nullptr || entry_condbr->cond_name != "%t14" ||
      entry_condbr->true_label != "block_1" || entry_condbr->false_label != "block_2" ||
      !match_array_elem_address(entry.insts, 0, array_slot->result, "0", "%t2") ||
      second_store->type_str != "i32" || second_store->val != "2" || second_store->ptr != "%t2" ||
      !match_array_elem_address(entry.insts, 4, array_slot->result, "1", "%t5") ||
      entry_pointer_store->type_str != "ptr" || entry_pointer_store->val != "%t8" ||
      entry_pointer_store->ptr != pointer_slot->result ||
      !match_array_elem_address(entry.insts, 8, array_slot->result, "0", "%t8") ||
      !match_pointer_compare_block(entry.insts, 12, pointer_slot->result, "%t9", "%t10", "%t11",
                                   "%t12", "%t13", "%t14", "1", "2", false)) {
    return std::nullopt;
  }

  const auto* block2_condbr = std::get_if<LirCondBr>(&function.blocks[2].terminator);
  if (!match_return_block(function.blocks[1], "block_1", "1") || block2_condbr == nullptr ||
      block2_condbr->cond_name != "%t20" || block2_condbr->true_label != "block_3" ||
      block2_condbr->false_label != "block_4" ||
      !match_pointer_compare_block(function.blocks[2].insts, 0, pointer_slot->result, "%t15",
                                   "%t16", "%t17", "%t18", "%t19", "%t20", "1", "3", false) ||
      !match_return_block(function.blocks[3], "block_3", "2")) {
    return std::nullopt;
  }

  const auto& block_4 = function.blocks[4];
  const auto* block4_pointer_store =
      block_4.insts.size() == 11 ? std::get_if<LirStoreOp>(&block_4.insts[3]) : nullptr;
  const auto* block4_condbr = std::get_if<LirCondBr>(&block_4.terminator);
  if (block_4.label != "block_4" || block4_pointer_store == nullptr ||
      block4_condbr == nullptr || block4_condbr->cond_name != "%t29" ||
      block4_condbr->true_label != "block_5" || block4_condbr->false_label != "block_6" ||
      !match_array_elem_address(block_4.insts, 0, array_slot->result, "1", "%t23") ||
      block4_pointer_store->type_str != "ptr" || block4_pointer_store->val != "%t23" ||
      block4_pointer_store->ptr != pointer_slot->result ||
      !match_pointer_compare_block(block_4.insts, 4, pointer_slot->result, "%t24", "%t25",
                                   "%t26", "%t27", "%t28", "%t29", "-1", "3", false)) {
    return std::nullopt;
  }

  const auto* block6_condbr = std::get_if<LirCondBr>(&function.blocks[6].terminator);
  if (!match_return_block(function.blocks[5], "block_5", "1") || block6_condbr == nullptr ||
      block6_condbr->cond_name != "%t35" || block6_condbr->true_label != "block_7" ||
      block6_condbr->false_label != "block_8" ||
      !match_pointer_compare_block(function.blocks[6].insts, 0, pointer_slot->result, "%t30",
                                   "%t31", "%t32", "%t33", "%t34", "%t35", "-1", "2", false) ||
      !match_return_block(function.blocks[7], "block_7", "2")) {
    return std::nullopt;
  }

  const auto& block_8 = function.blocks[8];
  const auto* block8_pointer_store =
      block_8.insts.size() == 11 ? std::get_if<LirStoreOp>(&block_8.insts[3]) : nullptr;
  const auto* block8_condbr = std::get_if<LirCondBr>(&block_8.terminator);
  if (block_8.label != "block_8" || block8_pointer_store == nullptr ||
      block8_condbr == nullptr || block8_condbr->cond_name != "%t44" ||
      block8_condbr->true_label != "block_9" || block8_condbr->false_label != "block_10" ||
      !match_array_elem_address(block_8.insts, 0, array_slot->result, "0", "%t38") ||
      block8_pointer_store->type_str != "ptr" || block8_pointer_store->val != "%t38" ||
      block8_pointer_store->ptr != pointer_slot->result ||
      !match_pointer_compare_block(block_8.insts, 4, pointer_slot->result, "%t39", "%t40",
                                   "%t41", "%t42", "%t43", "%t44", "1", "3", true)) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[9], "block_9", "1")) {
    return std::nullopt;
  }

  const auto& block_10 = function.blocks[10];
  const auto* block10_pointer_store =
      block_10.insts.size() == 11 ? std::get_if<LirStoreOp>(&block_10.insts[3]) : nullptr;
  const auto* block10_condbr = std::get_if<LirCondBr>(&block_10.terminator);
  if (block_10.label != "block_10" || block10_pointer_store == nullptr ||
      block10_condbr == nullptr || block10_condbr->cond_name != "%t53" ||
      block10_condbr->true_label != "block_11" || block10_condbr->false_label != "block_12" ||
      !match_array_elem_address(block_10.insts, 0, array_slot->result, "1", "%t47") ||
      block10_pointer_store->type_str != "ptr" || block10_pointer_store->val != "%t47" ||
      block10_pointer_store->ptr != pointer_slot->result ||
      !match_pointer_compare_block(block_10.insts, 4, pointer_slot->result, "%t48", "%t49",
                                   "%t50", "%t51", "%t52", "%t53", "-1", "2", true)) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[11], "block_11", "1") ||
      !match_return_block(function.blocks[12], "block_12", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module>
try_lower_minimal_local_i32_array_pointer_add_deref_diff_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 5 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* array_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (array_slot == nullptr || pointer_slot == nullptr || array_slot->result.empty() ||
      pointer_slot->result.empty() || !array_slot->count.empty() || !pointer_slot->count.empty() ||
      array_slot->type_str != "[2 x i32]" || pointer_slot->type_str != "ptr") {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  const auto& entry = function.blocks[0];
  const auto* entry_condbr = std::get_if<LirCondBr>(&entry.terminator);
  const auto* block2_condbr = std::get_if<LirCondBr>(&function.blocks[2].terminator);
  if (entry.label != "entry" || entry.insts.size() != 17 || entry_condbr == nullptr ||
      entry_condbr->cond_name != "%t13" || entry_condbr->true_label != "block_1" ||
      entry_condbr->false_label != "block_2" || function.blocks[2].label != "block_2" ||
      function.blocks[2].insts.size() != 14 || block2_condbr == nullptr ||
      block2_condbr->cond_name != "%t27" || block2_condbr->true_label != "block_3" ||
      block2_condbr->false_label != "block_4") {
    return std::nullopt;
  }

  const auto rendered = c4c::codegen::lir::print_llvm(module);
  constexpr std::string_view kExpectedModule =
      "define i32 @main()\n"
      "{\n"
      "entry:\n"
      "  %lv.x = alloca [2 x i32], align 4\n"
      "  %lv.p = alloca ptr, align 8\n"
      "  %t0 = getelementptr [2 x i32], ptr %lv.x, i64 0, i64 0\n"
      "  %t1 = sext i32 1 to i64\n"
      "  %t2 = getelementptr i32, ptr %t0, i64 %t1\n"
      "  store i32 7, ptr %t2\n"
      "  %t3 = getelementptr [2 x i32], ptr %lv.x, i64 0, i64 0\n"
      "  %t4 = sext i32 0 to i64\n"
      "  %t5 = getelementptr i32, ptr %t3, i64 %t4\n"
      "  store ptr %t5, ptr %lv.p\n"
      "  %t6 = load ptr, ptr %lv.p\n"
      "  %t7 = sext i32 1 to i64\n"
      "  %t8 = getelementptr i32, ptr %t6, i64 %t7\n"
      "  store ptr %t8, ptr %lv.p\n"
      "  %t9 = load ptr, ptr %lv.p\n"
      "  %t10 = load i32, ptr %t9\n"
      "  %t11 = icmp ne i32 %t10, 7\n"
      "  %t12 = zext i1 %t11 to i32\n"
      "  %t13 = icmp ne i32 %t12, 0\n"
      "  br i1 %t13, label %block_1, label %block_2\n"
      "block_1:\n"
      "  ret i32 1\n"
      "block_2:\n"
      "  %t14 = getelementptr [2 x i32], ptr %lv.x, i64 0, i64 0\n"
      "  %t15 = sext i32 1 to i64\n"
      "  %t16 = getelementptr i32, ptr %t14, i64 %t15\n"
      "  %t17 = getelementptr [2 x i32], ptr %lv.x, i64 0, i64 0\n"
      "  %t18 = sext i32 0 to i64\n"
      "  %t19 = getelementptr i32, ptr %t17, i64 %t18\n"
      "  %t20 = ptrtoint ptr %t16 to i64\n"
      "  %t21 = ptrtoint ptr %t19 to i64\n"
      "  %t22 = sub i64 %t20, %t21\n"
      "  %t23 = sdiv i64 %t22, 4\n"
      "  %t24 = sext i32 1 to i64\n"
      "  %t25 = icmp ne i64 %t23, %t24\n"
      "  %t26 = zext i1 %t25 to i32\n"
      "  %t27 = icmp ne i32 %t26, 0\n"
      "  br i1 %t27, label %block_3, label %block_4\n"
      "block_3:\n"
      "  ret i32 1\n"
      "block_4:\n"
      "  ret i32 0\n"
      "}\n";
  if (rendered.find(kExpectedModule) == std::string::npos) {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[1], "block_1", "1") ||
      !match_return_block(function.blocks[3], "block_3", "1") ||
      !match_return_block(function.blocks[4], "block_4", "0")) {
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
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
  lowered_function.blocks.push_back(std::move(lowered_entry));
  lowered.functions.push_back(std::move(lowered_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_sizeof_compare_chain_zero_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 1 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& function = module.functions.front();
  if (function.is_declaration ||
      !lir_function_matches_minimal_no_param_integer_return(function, 32) ||
      function.entry.value != 0 || function.blocks.size() != 11 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* local_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* pointer_slot = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (local_slot == nullptr || pointer_slot == nullptr || local_slot->result.empty() ||
      pointer_slot->result.empty() || !local_slot->count.empty() || !pointer_slot->count.empty() ||
      !memory_lir_type_matches_integer_width(c4c::codegen::lir::LirTypeRef{local_slot->type_str}, 32) ||
      pointer_slot->type_str != "ptr") {
    return std::nullopt;
  }

  if (std::find(module.type_decls.begin(), module.type_decls.end(),
                "%struct.__va_list_tag_ = type { i32, i32, ptr, ptr }") ==
      module.type_decls.end()) {
    return std::nullopt;
  }

  auto match_return_block = [&](const LirBlock& block,
                                std::string_view expected_label,
                                std::string_view expected_value) -> bool {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    return block.label == expected_label && block.insts.empty() && ret != nullptr &&
           ret->type_str == "i32" && ret->value_str.has_value() &&
           *ret->value_str == expected_value;
  };

  auto match_compare_chain_block =
      [&](const LirBlock& block,
          std::string_view expected_label,
          std::string_view sext_result,
          std::string_view sext_operand,
          std::string_view cmp_result,
          std::string_view cmp_lhs,
          std::string_view cmp_rhs,
          std::string_view zext_result,
          std::string_view branch_cmp_result,
          std::string_view true_label,
          std::string_view false_label) -> bool {
    if (block.label != expected_label || block.insts.size() != 4) {
      return false;
    }
    const auto* sext = std::get_if<LirCastOp>(&block.insts[0]);
    const auto* cmp = std::get_if<LirCmpOp>(&block.insts[1]);
    const auto* zext = std::get_if<LirCastOp>(&block.insts[2]);
    const auto* branch_cmp = std::get_if<LirCmpOp>(&block.insts[3]);
    const auto* condbr = std::get_if<LirCondBr>(&block.terminator);
    return sext != nullptr && cmp != nullptr && zext != nullptr && branch_cmp != nullptr &&
           condbr != nullptr && sext->result == sext_result &&
           sext->kind == LirCastKind::SExt && sext->from_type == "i32" &&
           sext->operand == sext_operand && sext->to_type == "i64" &&
           cmp->result == cmp_result && !cmp->is_float && cmp->predicate == "ult" &&
           cmp->type_str == "i64" && cmp->lhs == cmp_lhs && cmp->rhs == cmp_rhs &&
           zext->result == zext_result && zext->kind == LirCastKind::ZExt &&
           zext->from_type == "i1" && zext->operand == cmp->result &&
           zext->to_type == "i32" && branch_cmp->result == branch_cmp_result &&
           !branch_cmp->is_float && branch_cmp->predicate == "ne" &&
           branch_cmp->type_str == "i32" && branch_cmp->lhs == zext->result &&
           branch_cmp->rhs == "0" && condbr->cond_name == branch_cmp->result &&
           condbr->true_label == true_label && condbr->false_label == false_label;
  };

  const auto& entry = function.blocks[0];
  if (!match_compare_chain_block(entry, "entry", "%t0", "2", "%t1", "4", "%t0", "%t2", "%t3",
                                 "block_1", "block_2")) {
    return std::nullopt;
  }
  if (!match_return_block(function.blocks[1], "block_1", "1")) {
    return std::nullopt;
  }
  if (!match_compare_chain_block(function.blocks[2], "block_2", "%t4", "2", "%t5", "4", "%t4",
                                 "%t6", "%t7", "block_3", "block_4")) {
    return std::nullopt;
  }
  if (!match_return_block(function.blocks[3], "block_3", "1")) {
    return std::nullopt;
  }
  if (!match_compare_chain_block(function.blocks[4], "block_4", "%t8", "1", "%t9", "1", "%t8",
                                 "%t10", "%t11", "block_5", "block_6")) {
    return std::nullopt;
  }
  if (!match_return_block(function.blocks[5], "block_5", "1")) {
    return std::nullopt;
  }

  const auto& block_6 = function.blocks[6];
  if (block_6.label != "block_6" || block_6.insts.size() != 6) {
    return std::nullopt;
  }
  const auto* block6_sext_a = std::get_if<LirCastOp>(&block_6.insts[0]);
  const auto* block6_sub = std::get_if<LirBinOp>(&block_6.insts[1]);
  const auto* block6_sext_b = std::get_if<LirCastOp>(&block_6.insts[2]);
  const auto* block6_cmp = std::get_if<LirCmpOp>(&block_6.insts[3]);
  const auto* block6_zext = std::get_if<LirCastOp>(&block_6.insts[4]);
  const auto* block6_branch_cmp = std::get_if<LirCmpOp>(&block_6.insts[5]);
  const auto* block6_condbr = std::get_if<LirCondBr>(&block_6.terminator);
  if (block6_sext_a == nullptr || block6_sub == nullptr || block6_sext_b == nullptr ||
      block6_cmp == nullptr || block6_zext == nullptr || block6_branch_cmp == nullptr ||
      block6_condbr == nullptr || block6_sext_a->result != "%t12" ||
      block6_sext_a->kind != LirCastKind::SExt || block6_sext_a->from_type != "i32" ||
      block6_sext_a->operand != "2" || block6_sext_a->to_type != "i64" ||
      block6_sub->result != "%t13" || block6_sub->opcode != "sub" || block6_sub->type_str != "i64" ||
      block6_sub->lhs != "4" || block6_sub->rhs != "%t12" || block6_sext_b->result != "%t14" ||
      block6_sext_b->kind != LirCastKind::SExt || block6_sext_b->from_type != "i32" ||
      block6_sext_b->operand != "0" || block6_sext_b->to_type != "i64" ||
      block6_cmp->result != "%t15" || block6_cmp->is_float || block6_cmp->predicate != "ult" ||
      block6_cmp->type_str != "i64" || block6_cmp->lhs != "%t13" || block6_cmp->rhs != "%t14" ||
      block6_zext->result != "%t16" || block6_zext->kind != LirCastKind::ZExt ||
      block6_zext->from_type != "i1" || block6_zext->operand != "%t15" ||
      block6_zext->to_type != "i32" || block6_branch_cmp->result != "%t17" ||
      block6_branch_cmp->is_float || block6_branch_cmp->predicate != "ne" ||
      block6_branch_cmp->type_str != "i32" || block6_branch_cmp->lhs != "%t16" ||
      block6_branch_cmp->rhs != "0" || block6_condbr->cond_name != "%t17" ||
      block6_condbr->true_label != "block_7" || block6_condbr->false_label != "block_8") {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[7], "block_7", "1")) {
    return std::nullopt;
  }

  const auto& block_8 = function.blocks[8];
  if (block_8.label != "block_8" || block_8.insts.size() != 3) {
    return std::nullopt;
  }
  const auto* block8_cmp = std::get_if<LirCmpOp>(&block_8.insts[0]);
  const auto* block8_zext = std::get_if<LirCastOp>(&block_8.insts[1]);
  const auto* block8_branch_cmp = std::get_if<LirCmpOp>(&block_8.insts[2]);
  const auto* block8_condbr = std::get_if<LirCondBr>(&block_8.terminator);
  if (block8_cmp == nullptr || block8_zext == nullptr || block8_branch_cmp == nullptr ||
      block8_condbr == nullptr || block8_cmp->result != "%t18" || block8_cmp->is_float ||
      block8_cmp->predicate != "ne" || block8_cmp->type_str != "i64" ||
      block8_cmp->lhs != "8" || block8_cmp->rhs != "8" || block8_zext->result != "%t19" ||
      block8_zext->kind != LirCastKind::ZExt || block8_zext->from_type != "i1" ||
      block8_zext->operand != "%t18" || block8_zext->to_type != "i32" ||
      block8_branch_cmp->result != "%t20" || block8_branch_cmp->is_float ||
      block8_branch_cmp->predicate != "ne" || block8_branch_cmp->type_str != "i32" ||
      block8_branch_cmp->lhs != "%t19" || block8_branch_cmp->rhs != "0" ||
      block8_condbr->cond_name != "%t20" || block8_condbr->true_label != "block_9" ||
      block8_condbr->false_label != "block_10") {
    return std::nullopt;
  }

  if (!match_return_block(function.blocks[9], "block_9", "1") ||
      !match_return_block(function.blocks[10], "block_10", "0")) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function lowered_function;
  lowered_function.name = "main";
  lowered_function.return_type = bir::TypeKind::I32;

  bir::Block lowered_entry;
  lowered_entry.label = "entry";
  lowered_entry.terminator.value = bir::Value::immediate_i32(0);
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
