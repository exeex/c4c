#include "lir_to_bir.hpp"

#include "../../codegen/lir/ir.hpp"
#include "../../codegen/shared/llvm_helpers.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace c4c::backend::bir {
namespace {

using codegen::lir::LirBlock;
using codegen::lir::LirBinOp;
using codegen::lir::LirAbsOp;
using codegen::lir::LirAllocaOp;
using codegen::lir::LirBr;
using codegen::lir::LirCallOp;
using codegen::lir::LirCastOp;
using codegen::lir::LirCmpOp;
using codegen::lir::LirCondBr;
using codegen::lir::LirConstFloat;
using codegen::lir::LirConstInt;
using codegen::lir::LirExtAttr;
using codegen::lir::LirExternDecl;
using codegen::lir::LirFunction;
using codegen::lir::LirGepOp;
using codegen::lir::LirGlobal;
using codegen::lir::LirIndirectBr;
using codegen::lir::LirIndirectBrOp;
using codegen::lir::LirInlineAsmOp;
using codegen::lir::LirInlineAsmValueBinding;
using codegen::lir::LirInlineAsmValueRole;
using codegen::lir::LirIntrinsicKind;
using codegen::lir::LirModule;
using codegen::lir::LirLoadOp;
using codegen::lir::LirMemcpyOp;
using codegen::lir::LirPhiOp;
using codegen::lir::LirRet;
using codegen::lir::LirSelectOp;
using codegen::lir::LirStructDecl;
using codegen::lir::LirStoreOp;
using codegen::lir::LirSwitch;
using codegen::lir::LirUnreachable;

// Map native LIR terminator occurrence authority to Raw-BIR's ordinal among
// only the predecessor's successors that reach the PHI destination. This is
// intentionally independent of PHI input order and display labels.
std::optional<std::uint32_t> phi_edge_occurrence(
    const codegen::lir::LirTerminator& terminator,
    codegen::lir::LirBlockId destination,
    codegen::lir::LirSuccessorOccurrenceId selected) {
  if (!selected.valid()) return std::nullopt;
  std::vector<codegen::lir::LirBlockId> successors;
  if (const auto* branch = std::get_if<LirBr>(&terminator)) {
    if (!(selected == codegen::lir::LirSuccessorOccurrenceId::direct_branch())) return std::nullopt;
    successors = {branch->successor};
  } else if (const auto* branch = std::get_if<LirCondBr>(&terminator)) {
    if (selected.value > codegen::lir::LirSuccessorOccurrenceId::conditional_false().value) return std::nullopt;
    successors = {branch->true_successor, branch->false_successor};
  } else if (const auto* sw = std::get_if<LirSwitch>(&terminator)) {
    if (selected.value > sw->case_successors.size()) return std::nullopt;
    successors.reserve(sw->case_successors.size() + 1);
    successors.push_back(sw->default_successor);
    successors.insert(successors.end(), sw->case_successors.begin(), sw->case_successors.end());
  } else {
    return std::nullopt;
  }
  if (!(successors[selected.value] == destination)) return std::nullopt;
  return static_cast<std::uint32_t>(std::count(
      successors.begin(), successors.begin() + selected.value, destination));
}

template <class T>
Result<T, ImportError> fail(ImportErrorCode code, std::string function = {},
                            std::string block = {}, std::string detail = {}) {
  return Result<T, ImportError>::failure(
      {code, std::move(function), std::move(block), std::move(detail)});
}

bool aggregate_fields_match(
    const std::vector<codegen::lir::LirStructField>& lhs,
    const std::vector<codegen::lir::LirStructField>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index].type.str() != rhs[index].type.str()) return false;
  }
  return true;
}

bool aggregate_store_entry_matches_decl(
    const codegen::lir::LirAggregateStoreEntry& entry,
    const LirStructDecl& decl) {
  if (entry.name_id != decl.name_id || entry.is_packed != decl.is_packed ||
      entry.is_opaque != decl.is_opaque ||
      !aggregate_fields_match(entry.fields, decl.fields)) {
    return false;
  }
  return entry.layout_kind == codegen::lir::LirAggregateLayoutKind::Union
             ? entry.is_union
             : !entry.is_union;
}

std::optional<std::vector<const LirStructDecl*>>
authoritative_struct_decls(const LirModule& module) {
  std::vector<const LirStructDecl*> declarations;
  if (module.aggregate_store.empty()) {
    declarations.reserve(module.struct_decls.size());
    for (const auto& declaration : module.struct_decls) {
      declarations.push_back(&declaration);
    }
    return declarations;
  }

  declarations.reserve(module.aggregate_store.size());
  for (const auto& entry : module.aggregate_store) {
    const LirStructDecl* declaration = module.find_struct_decl(entry.name_id);
    if (declaration == nullptr ||
        module.struct_names.spelling(entry.name_id).empty() ||
        !aggregate_store_entry_matches_decl(entry, *declaration)) {
      return std::nullopt;
    }
    declarations.push_back(declaration);
  }
  return declarations;
}

std::string function_link_name(const LirModule& module,
                               const LirFunction& function) {
  if (function.link_name_id != kInvalidLinkName)
    return std::string(module.link_names.spelling(function.link_name_id));
  return function.name;
}

std::optional<Type> lower_lir_type(const LirModule& module,
                                  const codegen::lir::LirTypeRef& type) {
  using codegen::lir::LirTypeKind;
  const auto classified = codegen::lir::LirTypeRef(type.str()).kind();
  switch (type.kind()) {
    case LirTypeKind::Void:
      if (type.str() == "void") return Type{TypeKind::Void, 0, "void"};
      break;
    case LirTypeKind::Integer:
      if (classified == LirTypeKind::Integer && type.integer_bit_width() &&
          *type.integer_bit_width() != 0)
        return Type{TypeKind::Integer, *type.integer_bit_width(), type.str()};
      break;
    case LirTypeKind::Floating: {
      std::uint32_t width = 0;
      if (type.str() == "half") width = 16;
      else if (type.str() == "float") width = 32;
      else if (type.str() == "double") width = 64;
      else if (type.str() == "x86_fp80") width = 80;
      else if (type.str() == "fp128") width = 128;
      if (width != 0)
        return Type{TypeKind::Floating, width, type.str()};
      break;
    }
    case LirTypeKind::Pointer:
      if (type.str() == "ptr") return Type{TypeKind::Pointer};
      break;
    case LirTypeKind::Vector:
      if (classified == LirTypeKind::Vector && type.str().back() == '>')
        return Type{TypeKind::Vector, 0, type.str()};
      break;
    case LirTypeKind::VrmRegister:
      if (classified == LirTypeKind::VrmRegister && type.vrm_width())
        return Type{TypeKind::VrmRegister, *type.vrm_width(), type.str()};
      break;
    case LirTypeKind::Array:
      if (classified == LirTypeKind::Array && type.str().back() == ']')
        return Type{TypeKind::Array, 0, type.str()};
      break;
    case LirTypeKind::Struct:
      if (type.has_struct_name_id() &&
          !module.struct_names.spelling(type.struct_name_id()).empty() &&
          module.struct_names.spelling(type.struct_name_id()) == type.str())
        return Type{TypeKind::Struct, 0, type.str(), type.struct_name_id()};
      if (!type.has_struct_name_id() && classified == LirTypeKind::Struct &&
          type.str().front() == '{' && type.str().back() == '}')
        return Type{TypeKind::Struct, 0, type.str()};
      break;
    case LirTypeKind::Function:
      if (classified == LirTypeKind::Function)
        return Type{TypeKind::Function, 0, type.str()};
      break;
    case LirTypeKind::Opaque:
      if (classified == LirTypeKind::Opaque)
        return Type{TypeKind::Opaque, 0, type.str()};
      break;
    case LirTypeKind::RawText: break;
  }
  return std::nullopt;
}

bool integer_immediate_representable(long long value,
                                     unsigned bit_width) noexcept {
  if (bit_width == 0) return false;
  if (bit_width >= 64) return true;
  if (value < 0)
    return value >= -(1LL << (bit_width - 1));
  return static_cast<unsigned long long>(value) <=
         ((1ULL << bit_width) - 1ULL);
}

bool is_integer_type(const Type& type) noexcept {
  switch (type.kind) {
    case TypeKind::I1:
    case TypeKind::I8:
    case TypeKind::I16:
    case TypeKind::I32:
    case TypeKind::I64:
    case TypeKind::Integer: return true;
    default: return false;
  }
}

bool is_local_scalar_load_type(const Type& type) noexcept {
  if (is_integer_type(type)) return true;
  switch (type.kind) {
    case TypeKind::F32:
    case TypeKind::F64:
    case TypeKind::Floating: return true;
    default: return false;
  }
}

std::optional<Type> lower_constant_type(const LirModule& module,
                                        const TypeSpec& type);

std::optional<Type> lower_signature_type(
    const LirModule& module, const TypeSpec& structured,
    const std::optional<codegen::lir::LirTypeRef>& mirror) {
  const auto compatibility_array_fact = [](long long value) {
    return value == -1 || value == 0;
  };
  if (structured.ptr_level != 0 || structured.is_lvalue_ref ||
      structured.is_rvalue_ref || structured.array_rank != 0 ||
      !compatibility_array_fact(structured.array_size) ||
      std::any_of(std::begin(structured.array_dims),
                  std::end(structured.array_dims),
                  [&](long long dimension) {
                    return !compatibility_array_fact(dimension);
                  }) ||
      structured.is_ptr_to_array ||
      (structured.inner_rank != -1 && structured.inner_rank != 0) ||
      structured.is_fn_ptr || structured.is_vector ||
      structured.vector_lanes != 0 || structured.vector_bytes != 0 ||
      structured.array_size_expr != nullptr ||
      (structured.base != TB_ENUM &&
       structured.enum_underlying_base != TB_VOID))
    return std::nullopt;

  TypeSpec normalized = structured;
  normalized.inner_rank = 0;

  if (normalized.base != TB_VOID) {
    switch (normalized.base) {
      case TB_BOOL:
      case TB_CHAR:
      case TB_UCHAR:
      case TB_SCHAR:
      case TB_SHORT:
      case TB_USHORT:
      case TB_INT:
      case TB_UINT:
      case TB_LONG:
      case TB_ULONG:
      case TB_LONGLONG:
      case TB_ULONGLONG:
      case TB_INT128:
      case TB_UINT128:
      case TB_ENUM:
      case TB_FLOAT:
      case TB_DOUBLE:
      case TB_LONGDOUBLE: break;
      default: return std::nullopt;
    }
    const auto result = lower_constant_type(module, normalized);
    if (!result || (result->kind != TypeKind::Integer &&
                    result->kind != TypeKind::Floating) ||
        !is_well_formed(*result))
      return std::nullopt;
    if (mirror) {
      const auto mirrored = lower_lir_type(module, *mirror);
      if (!mirrored || mirrored->kind != result->kind ||
          mirrored->bit_width != result->bit_width ||
          mirrored->spelling != result->spelling ||
          !is_well_formed(*mirrored))
        return std::nullopt;
    }
    return result;
  }

  if (normalized.enum_underlying_base != TB_VOID ||
      normalized.vrm_width != 0)
    return std::nullopt;

  StructuredTypeSpecFacts facts;
  facts.pointer_level = normalized.ptr_level;
  facts.is_lvalue_reference = normalized.is_lvalue_ref;
  facts.is_rvalue_reference = normalized.is_rvalue_ref;
  facts.array_rank = normalized.array_rank;
  facts.is_pointer_to_array = normalized.is_ptr_to_array;
  facts.inner_array_rank = normalized.inner_rank;
  facts.is_function_pointer = normalized.is_fn_ptr;

  Type result{TypeKind::Void, 0, "void"};
  result.structured_spec = facts;
  if (!is_well_formed(result)) return std::nullopt;
  if (mirror) {
    const auto mirrored = lower_lir_type(module, *mirror);
    if (!mirrored || mirrored->kind != TypeKind::Void) return std::nullopt;
  }
  return result;
}

bool default_parameter_type_metadata(const TypeSpec& type) {
  const auto compatibility_array_fact = [](long long value) {
    return value == -1 || value == 0;
  };
  return type.enum_underlying_base == TB_VOID && type.ptr_level == 0 &&
         !type.is_lvalue_ref && !type.is_rvalue_ref && type.align_bytes == 0 &&
         compatibility_array_fact(type.array_size) && type.array_rank == 0 &&
         std::all_of(std::begin(type.array_dims), std::end(type.array_dims),
                     compatibility_array_fact) &&
         !type.is_ptr_to_array &&
         (type.inner_rank == -1 || type.inner_rank == 0) && !type.is_vector &&
         type.vector_lanes == 0 && type.vector_bytes == 0 &&
         type.vrm_width == 0 && type.array_size_expr == nullptr &&
         !type.is_const && !type.is_volatile && !type.is_fn_ptr &&
         !type.is_packed && !type.is_noinline && !type.is_always_inline &&
         type.tag_text_id == kInvalidText &&
         type.template_param_owner_namespace_context_id == -1 &&
         type.template_param_owner_text_id == kInvalidText &&
         type.template_param_index == -1 &&
         type.template_param_text_id == kInvalidText &&
         type.record_def == nullptr && type.qualifier_segments == nullptr &&
         type.qualifier_text_ids == nullptr &&
         type.n_qualifier_segments == 0 && !type.is_global_qualified &&
         type.namespace_context_id == -1 && type.tpl_struct_origin == nullptr &&
         type.tpl_struct_origin_key == c4c::QualifiedNameKey{} &&
         type.tpl_struct_args.data == nullptr && type.tpl_struct_args.size == 0 &&
         type.deferred_member_type_owner_key == c4c::QualifiedNameKey{} &&
         type.deferred_member_type_name == nullptr &&
         type.deferred_member_type_text_id == kInvalidText;
}

bool supported_plain_parameter_base(TypeBase base) {
  switch (base) {
    case TB_INT:
    case TB_UINT:
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG:
    case TB_FLOAT:
    case TB_DOUBLE: return true;
    default: return false;
  }
}

bool selected_direct_pointer_body_parameter(const LirFunction& function,
                                            std::size_t index,
                                            const codegen::lir::LirTypeRef& mirror) {
  return std::count_if(function.native_body_parameter_definitions.begin(),
                       function.native_body_parameter_definitions.end(),
                       [&](const auto& definition) {
                         return definition.parameter_index == index &&
                                definition.type == mirror &&
                                definition.owner == function.link_name_id &&
                                definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
                       }) == 1;
}

bool same_default_parameter_type(const TypeSpec& lhs, const TypeSpec& rhs) {
  return lhs.base == rhs.base &&
         lhs.enum_underlying_base == rhs.enum_underlying_base &&
         lhs.ptr_level == rhs.ptr_level &&
         lhs.is_lvalue_ref == rhs.is_lvalue_ref &&
         lhs.is_rvalue_ref == rhs.is_rvalue_ref &&
         lhs.align_bytes == rhs.align_bytes &&
         lhs.array_size == rhs.array_size &&
         lhs.array_rank == rhs.array_rank &&
         std::equal(std::begin(lhs.array_dims), std::end(lhs.array_dims),
                    std::begin(rhs.array_dims)) &&
         lhs.is_ptr_to_array == rhs.is_ptr_to_array &&
         lhs.inner_rank == rhs.inner_rank && lhs.is_const == rhs.is_const &&
         lhs.is_volatile == rhs.is_volatile;
}

std::optional<std::vector<Type>> lower_function_parameter_types(
    const LirModule& module, const LirFunction& function) {
  if (function.signature_is_variadic) return std::nullopt;
  if (function.signature_has_void_param_list) {
    if (function.params.size() != 1 ||
        function.params.front().second.base != TB_VOID ||
        !default_parameter_type_metadata(function.params.front().second) ||
        !function.signature_params.empty() ||
        !function.signature_param_type_refs.empty())
      return std::nullopt;
    return std::vector<Type>{};
  }
  if (function.params.empty() && function.signature_params.empty() &&
      function.signature_param_type_refs.empty())
    return std::vector<Type>{};
  if (function.params.empty() ||
      function.params.size() != function.signature_params.size() ||
      function.params.size() != function.signature_param_type_refs.size())
    return std::nullopt;

  std::vector<Type> lowered;
  lowered.reserve(function.params.size());
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    const auto& logical = function.params[index].second;
    const auto& signature = function.signature_params[index];
    const auto& mirror = function.signature_param_type_refs[index];
    const bool selected_pointer =
        mirror.kind() == codegen::lir::LirTypeKind::Pointer &&
        logical.ptr_level > 0 && signature.type.ptr_level > 0 &&
        !signature.is_byval &&
        same_default_parameter_type(logical, signature.type) &&
        selected_direct_pointer_body_parameter(function, index, mirror);
    if ((!selected_pointer &&
         (!supported_plain_parameter_base(logical.base) ||
          !supported_plain_parameter_base(signature.type.base) ||
          !default_parameter_type_metadata(logical) ||
          !default_parameter_type_metadata(signature.type) || signature.is_byval ||
          !same_default_parameter_type(logical, signature.type) ||
          mirror.has_struct_name_id())) ||
        (selected_pointer && mirror.has_struct_name_id()))
      return std::nullopt;
    const auto type = selected_pointer
        ? std::optional<Type>{Type{TypeKind::Pointer}}
        : lower_signature_type(module, signature.type, mirror);
    if (!type) return std::nullopt;
    if ((type->kind == TypeKind::Pointer && !selected_pointer) ||
        (type->kind == TypeKind::Integer &&
         (mirror.kind() != codegen::lir::LirTypeKind::Integer ||
          !mirror.integer_bit_width() ||
          *mirror.integer_bit_width() != type->bit_width)) ||
        (type->kind == TypeKind::Floating &&
         mirror.kind() != codegen::lir::LirTypeKind::Floating) ||
        (type->kind != TypeKind::Pointer && mirror.str() != type->spelling))
      return std::nullopt;
    lowered.push_back(*type);
  }
  return lowered;
}

std::optional<std::vector<Type>> lower_store_backed_function_parameter_types(
    const LirModule& module,
    const codegen::lir::LirFunctionSignatureStoreEntry& signature) {
  if (signature.fixed_param_is_byval.size() !=
      signature.fixed_param_type_refs.size())
    return std::nullopt;
  const bool has_byval =
      std::any_of(signature.fixed_param_is_byval.begin(),
                  signature.fixed_param_is_byval.end(),
                  [](bool is_byval) { return is_byval; });
  if (!has_byval) return std::nullopt;
  std::vector<Type> lowered;
  lowered.reserve(signature.fixed_param_type_refs.size());
  for (std::size_t index = 0; index < signature.fixed_param_type_refs.size();
       ++index) {
    if (signature.fixed_param_is_byval[index]) {
      if (!signature.fixed_param_type_refs[index].has_struct_name_id()) {
        return std::nullopt;
      }
      lowered.push_back(Type{TypeKind::Pointer});
      continue;
    }
    return std::nullopt;
  }
  return lowered;
}

bool variadic_declaration_signature_store_matches(
    const LirModule& module, const LirFunction& function) {
  if (!function.is_declaration || !function.signature_is_variadic ||
      function.signature_has_void_param_list)
    return false;
  const auto* stored =
      module.find_function_signature(function.function_signature_ref);
  if (stored == nullptr || !stored->is_variadic ||
      stored->has_void_param_list != function.signature_has_void_param_list ||
      stored->return_ext_attr != function.signature_return_ext_attr ||
      stored->return_type_ref.has_value() !=
          function.signature_return_type_ref.has_value() ||
      (stored->return_type_ref.has_value() &&
       *stored->return_type_ref != *function.signature_return_type_ref) ||
      stored->fixed_param_type_refs != function.signature_param_type_refs ||
      stored->fixed_param_is_byval.size() != function.signature_params.size())
    return false;
  for (std::size_t index = 0; index < function.signature_params.size();
       ++index) {
    if (stored->fixed_param_is_byval[index] !=
        function.signature_params[index].is_byval)
      return false;
  }
  LirFunction fixed_prefix = function;
  fixed_prefix.signature_is_variadic = false;
  return lower_function_parameter_types(module, fixed_prefix).has_value();
}

bool byval_declaration_signature_store_matches(
    const LirModule& module, const LirFunction& function) {
  if (!function.is_declaration || function.signature_is_variadic ||
      function.signature_has_void_param_list)
    return false;
  const auto* stored =
      module.find_function_signature(function.function_signature_ref);
  if (stored == nullptr || stored->is_variadic ||
      stored->has_void_param_list != function.signature_has_void_param_list ||
      stored->return_ext_attr != function.signature_return_ext_attr ||
      stored->return_type_ref.has_value() !=
          function.signature_return_type_ref.has_value() ||
      (stored->return_type_ref.has_value() &&
       *stored->return_type_ref != *function.signature_return_type_ref) ||
      stored->fixed_param_type_refs != function.signature_param_type_refs ||
      stored->fixed_param_is_byval.size() != function.signature_params.size())
    return false;
  bool has_byval = false;
  for (std::size_t index = 0; index < function.signature_params.size();
       ++index) {
    if (stored->fixed_param_is_byval[index] !=
        function.signature_params[index].is_byval)
      return false;
    has_byval = has_byval || stored->fixed_param_is_byval[index];
  }
  return has_byval;
}

const codegen::lir::LirFunctionSignatureStoreEntry* resolved_call_signature(
    const LirModule& module, const LirCallOp& call) {
  if (!call.callee_signature_ref.valid()) return nullptr;
  return module.find_function_signature(call.callee_signature_ref);
}

bool retained_signature_matches_store(
    const codegen::lir::LirCallSignature& retained,
    const codegen::lir::LirFunctionSignatureStoreEntry& stored) {
  return retained.return_type_ref.has_value() ==
             stored.return_type_ref.has_value() &&
         (!retained.return_type_ref.has_value() ||
          *retained.return_type_ref == *stored.return_type_ref) &&
         retained.return_ext_attr == stored.return_ext_attr &&
         retained.fixed_param_type_refs == stored.fixed_param_type_refs &&
         retained.is_variadic == stored.is_variadic &&
         retained.has_void_param_list == stored.has_void_param_list &&
         !retained.has_unspecified_params;
}

const std::vector<codegen::lir::LirTypeRef>* call_fixed_param_type_refs(
    const LirModule& module, const LirCallOp& call) {
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr) {
    if (call.callee_signature.has_value() &&
        !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
      return nullptr;
    }
    if (stored_signature->is_variadic ||
        stored_signature->has_void_param_list) {
      return nullptr;
    }
    return &stored_signature->fixed_param_type_refs;
  }
  if (!call.callee_signature || call.callee_signature->is_variadic ||
      call.callee_signature->has_unspecified_params ||
      call.callee_signature->has_void_param_list) {
    return nullptr;
  }
  return &call.callee_signature->fixed_param_type_refs;
}

bool exact_direct_void_call(const LirModule& module, const LirCallOp& call) {
  if (!call.result.empty() || call.result.has_authority() ||
      call.return_type.kind() != codegen::lir::LirTypeKind::Void ||
      call.return_type.str() != "void" ||
      call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      !call.structured_args.empty() || !call.arg_type_refs.empty())
    return false;

  const auto return_type = lower_lir_type(module, call.return_type);
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr && call.callee_signature.has_value() &&
      !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
    return false;
  }
  if (stored_signature == nullptr && !call.callee_signature.has_value()) return false;

  const auto return_ref = stored_signature != nullptr
                              ? stored_signature->return_type_ref
                              : call.callee_signature->return_type_ref;
  const auto return_ext_attr = stored_signature != nullptr
                                   ? stored_signature->return_ext_attr
                                   : call.callee_signature->return_ext_attr;
  const auto is_variadic = stored_signature != nullptr
                               ? stored_signature->is_variadic
                               : call.callee_signature->is_variadic;
  const auto has_unspecified_params =
      stored_signature == nullptr && call.callee_signature->has_unspecified_params;
  const auto has_void_param_list = stored_signature != nullptr
                                       ? stored_signature->has_void_param_list
                                       : call.callee_signature->has_void_param_list;
  const auto& fixed_param_type_refs = stored_signature != nullptr
                                          ? stored_signature->fixed_param_type_refs
                                          : call.callee_signature->fixed_param_type_refs;
  if (!return_type || return_type->kind != TypeKind::Void ||
      !return_ref || return_ref->kind() != codegen::lir::LirTypeKind::Void ||
      return_ref->str() != "void" || *return_ref != call.return_type ||
      return_ext_attr != LirExtAttr::None || !fixed_param_type_refs.empty() ||
      is_variadic || has_unspecified_params || !has_void_param_list)
    return false;
  if (stored_signature == nullptr && !call.callee_signature->fixed_param_types.empty())
    return false;

  bool resolved = false;
  for (const auto& target : module.functions) {
    if (target.link_name_id != call.direct_callee_link_name_id) continue;
    const auto target_return = lower_signature_type(
        module, target.return_type, target.signature_return_type_ref);
    const auto target_params = lower_function_parameter_types(module, target);
    if (!target_return || target_return->kind != TypeKind::Void ||
        !target_params || !target_params->empty() ||
        target.signature_is_variadic)
      return false;
    resolved = true;
  }
  return resolved;
}

bool exact_direct_integer_call(
    const LirModule& module, const LirCallOp& call,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  const auto* result = call.result.value_id();
  if (call.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || call.return_type.kind() != codegen::lir::LirTypeKind::Integer ||
      call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName)
    return false;
  const auto return_type = lower_lir_type(module, call.return_type);
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr && call.callee_signature.has_value() &&
      !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
    return false;
  }
  if (stored_signature == nullptr && !call.callee_signature.has_value()) return false;

  const auto return_ref = stored_signature != nullptr
                              ? stored_signature->return_type_ref
                              : call.callee_signature->return_type_ref;
  const auto return_ext_attr = stored_signature != nullptr
                                   ? stored_signature->return_ext_attr
                                   : call.callee_signature->return_ext_attr;
  const auto is_variadic = stored_signature != nullptr
                               ? stored_signature->is_variadic
                               : call.callee_signature->is_variadic;
  const auto has_unspecified_params =
      stored_signature == nullptr && call.callee_signature->has_unspecified_params;
  const auto has_void_param_list = stored_signature != nullptr
                                       ? stored_signature->has_void_param_list
                                       : call.callee_signature->has_void_param_list;
  const auto& fixed_param_type_refs = stored_signature != nullptr
                                          ? stored_signature->fixed_param_type_refs
                                          : call.callee_signature->fixed_param_type_refs;
  const auto* fixed_param_is_byval =
      stored_signature != nullptr ? &stored_signature->fixed_param_is_byval : nullptr;
  if (stored_signature == nullptr &&
      call.callee_signature->fixed_param_types.size() !=
          fixed_param_type_refs.size())
    return false;
  if (fixed_param_is_byval != nullptr &&
      fixed_param_is_byval->size() != fixed_param_type_refs.size())
    return false;
  if (!return_type || !is_integer_type(*return_type) ||
      !return_ref || *return_ref != call.return_type ||
      return_ext_attr != LirExtAttr::None || is_variadic ||
      has_unspecified_params || has_void_param_list ||
      fixed_param_type_refs.size() != call.structured_args.size())
    return false;
  std::vector<Type> parameter_types;
  parameter_types.reserve(fixed_param_type_refs.size());
  for (std::size_t index = 0; index < call.structured_args.size(); ++index) {
    const auto& parameter_ref = fixed_param_type_refs[index];
    const auto& argument = call.structured_args[index];
    const bool parameter_is_byval =
        fixed_param_is_byval != nullptr && (*fixed_param_is_byval)[index];
    if (parameter_is_byval) {
      const auto* value_id = argument.operand.value_id();
      const auto found = value_id ? source_values.find(value_id->value)
                                  : source_values.end();
      const bool current_pointer =
          argument.operand.kind() == codegen::lir::LirOperandKind::SsaValue &&
          value_id && value_id->valid() && found != source_values.end() &&
          found->second.kind == TypeKind::Pointer;
      const bool global_pointer =
          argument.operand.kind() == codegen::lir::LirOperandKind::Global;
      if (!parameter_ref.has_struct_name_id() ||
          argument.type_ref != parameter_ref ||
          argument.ext_attr != LirExtAttr::None ||
          (!current_pointer && !global_pointer))
        return false;
      parameter_types.push_back(Type{TypeKind::Pointer});
      continue;
    }
    const auto parameter_type = lower_lir_type(module, parameter_ref);
    if (!parameter_type || !is_integer_type(*parameter_type) ||
        (stored_signature == nullptr &&
         call.callee_signature->fixed_param_types[index] != parameter_ref.str()) ||
        argument.type != parameter_ref.str() || argument.type_ref != parameter_ref ||
        argument.ext_attr != LirExtAttr::None)
      return false;
    if (argument.operand.kind() == codegen::lir::LirOperandKind::Immediate) {
      const auto* immediate = argument.operand.integer_immediate();
      if (!immediate || !parameter_ref.integer_bit_width() ||
          !integer_immediate_representable(immediate->value,
                                           *parameter_ref.integer_bit_width()))
        return false;
    } else if (argument.operand.kind() == codegen::lir::LirOperandKind::SsaValue) {
      const auto* value_id = argument.operand.value_id();
      const auto found = value_id ? source_values.find(value_id->value)
                                  : source_values.end();
      if (!value_id || !value_id->valid() || found == source_values.end() ||
          found->second != *parameter_type)
        return false;
    } else {
      return false;
    }
    parameter_types.push_back(*parameter_type);
  }
  bool resolved = false;
  for (const auto& target : module.functions) {
    if (target.link_name_id != call.direct_callee_link_name_id) continue;
    const auto target_return = lower_signature_type(
        module, target.return_type, target.signature_return_type_ref);
    auto target_params = lower_function_parameter_types(module, target);
    if (!target_params && stored_signature != nullptr &&
        target.function_signature_ref.value == call.callee_signature_ref.value &&
        stored_signature->fixed_param_is_byval.size() ==
            stored_signature->fixed_param_type_refs.size()) {
      std::vector<Type> store_params;
      store_params.reserve(stored_signature->fixed_param_type_refs.size());
      bool supported = true;
      for (std::size_t index = 0;
           index < stored_signature->fixed_param_type_refs.size(); ++index) {
        if (stored_signature->fixed_param_is_byval[index] &&
            stored_signature->fixed_param_type_refs[index].has_struct_name_id()) {
          store_params.push_back(Type{TypeKind::Pointer});
          continue;
        }
        const auto lowered =
            lower_lir_type(module, stored_signature->fixed_param_type_refs[index]);
        if (!lowered) {
          supported = false;
          break;
        }
        store_params.push_back(*lowered);
      }
      if (supported) target_params.emplace(std::move(store_params));
    }
    if (!target_return || *target_return != *return_type || !target_params ||
        *target_params != parameter_types || target.signature_is_variadic)
      return false;
    resolved = true;
  }
  if (!resolved) {
    const auto extern_it =
        module.extern_decl_link_name_map.find(call.direct_callee_link_name_id);
    if (extern_it == module.extern_decl_link_name_map.end() ||
        extern_it->second.function_signature_ref.value !=
            call.callee_signature_ref.value ||
        extern_it->second.return_type != call.return_type ||
        extern_it->second.return_ext_attr != LirExtAttr::None) {
      return false;
    }
    resolved = true;
  }
  return resolved;
}

std::optional<Type> native_floating_call_type(
    const LirModule& module, const codegen::lir::LirTypeRef& type) {
  if (type.kind() != codegen::lir::LirTypeKind::Floating) return std::nullopt;
  if (type.str() == "float") return Type{TypeKind::F32, 32, "float"};
  if (type.str() == "double") return Type{TypeKind::F64, 64, "double"};
  return std::nullopt;
}

bool exact_direct_native_floating_call(const LirModule& module,
                                       const LirCallOp& call) {
  const auto* result = call.result.value_id();
  if (call.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      !call.structured_args.empty() || !call.arg_type_refs.empty())
    return false;

  const auto return_type = native_floating_call_type(module, call.return_type);
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr && call.callee_signature.has_value() &&
      !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
    return false;
  }
  if (stored_signature == nullptr && !call.callee_signature.has_value()) return false;

  const auto return_ref = stored_signature != nullptr
                              ? stored_signature->return_type_ref
                              : call.callee_signature->return_type_ref;
  const auto return_ext_attr = stored_signature != nullptr
                                   ? stored_signature->return_ext_attr
                                   : call.callee_signature->return_ext_attr;
  const auto is_variadic = stored_signature != nullptr
                               ? stored_signature->is_variadic
                               : call.callee_signature->is_variadic;
  const auto has_unspecified_params =
      stored_signature == nullptr && call.callee_signature->has_unspecified_params;
  const auto has_void_param_list = stored_signature != nullptr
                                       ? stored_signature->has_void_param_list
                                       : call.callee_signature->has_void_param_list;
  const auto& fixed_param_type_refs = stored_signature != nullptr
                                          ? stored_signature->fixed_param_type_refs
                                          : call.callee_signature->fixed_param_type_refs;
  const auto signature_return_type =
      return_ref ? native_floating_call_type(module, *return_ref) : std::nullopt;
  if (!return_type || !return_ref || *return_ref != call.return_type ||
      !signature_return_type || *signature_return_type != *return_type ||
      return_ext_attr != LirExtAttr::None || is_variadic ||
      has_unspecified_params || !has_void_param_list ||
      !fixed_param_type_refs.empty())
    return false;
  if (stored_signature == nullptr && !call.callee_signature->fixed_param_types.empty())
    return false;

  const LirFunction* resolved = nullptr;
  for (const auto& target : module.functions) {
    if (target.link_name_id != call.direct_callee_link_name_id) continue;
    if (resolved) return false;
    const auto target_return = lower_signature_type(
        module, target.return_type, target.signature_return_type_ref);
    const auto target_params = lower_function_parameter_types(module, target);
    const auto target_mirror = target.signature_return_type_ref
                                   ? native_floating_call_type(
                                         module, *target.signature_return_type_ref)
                                   : std::nullopt;
    if (!target.is_declaration || !target_return || *target_return != *return_type ||
        !target_mirror || *target_mirror != *return_type ||
        !target_params || !target_params->empty() || target.signature_is_variadic ||
        !target.signature_has_void_param_list)
      return false;
    resolved = &target;
  }
  return resolved != nullptr;
}

bool exact_downstream_double_fadd(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& native_floating_call_results) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.value_id();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::FAdd ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Floating ||
      bin.type_str.str() != "double" ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs ||
      !lhs->valid() || bin.rhs.kind() != codegen::lir::LirOperandKind::SsaValue ||
      !rhs || !rhs->valid() ||
      native_floating_call_results.count(lhs->value) == 0)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
         lhs_value->second == f64 && rhs_value->second == f64;
}

bool exact_direct_zero_arg_scalar_floating_call_authority(
    const LirModule& module, const LirFunction& function, const LirCallOp& call,
    const codegen::lir::LirDirectZeroArgScalarFloatingCallAuthority&
        authority) {
  const auto* result = call.result.value_id();
  return result && *result == authority.result && authority.result.valid() &&
         authority.owner == function.link_name_id &&
         authority.callee == call.direct_callee_link_name_id &&
         authority.callee != c4c::kInvalidLinkName &&
         authority.return_type == call.return_type &&
         native_floating_call_type(module, authority.return_type)
             .has_value() &&
         authority.role == codegen::lir::
                               LirDirectZeroArgScalarFloatingCallRole::
                                   ResultIntoFloatingBinaryLhs;
}

bool exact_direct_one_double_arg_scalar_floating_call(
    const LirModule& module, const LirCallOp& call) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = call.result.value_id();
  if (call.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      call.structured_args.size() != 1 || call.arg_type_refs.size() != 1 ||
      call.structured_args[0].type_ref != codegen::lir::LirTypeRef("double") ||
      call.arg_type_refs[0] != codegen::lir::LirTypeRef("double") ||
      call.structured_args[0].operand.kind() !=
          codegen::lir::LirOperandKind::SsaValue ||
      !call.structured_args[0].operand.value_id() ||
      !call.structured_args[0].operand.value_id()->valid() ||
      call.return_type != codegen::lir::LirTypeRef("double"))
    return false;

  const auto return_type = native_floating_call_type(module, call.return_type);
  const auto argument_type = native_floating_call_type(module, call.arg_type_refs[0]);
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr && call.callee_signature.has_value() &&
      !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
    return false;
  }
  if (stored_signature == nullptr && !call.callee_signature.has_value()) return false;

  const auto return_ref = stored_signature != nullptr
                              ? stored_signature->return_type_ref
                              : call.callee_signature->return_type_ref;
  const auto return_ext_attr = stored_signature != nullptr
                                   ? stored_signature->return_ext_attr
                                   : call.callee_signature->return_ext_attr;
  const auto is_variadic = stored_signature != nullptr
                               ? stored_signature->is_variadic
                               : call.callee_signature->is_variadic;
  const auto has_unspecified_params =
      stored_signature == nullptr && call.callee_signature->has_unspecified_params;
  const auto& fixed_param_type_refs = stored_signature != nullptr
                                          ? stored_signature->fixed_param_type_refs
                                          : call.callee_signature->fixed_param_type_refs;
  if (!return_type || *return_type != f64 || !argument_type ||
      *argument_type != f64 || !return_ref ||
      *return_ref != codegen::lir::LirTypeRef("double") ||
      return_ext_attr != LirExtAttr::None || is_variadic ||
      has_unspecified_params || fixed_param_type_refs.size() != 1 ||
      fixed_param_type_refs[0] != codegen::lir::LirTypeRef("double"))
    return false;
  if (stored_signature == nullptr &&
      call.callee_signature->fixed_param_types.size() != 1)
    return false;

  const LirFunction* resolved = nullptr;
  for (const auto& target : module.functions) {
    if (target.link_name_id != call.direct_callee_link_name_id) continue;
    if (resolved) return false;
    const auto target_return = lower_signature_type(
        module, target.return_type, target.signature_return_type_ref);
    const auto target_params = lower_function_parameter_types(module, target);
    if (!target_return || *target_return != f64 || !target_params ||
        target_params->size() != 1 ||
        (*target_params)[0] != f64 || target.signature_is_variadic ||
        target.signature_has_void_param_list)
      return false;
    resolved = &target;
  }
  return resolved != nullptr;
}

bool exact_direct_one_double_arg_scalar_floating_call_authority(
    const LirFunction& function, const LirCallOp& call,
    const codegen::lir::LirDirectOneDoubleArgScalarFloatingCallAuthority&
        authority) {
  const auto* result = call.result.value_id();
  return result && *result == authority.result && authority.result.valid() &&
         authority.owner == function.link_name_id &&
         authority.callee == call.direct_callee_link_name_id &&
         authority.callee != c4c::kInvalidLinkName &&
         authority.return_type == codegen::lir::LirTypeRef("double") &&
         authority.argument_type == codegen::lir::LirTypeRef("double") &&
         authority.role == codegen::lir::
                               LirDirectOneDoubleArgScalarFloatingCallRole::
                                   DirectCallResult;
}

bool exact_downstream_double_fmul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& downstream_double_fadd_results) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.value_id();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::FMul ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Floating ||
      bin.type_str.str() != "double" ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs ||
      !lhs->valid() || bin.rhs.kind() != codegen::lir::LirOperandKind::SsaValue ||
      !rhs || !rhs->valid() ||
      downstream_double_fadd_results.count(lhs->value) == 0)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
         lhs_value->second == f64 && rhs_value->second == f64;
}

bool exact_downstream_double_olt_compare(
    const LirCmpOp& compare,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& downstream_double_fmul_results) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = compare.result.value_id();
  const auto* lhs = compare.lhs.value_id();
  const auto* rhs = compare.rhs.value_id();
  if (compare.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !compare.is_float ||
      compare.predicate.typed() != std::optional{codegen::lir::LirCmpPredicate::OLt} ||
      compare.type_str.kind() != codegen::lir::LirTypeKind::Floating ||
      compare.type_str.str() != "double" ||
      compare.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs || !lhs->valid() ||
      compare.rhs.kind() != codegen::lir::LirOperandKind::SsaValue || !rhs || !rhs->valid() ||
      downstream_double_fmul_results.count(lhs->value) != 1)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
      lhs_value->second == f64 && rhs_value->second == f64;
}

bool exact_double_olt_zext_use(
    const LirCastOp& cast,
    const std::unordered_set<std::uint32_t>& double_olt_compare_results) {
  using codegen::lir::LirOperandKind;
  const auto* operand = cast.operand.value_id();
  return cast.kind == codegen::lir::LirCastKind::ZExt && !cast.result.has_authority() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type == codegen::lir::LirTypeRef::integer(1) &&
      cast.to_type == codegen::lir::LirTypeRef::integer(32) &&
      double_olt_compare_results.count(operand->value) == 1;
}

bool exact_double_to_float_fptrunc(
    const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& downstream_double_fmul_results) {
  using codegen::lir::LirOperandKind;
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  if (cast.kind != codegen::lir::LirCastKind::FPTrunc ||
      cast.result.kind() != LirOperandKind::SsaValue || !result || !result->valid() ||
      cast.operand.kind() != LirOperandKind::SsaValue || !operand || !operand->valid() ||
      cast.from_type.kind() != codegen::lir::LirTypeKind::Floating ||
      cast.to_type.kind() != codegen::lir::LirTypeKind::Floating ||
      cast.from_type.str() != "double" || cast.to_type.str() != "float" ||
      downstream_double_fmul_results.count(operand->value) != 1)
    return false;
  const auto found = source_values.find(operand->value);
  return found != source_values.end() && found->second == f64;
}

bool exact_downstream_float_fmul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_fptrunc_results) {
  const Type f32{TypeKind::F32, 32, "float"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.value_id();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::FMul ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Floating ||
      bin.type_str.str() != "float" ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs || !lhs->valid() ||
      bin.rhs.kind() != codegen::lir::LirOperandKind::SsaValue || !rhs || !rhs->valid() ||
      scalar_fptrunc_results.count(lhs->value) != 1)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
      lhs_value->second == f32 && rhs_value->second == f32;
}

bool exact_float_to_double_fpext(
    const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  using codegen::lir::LirOperandKind;
  const Type f32{TypeKind::F32, 32, "float"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  if (cast.kind != codegen::lir::LirCastKind::FPExt ||
      cast.result.kind() != LirOperandKind::SsaValue || !result || !result->valid() ||
      cast.operand.kind() != LirOperandKind::SsaValue || !operand || !operand->valid() ||
      cast.from_type.kind() != codegen::lir::LirTypeKind::Floating ||
      cast.to_type.kind() != codegen::lir::LirTypeKind::Floating ||
      cast.from_type.str() != "float" || cast.to_type.str() != "double")
    return false;
  const auto found = source_values.find(operand->value);
  return found != source_values.end() && found->second == f32;
}

bool exact_signed_i32_to_double_sitofp(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  using codegen::lir::LirOperandKind;
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::SIToFP &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Integer &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Floating && from && to &&
      *from == i32 && *to == f64 && found != source_values.end() && found->second == i32;
}

bool exact_downstream_double_sitofp_fmul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_sitofp_results) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.value_id();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || bin.opcode.typed() != std::optional{codegen::lir::LirBinaryOpcode::FMul} ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Floating || bin.type_str.str() != "double" ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs || !lhs->valid() ||
      bin.rhs.kind() != codegen::lir::LirOperandKind::SsaValue || !rhs || !rhs->valid() ||
      scalar_sitofp_results.count(lhs->value) != 1)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
      lhs_value->second == f64 && rhs_value->second == f64;
}

bool exact_unsigned_i32_to_double_uitofp(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  using codegen::lir::LirOperandKind;
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::UIToFP &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Integer &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Floating && from && to &&
      *from == i32 && *to == f64 && found != source_values.end() && found->second == i32;
}

bool exact_downstream_double_uitofp_fmul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_uitofp_results) {
  return exact_downstream_double_sitofp_fmul(bin, source_values, scalar_uitofp_results);
}

bool exact_double_to_signed_i32_fptosi(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& downstream_double_fadd_results) {
  using codegen::lir::LirOperandKind;
  const Type f64{TypeKind::F64, 64, "double"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::FPToSI &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Floating &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Integer && from && to &&
      *from == f64 && *to == i32 && downstream_double_fadd_results.count(operand->value) == 1 &&
      found != source_values.end() && found->second == f64;
}

bool exact_downstream_i32_fptosi_add(
    const LirBinOp& bin, const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_fptosi_results) {
  using codegen::lir::LirOperandKind;
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  if (bin.result.kind() != LirOperandKind::SsaValue || !result || !result->valid() ||
      bin.opcode.typed() != std::optional{codegen::lir::LirBinaryOpcode::Add} ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Integer ||
      bin.type_str.integer_bit_width() != 32 ||
      bin.lhs.kind() != LirOperandKind::SsaValue || !lhs || !lhs->valid() ||
      bin.rhs.kind() != LirOperandKind::Immediate || scalar_fptosi_results.count(lhs->value) != 1)
    return false;
  const auto found = source_values.find(lhs->value);
  return found != source_values.end() && found->second == i32;
}

bool exact_double_to_unsigned_i32_fptoui(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& downstream_double_fadd_results) {
  using codegen::lir::LirOperandKind;
  const Type f64{TypeKind::F64, 64, "double"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::FPToUI &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Floating &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Integer && from && to &&
      *from == f64 && *to == i32 && downstream_double_fadd_results.count(operand->value) == 1 &&
      found != source_values.end() && found->second == f64;
}

bool exact_downstream_i32_fptoui_add(
    const LirBinOp& bin, const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_fptoui_results) {
  return exact_downstream_i32_fptosi_add(bin, source_values, scalar_fptoui_results);
}

bool exact_wide_ffs_select(const LirSelectOp& select) {
  using codegen::lir::LirOperandKind;
  const auto* result = select.result.value_id();
  return select.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      select.type_str.kind() == codegen::lir::LirTypeKind::Integer &&
      select.type_str.integer_bit_width() == 64 &&
      select.false_val.kind() == LirOperandKind::RawText;
}

bool exact_wide_ffs_trunc(const LirModule& module, const LirCastOp& cast,
                           const std::unordered_map<std::uint32_t, Type>& source_values,
                           const std::unordered_set<std::uint32_t>& select_results) {
  using codegen::lir::LirOperandKind;
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::Trunc &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Integer &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Integer && from && to &&
      *from == Type{TypeKind::Integer, 64, "i64"} &&
      *to == Type{TypeKind::Integer, 32, "i32"} &&
      select_results.count(operand->value) == 1 && found != source_values.end() &&
      found->second == *from;
}

bool exact_downstream_double_fpext_fmul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_fpext_results) {
  const Type f64{TypeKind::F64, 64, "double"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.value_id();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || bin.opcode.typed() != std::optional{codegen::lir::LirBinaryOpcode::FMul} ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Floating || bin.type_str.str() != "double" ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs || !lhs->valid() ||
      bin.rhs.kind() != codegen::lir::LirOperandKind::SsaValue || !rhs || !rhs->valid() ||
      scalar_fpext_results.count(lhs->value) != 1)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  const auto rhs_value = source_values.find(rhs->value);
  return lhs_value != source_values.end() && rhs_value != source_values.end() &&
      lhs_value->second == f64 && rhs_value->second == f64;
}

bool exact_normalized_i32_add(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& selected_global_i32_load_results) {
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.integer_immediate();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::Add ||
      bin.type_str != codegen::lir::LirTypeRef::integer(32) ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs ||
      !lhs->valid() || bin.rhs.kind() != codegen::lir::LirOperandKind::Immediate ||
      !rhs || rhs->value != 1 ||
      selected_global_i32_load_results.count(lhs->value) == 0)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  return lhs_value != source_values.end() && lhs_value->second == i32;
}

bool exact_native_i32_cttz_add(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& native_i32_cttz_results) {
  return exact_normalized_i32_add(bin, source_values, native_i32_cttz_results);
}

bool exact_builtin_ffs_add(const LirBinOp& bin,
                           const std::unordered_map<std::uint32_t, Type>& source_values,
                           const std::unordered_set<std::uint32_t>& cttz_results) {
  using codegen::lir::LirOperandKind;
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* immediate = bin.rhs.integer_immediate();
  const auto width = bin.type_str.integer_bit_width();
  if (bin.result.kind() != LirOperandKind::SsaValue || !result || !result->valid() ||
      bin.opcode.typed() != std::optional{codegen::lir::LirBinaryOpcode::Add} ||
      bin.type_str.kind() != codegen::lir::LirTypeKind::Integer || !width ||
      (*width != 32 && *width != 64) || bin.lhs.kind() != LirOperandKind::SsaValue ||
      !lhs || !lhs->valid() || bin.rhs.kind() != LirOperandKind::Immediate ||
      !immediate || immediate->value != 1 ||
      !integer_immediate_representable(immediate->value, *width) ||
      cttz_results.count(lhs->value) != 1)
    return false;
  const auto found = source_values.find(lhs->value);
  return found != source_values.end() && found->second ==
      Type{TypeKind::Integer, *width, *width == 32 ? "i32" : "i64"};
}

bool exact_builtin_ffs_zero_compare(
    const LirCmpOp& compare) {
  using codegen::lir::LirOperandKind;
  const auto* result = compare.result.value_id();
  const auto* immediate = compare.rhs.integer_immediate();
  const auto width = compare.type_str.integer_bit_width();
  if (compare.result.kind() != LirOperandKind::SsaValue || !result || !result->valid() ||
      compare.is_float ||
      compare.predicate.typed() != std::optional{codegen::lir::LirCmpPredicate::Eq} ||
      compare.type_str.kind() != codegen::lir::LirTypeKind::Integer || !width ||
      (*width != 32 && *width != 64) || compare.rhs.kind() != LirOperandKind::Immediate ||
      !immediate || immediate->value != 0 ||
      !integer_immediate_representable(immediate->value, *width))
    return false;
  const auto* lhs = compare.lhs.integer_immediate();
  return compare.lhs.kind() == LirOperandKind::Immediate && lhs &&
      integer_immediate_representable(lhs->value, *width);
}

bool exact_selected_global_i32_abs(
    const LirAbsOp& abs,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& selected_global_i32_load_results) {
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = abs.result.value_id();
  const auto* argument = abs.arg.value_id();
  if (abs.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || abs.arg.kind() != codegen::lir::LirOperandKind::SsaValue ||
      !argument || !argument->valid() ||
      abs.int_type.kind() != codegen::lir::LirTypeKind::Integer ||
      abs.int_type != codegen::lir::LirTypeRef::integer(32) ||
      selected_global_i32_load_results.count(argument->value) != 1)
    return false;
  const auto argument_value = source_values.find(argument->value);
  return argument_value != source_values.end() && argument_value->second == i32;
}

bool exact_normalized_i32_mul(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& normalized_i32_add_results) {
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.integer_immediate();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::Mul ||
      bin.type_str != codegen::lir::LirTypeRef::integer(32) ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs ||
      !lhs->valid() || bin.rhs.kind() != codegen::lir::LirOperandKind::Immediate ||
      !rhs || rhs->value != 2 ||
      normalized_i32_add_results.count(lhs->value) == 0)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  return lhs_value != source_values.end() && lhs_value->second == i32;
}

bool exact_scalar_i32_to_i64_sext(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  using codegen::lir::LirOperandKind;
  const Type i32{TypeKind::Integer, 32, "i32"};
  const Type i64{TypeKind::Integer, 64, "i64"};
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::SExt &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Integer &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Integer &&
      from && to && *from == i32 && *to == i64 && found != source_values.end() &&
      found->second == i32;
}

bool exact_selected_global_i32_slt_compare(
    const LirCmpOp& compare,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& selected_global_i32_load_results) {
  const auto* result = compare.result.value_id();
  const auto* lhs = compare.lhs.value_id();
  const auto* rhs = compare.rhs.integer_immediate();
  return compare.result.kind() == codegen::lir::LirOperandKind::SsaValue && result &&
      result->valid() && !compare.is_float &&
      compare.predicate.typed() == std::optional{codegen::lir::LirCmpPredicate::Slt} &&
      compare.type_str == codegen::lir::LirTypeRef::integer(32) &&
      compare.lhs.kind() == codegen::lir::LirOperandKind::SsaValue && lhs && lhs->valid() &&
      compare.rhs.kind() == codegen::lir::LirOperandKind::Immediate && rhs && rhs->value == 7 &&
      source_values.count(lhs->value) == 1 &&
      selected_global_i32_load_results.count(lhs->value) == 1;
}

bool exact_downstream_i64_sext_add(
    const LirBinOp& bin,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& scalar_sext_results) {
  const Type i64{TypeKind::Integer, 64, "i64"};
  const auto* result = bin.result.value_id();
  const auto* lhs = bin.lhs.value_id();
  const auto* rhs = bin.rhs.integer_immediate();
  if (bin.result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
      !result->valid() || !bin.opcode.typed() ||
      *bin.opcode.typed() != codegen::lir::LirBinaryOpcode::Add ||
      bin.type_str != codegen::lir::LirTypeRef::integer(64) ||
      bin.lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !lhs ||
      !lhs->valid() || bin.rhs.kind() != codegen::lir::LirOperandKind::Immediate ||
      !rhs || rhs->value != 1 || scalar_sext_results.count(lhs->value) != 1)
    return false;
  const auto lhs_value = source_values.find(lhs->value);
  return lhs_value != source_values.end() && lhs_value->second == i64;
}

bool exact_integer_intrinsic_call(
    const LirModule& module, const LirCallOp& call,
    const std::unordered_map<std::uint32_t, Type>& source_values) {
  using codegen::lir::LirIntrinsicKind;
  using codegen::lir::LirOperandKind;
  const auto* result = call.result.value_id();
  if (!call.intrinsic_kind || !result || !result->valid() ||
      call.result.kind() != LirOperandKind::SsaValue ||
      (call.intrinsic_kind != LirIntrinsicKind::Cttz &&
       call.intrinsic_kind != LirIntrinsicKind::Ctlz &&
       call.intrinsic_kind != LirIntrinsicKind::Ctpop) ||
      call.return_type.kind() != codegen::lir::LirTypeKind::Integer ||
      call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      call.callee.kind() != LirOperandKind::Global ||
      !call.callee.link_name_id() ||
      *call.callee.link_name_id() != call.direct_callee_link_name_id ||
      module.link_names.spelling(call.direct_callee_link_name_id).empty() ||
      !call.callee_type_suffix.empty())
    return false;
  const auto type = lower_lir_type(module, call.return_type);
  const auto* stored_signature = resolved_call_signature(module, call);
  if (stored_signature != nullptr && call.callee_signature.has_value() &&
      !retained_signature_matches_store(*call.callee_signature, *stored_signature)) {
    return false;
  }
  if (stored_signature == nullptr && !call.callee_signature.has_value()) return false;

  const bool count_flag = call.intrinsic_kind == LirIntrinsicKind::Cttz ||
                          call.intrinsic_kind == LirIntrinsicKind::Ctlz;
  const std::size_t count = count_flag ? 2 : 1;
  const auto return_ref = stored_signature != nullptr
                              ? stored_signature->return_type_ref
                              : call.callee_signature->return_type_ref;
  const auto return_ext_attr = stored_signature != nullptr
                                   ? stored_signature->return_ext_attr
                                   : call.callee_signature->return_ext_attr;
  const auto is_variadic = stored_signature != nullptr
                               ? stored_signature->is_variadic
                               : call.callee_signature->is_variadic;
  const auto has_unspecified_params =
      stored_signature == nullptr && call.callee_signature->has_unspecified_params;
  const auto has_void_param_list = stored_signature != nullptr
                                       ? stored_signature->has_void_param_list
                                       : call.callee_signature->has_void_param_list;
  const auto& fixed_param_type_refs = stored_signature != nullptr
                                          ? stored_signature->fixed_param_type_refs
                                          : call.callee_signature->fixed_param_type_refs;
  if (!type || !is_integer_type(*type) || !return_ref ||
      *return_ref != call.return_type || return_ext_attr != LirExtAttr::None ||
      is_variadic || has_unspecified_params || has_void_param_list ||
      fixed_param_type_refs.size() != count ||
      call.arg_type_refs.size() != count || call.structured_args.size() != count ||
      (stored_signature == nullptr &&
       call.callee_signature->fixed_param_types.size() != count) ||
      (stored_signature == nullptr &&
       call.callee_signature->fixed_param_types[0] != call.return_type.str()) ||
      fixed_param_type_refs[0] != call.return_type ||
      call.arg_type_refs[0] != call.return_type ||
      call.structured_args[0].type != call.return_type.str() ||
      call.structured_args[0].type_ref != call.return_type ||
      call.structured_args[0].ext_attr != LirExtAttr::None ||
      (count_flag != call.zero_count_behavior.has_value()))
    return false;
  const auto& operand = call.structured_args[0].operand;
  if (operand.kind() == LirOperandKind::Immediate) {
    const auto* immediate = operand.integer_immediate();
    if (!immediate || !call.return_type.integer_bit_width() ||
        !integer_immediate_representable(immediate->value,
                                         *call.return_type.integer_bit_width()))
      return false;
  } else if (operand.kind() == LirOperandKind::SsaValue) {
    const auto* value = operand.value_id();
    const auto found = value ? source_values.find(value->value) : source_values.end();
    if (!value || !value->valid() || found == source_values.end() ||
        found->second != *type) return false;
  } else return false;
  if (count_flag) {
    const auto expected = *call.zero_count_behavior ==
        codegen::lir::LirZeroCountBehavior::Undefined ? 1 : 0;
    const auto& flag = call.structured_args[1];
    const auto* immediate = flag.operand.integer_immediate();
    if ((stored_signature == nullptr &&
         call.callee_signature->fixed_param_types[1] != "i1") ||
        fixed_param_type_refs[1] != codegen::lir::LirTypeRef::integer(1) ||
        call.arg_type_refs[1] != codegen::lir::LirTypeRef::integer(1) ||
        flag.type != "i1" || flag.type_ref != codegen::lir::LirTypeRef::integer(1) ||
        flag.ext_attr != LirExtAttr::None || !immediate || immediate->value != expected)
      return false;
  }
  return true;
}

bool exact_i64_intrinsic_trunc_cast(
    const LirModule& module, const LirCastOp& cast,
    const std::unordered_map<std::uint32_t, Type>& source_values,
    const std::unordered_set<std::uint32_t>& intrinsic_results) {
  using codegen::lir::LirOperandKind;
  const auto* result = cast.result.value_id();
  const auto* operand = cast.operand.value_id();
  const auto from = lower_lir_type(module, cast.from_type);
  const auto to = lower_lir_type(module, cast.to_type);
  const Type i64{TypeKind::Integer, 64, "i64"};
  const Type i32{TypeKind::Integer, 32, "i32"};
  const auto found = operand ? source_values.find(operand->value) : source_values.end();
  return cast.kind == codegen::lir::LirCastKind::Trunc &&
      cast.result.kind() == LirOperandKind::SsaValue && result && result->valid() &&
      cast.operand.kind() == LirOperandKind::SsaValue && operand && operand->valid() &&
      cast.from_type.kind() == codegen::lir::LirTypeKind::Integer &&
      cast.to_type.kind() == codegen::lir::LirTypeKind::Integer &&
      from && to && *from == i64 && *to == i32 && found != source_values.end() &&
      found->second == i64 && intrinsic_results.count(operand->value) == 1;
}

bool exact_builtin_ctz_call(const LirModule& module, const LirCallOp& call,
                            const std::unordered_map<std::uint32_t, Type>& source_values) {
  const auto type = lower_lir_type(module, call.return_type);
  return exact_integer_intrinsic_call(module, call, source_values) &&
      call.intrinsic_kind == codegen::lir::LirIntrinsicKind::Cttz &&
      call.zero_count_behavior == codegen::lir::LirZeroCountBehavior::Undefined && type &&
      (*type == Type{TypeKind::Integer, 32, "i32"} ||
       *type == Type{TypeKind::Integer, 64, "i64"});
}

bool exact_builtin_clz_call(const LirModule& module, const LirCallOp& call,
                            const std::unordered_map<std::uint32_t, Type>& source_values) {
  const auto type = lower_lir_type(module, call.return_type);
  return exact_integer_intrinsic_call(module, call, source_values) &&
      call.intrinsic_kind == codegen::lir::LirIntrinsicKind::Ctlz &&
      call.zero_count_behavior == codegen::lir::LirZeroCountBehavior::Undefined && type &&
      (*type == Type{TypeKind::Integer, 32, "i32"} ||
       *type == Type{TypeKind::Integer, 64, "i64"});
}

bool exact_builtin_popcount_call(const LirModule& module, const LirCallOp& call,
                                 const std::unordered_map<std::uint32_t, Type>& source_values) {
  const auto type = lower_lir_type(module, call.return_type);
  return exact_integer_intrinsic_call(module, call, source_values) &&
      call.intrinsic_kind == codegen::lir::LirIntrinsicKind::Ctpop &&
      !call.zero_count_behavior.has_value() && type &&
      (*type == Type{TypeKind::Integer, 32, "i32"} ||
       *type == Type{TypeKind::Integer, 64, "i64"});
}

std::optional<Type> lower_constant_type(const LirModule& module,
                                        const TypeSpec& type) {
  if (type.ptr_level != 0 || type.is_lvalue_ref || type.is_rvalue_ref ||
      type.array_rank != 0 || type.is_ptr_to_array || type.inner_rank != 0 ||
      type.is_fn_ptr)
    return std::nullopt;

  if (type.base != TB_VRM_REGISTER && type.vrm_width != 0)
    return std::nullopt;
  if (type.base == TB_VA_LIST) {
    if (type.enum_underlying_base != TB_VOID || type.is_vector ||
        type.vector_lanes != 0 || type.vector_bytes != 0)
      return std::nullopt;

    namespace llvm_helpers = c4c::codegen::llvm_helpers;
    const bool pointer_object =
        llvm_helpers::llvm_va_list_is_pointer_object(module.target_profile);
    VaListTypeFacts facts;
    facts.is_pointer_object = pointer_object;
    facts.storage_size = static_cast<std::uint32_t>(
        llvm_helpers::llvm_va_list_storage_size(module.target_profile));
    facts.storage_alignment = static_cast<std::uint32_t>(
        llvm_helpers::llvm_va_list_alignment(module.target_profile));
    if (!pointer_object) {
      facts.struct_name_id =
          module.struct_names.find("%struct.__va_list_tag_");
      const auto* declaration = module.find_struct_decl(facts.struct_name_id);
      if (facts.struct_name_id == c4c::kInvalidStructName || !declaration ||
          declaration->is_packed || declaration->is_opaque)
        return std::nullopt;

      const auto is_i32 = [](const codegen::lir::LirStructField& field) {
        return field.type.kind() == codegen::lir::LirTypeKind::Integer &&
               field.type.str() == "i32" &&
               field.type.integer_bit_width() == 32 &&
               !field.type.has_struct_name_id();
      };
      const auto is_ptr = [](const codegen::lir::LirStructField& field) {
        return field.type.kind() == codegen::lir::LirTypeKind::Pointer &&
               field.type.str() == "ptr" &&
               !field.type.has_struct_name_id();
      };
      const bool amd64_sysv =
          llvm_helpers::llvm_target_is_amd64_sysv(module.target_profile);
      const auto& fields = declaration->fields;
      if ((amd64_sysv &&
           (fields.size() != 4 || !is_i32(fields[0]) || !is_i32(fields[1]) ||
            !is_ptr(fields[2]) || !is_ptr(fields[3]))) ||
          (!amd64_sysv &&
           (fields.size() != 5 || !is_ptr(fields[0]) || !is_ptr(fields[1]) ||
            !is_ptr(fields[2]) || !is_i32(fields[3]) ||
            !is_i32(fields[4]))))
        return std::nullopt;
    }

    Type result{TypeKind::VaList, 0,
                llvm_helpers::llvm_va_list_storage_ty(module.target_profile)};
    result.va_list_facts = facts;
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }
  if (type.base == TB_VRM_REGISTER) {
    if (type.vrm_width != 1 && type.vrm_width != 2 &&
        type.vrm_width != 4 && type.vrm_width != 8)
      return std::nullopt;
    return Type{TypeKind::VrmRegister,
                static_cast<std::uint32_t>(type.vrm_width),
                "c4c.vrm" + std::to_string(type.vrm_width)};
  }

  std::optional<Type> complex_component;
  switch (type.base) {
    case TB_COMPLEX_FLOAT:
      complex_component = Type{TypeKind::Floating, 32, "float"};
      break;
    case TB_COMPLEX_DOUBLE:
      complex_component = Type{TypeKind::Floating, 64, "double"};
      break;
    case TB_COMPLEX_LONGDOUBLE:
      if (module.target_profile.os == c4c::TargetOs::Windows)
        complex_component = Type{TypeKind::Floating, 64, "double"};
      else if (module.target_profile.arch == c4c::TargetArch::X86_64 ||
               module.target_profile.arch == c4c::TargetArch::I686)
        complex_component = Type{TypeKind::Floating, 80, "x86_fp80"};
      else
        complex_component = Type{TypeKind::Floating, 128, "fp128"};
      break;
    case TB_COMPLEX_CHAR:
    case TB_COMPLEX_SCHAR:
    case TB_COMPLEX_UCHAR:
      complex_component = Type{TypeKind::Integer, 8, "i8"};
      break;
    case TB_COMPLEX_SHORT:
    case TB_COMPLEX_USHORT:
      complex_component = Type{TypeKind::Integer, 16, "i16"};
      break;
    case TB_COMPLEX_INT:
    case TB_COMPLEX_UINT:
      complex_component = Type{TypeKind::Integer, 32, "i32"};
      break;
    case TB_COMPLEX_LONG:
    case TB_COMPLEX_ULONG:
    case TB_COMPLEX_LONGLONG:
    case TB_COMPLEX_ULONGLONG:
      complex_component = Type{TypeKind::Integer, 64, "i64"};
      break;
    default: break;
  }
  if (complex_component) {
    Type result{TypeKind::Complex, complex_component->bit_width,
                "{ " + complex_component->spelling + ", " +
                    complex_component->spelling + " }"};
    result.complex_facts = ComplexTypeFacts{complex_component->kind,
                                            complex_component->bit_width};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  TypeBase storage_base = type.base;
  if (storage_base == TB_ENUM) {
    storage_base = type.enum_underlying_base;
    if (storage_base == TB_VOID) storage_base = TB_INT;
    switch (storage_base) {
      case TB_BOOL:
      case TB_CHAR:
      case TB_UCHAR:
      case TB_SCHAR:
      case TB_SHORT:
      case TB_USHORT:
      case TB_INT:
      case TB_UINT:
      case TB_LONG:
      case TB_ULONG:
      case TB_LONGLONG:
      case TB_ULONGLONG:
      case TB_INT128:
      case TB_UINT128: break;
      default: return std::nullopt;
    }
  }

  std::uint32_t width = 0;
  switch (storage_base) {
    case TB_BOOL: width = 1; break;
    case TB_CHAR:
    case TB_UCHAR:
    case TB_SCHAR: width = 8; break;
    case TB_SHORT:
    case TB_USHORT: width = 16; break;
    case TB_INT:
    case TB_UINT: width = 32; break;
    case TB_LONG:
    case TB_ULONG:
      width = c4c::long_width_bits(module.target_profile);
      break;
    case TB_LONGLONG:
    case TB_ULONGLONG: width = 64; break;
    case TB_INT128:
    case TB_UINT128: width = 128; break;
    case TB_FLOAT: return Type{TypeKind::Floating, 32, "float"};
    case TB_DOUBLE: return Type{TypeKind::Floating, 64, "double"};
    case TB_LONGDOUBLE:
      if (module.target_profile.os == c4c::TargetOs::Windows)
        return Type{TypeKind::Floating, 64, "double"};
      if (module.target_profile.arch == c4c::TargetArch::X86_64 ||
          module.target_profile.arch == c4c::TargetArch::I686)
        return Type{TypeKind::Floating, 80, "x86_fp80"};
      return Type{TypeKind::Floating, 128, "fp128"};
    default: return std::nullopt;
  }
  return Type{TypeKind::Integer, width, "i" + std::to_string(width)};
}

std::optional<Type> lower_global_compatibility_type(const LirModule& module,
                                                    const TypeSpec& type) {
  return lower_constant_type(module, type);
}

std::optional<ReturnExtension> lower_return_extension(
    LirExtAttr extension) {
  switch (extension) {
    case LirExtAttr::None: return ReturnExtension::None;
    case LirExtAttr::SignExt: return ReturnExtension::SignExt;
    case LirExtAttr::ZeroExt: return ReturnExtension::ZeroExt;
  }
  return std::nullopt;
}

struct GlobalLinkageFacts {
  bool is_weak = false;
  SymbolVisibility visibility = SymbolVisibility::Default;
};

std::optional<GlobalLinkageFacts> decode_global_linkage(
    const LirGlobal& global) {
  std::string_view expected_prefix;
  bool is_weak = false;
  if (global.is_extern_decl) {
    if (global.is_internal) return std::nullopt;
    if (global.linkage_vis.rfind("extern_weak ", 0) == 0) {
      expected_prefix = "extern_weak ";
      is_weak = true;
    } else {
      expected_prefix = "external ";
    }
  } else if (global.is_internal) {
    expected_prefix = "internal ";
  } else if (global.linkage_vis.rfind("weak ", 0) == 0) {
    expected_prefix = "weak ";
    is_weak = true;
  }

  if (global.linkage_vis.size() < expected_prefix.size() ||
      global.linkage_vis.compare(0, expected_prefix.size(), expected_prefix) !=
          0)
    return std::nullopt;
  const std::string_view suffix(global.linkage_vis.data() + expected_prefix.size(),
                                global.linkage_vis.size() - expected_prefix.size());
  SymbolVisibility visibility;
  if (suffix.empty())
    visibility = SymbolVisibility::Default;
  else if (suffix == "hidden ")
    visibility = SymbolVisibility::Hidden;
  else if (suffix == "protected ")
    visibility = SymbolVisibility::Protected;
  else
    return std::nullopt;
  return GlobalLinkageFacts{is_weak, visibility};
}

std::optional<Type> lower_global_type(const LirModule& module,
                                      const LirGlobal& global) {
  constexpr int kArrayDimensionCapacity =
      sizeof(global.type.array_dims) / sizeof(global.type.array_dims[0]);
  const bool ordinary_no_split =
      global.type.inner_rank == -1 || global.type.inner_rank == 0;
  if (global.type.base != TB_VRM_REGISTER && global.type.vrm_width != 0)
    return std::nullopt;
  if (!global.type.is_vector &&
      (global.type.vector_lanes != 0 || global.type.vector_bytes != 0))
    return std::nullopt;
  const auto lower_vector_facts = [&](TypeSpec element_spec)
      -> std::optional<std::pair<VectorTypeFacts, std::string>> {
    if (element_spec.vector_lanes <= 0 || element_spec.vector_bytes <= 0 ||
        element_spec.vrm_width != 0 || element_spec.base == TB_ENUM)
      return std::nullopt;
    const auto lane_count = element_spec.vector_lanes;
    const auto storage_bytes = element_spec.vector_bytes;
    element_spec.is_vector = false;
    element_spec.vector_lanes = 0;
    element_spec.vector_bytes = 0;
    element_spec.ptr_level = 0;
    element_spec.is_lvalue_ref = false;
    element_spec.is_rvalue_ref = false;
    element_spec.array_rank = 0;
    element_spec.array_size = -1;
    for (auto& dimension : element_spec.array_dims) dimension = -1;
    element_spec.is_ptr_to_array = false;
    element_spec.inner_rank = 0;
    element_spec.is_fn_ptr = false;
    element_spec.array_size_expr = nullptr;
    const auto element = lower_constant_type(module, element_spec);
    if (!element || (element->kind != TypeKind::Integer &&
                     element->kind != TypeKind::Floating))
      return std::nullopt;
    return std::pair{
        VectorTypeFacts{element->kind, element->bit_width, lane_count,
                        storage_bytes},
        "<" + std::to_string(lane_count) + " x " + element->spelling + ">"};
  };
  const auto lower_function_return_facts = [&](TypeSpec return_spec)
      -> std::optional<FunctionPointerTypeFacts> {
    return_spec.ptr_level = 0;
    return_spec.is_lvalue_ref = false;
    return_spec.is_rvalue_ref = false;
    return_spec.array_rank = 0;
    return_spec.array_size = -1;
    for (auto& dimension : return_spec.array_dims) dimension = -1;
    return_spec.is_ptr_to_array = false;
    return_spec.inner_rank = 0;
    return_spec.is_fn_ptr = false;
    return_spec.array_size_expr = nullptr;
    if (return_spec.base == TB_VOID) {
      if (return_spec.enum_underlying_base != TB_VOID ||
          return_spec.vrm_width != 0 || return_spec.is_vector ||
          return_spec.vector_lanes != 0 || return_spec.vector_bytes != 0)
        return std::nullopt;
      return FunctionPointerTypeFacts{TypeKind::Void, 0};
    }
    const auto result = lower_constant_type(module, return_spec);
    if (!result || (result->kind != TypeKind::Integer &&
                    result->kind != TypeKind::Floating))
      return std::nullopt;
    return FunctionPointerTypeFacts{result->kind, result->bit_width};
  };
  const bool direct_scalar_vector =
      global.type.is_vector && global.type.vector_lanes > 0 &&
      global.type.vector_bytes > 0 && global.type.vrm_width == 0 &&
      global.type.base != TB_ENUM &&
      global.type.ptr_level == 0 &&
      !global.type.is_lvalue_ref && !global.type.is_rvalue_ref &&
      global.type.array_rank == 0 && !global.type.is_ptr_to_array &&
      ordinary_no_split && !global.type.is_fn_ptr &&
      global.type.array_size_expr == nullptr;
  if (global.type.is_vector) {
    if (global.llvm_type_ref) return std::nullopt;
    const auto vector = lower_vector_facts(global.type);
    if (!vector) return std::nullopt;

    const bool pointer_to_vector =
        (global.is_extern_decl || !global.init_text.empty()) &&
        global.type.ptr_level > 0 && !global.type.is_lvalue_ref &&
        !global.type.is_rvalue_ref && global.type.array_rank == 0 &&
        !global.type.is_ptr_to_array && ordinary_no_split &&
        !global.type.is_fn_ptr && global.type.array_size_expr == nullptr;
    if (pointer_to_vector) {
      if (global.llvm_type != "ptr") return std::nullopt;
      Type result{TypeKind::Pointer};
      result.pointer_facts = PointerTypeFacts{
          TypeKind::Vector, 0, global.type.ptr_level, std::nullopt,
          std::nullopt, vector->first};
      if (!is_well_formed(result)) return std::nullopt;
      return result;
    }

    const bool fixed_vector_array =
        global.type.ptr_level >= 0 && !global.type.is_lvalue_ref &&
        !global.type.is_rvalue_ref && global.type.array_rank >= 1 &&
        global.type.array_rank <= kArrayDimensionCapacity &&
        global.type.array_size >= 0 && !global.type.is_ptr_to_array &&
        ordinary_no_split && !global.type.is_fn_ptr &&
        global.type.array_size_expr == nullptr;
    if (fixed_vector_array) {
      std::vector<std::int64_t> dimensions;
      dimensions.reserve(global.type.array_rank);
      for (int i = 0; i < global.type.array_rank; ++i) {
        if (global.type.array_dims[i] < 0) return std::nullopt;
        dimensions.push_back(global.type.array_dims[i]);
      }
      if (dimensions.front() != global.type.array_size) return std::nullopt;
      std::string expected =
          global.type.ptr_level > 0 ? "ptr" : vector->second;
      for (auto dimension = dimensions.rbegin(); dimension != dimensions.rend();
           ++dimension)
        expected = "[" + std::to_string(*dimension) + " x " + expected + "]";
      if (global.llvm_type != expected) return std::nullopt;
      Type result{TypeKind::Array, 0, expected};
      result.array_facts = ArrayTypeFacts{
          TypeKind::Vector, 0, global.type.ptr_level, std::move(dimensions),
          std::nullopt, std::nullopt, vector->first};
      if (!is_well_formed(result)) return std::nullopt;
      return result;
    }

    if (!direct_scalar_vector || global.llvm_type != vector->second)
      return std::nullopt;
    Type result{TypeKind::Vector, 0, vector->second};
    result.vector_facts = vector->first;
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool fixed_function_pointer_array =
      global.type.is_fn_ptr && global.type.ptr_level > 0 &&
      !global.type.is_lvalue_ref && !global.type.is_rvalue_ref &&
      global.type.array_rank >= 1 &&
      global.type.array_rank <= kArrayDimensionCapacity &&
      global.type.array_size >= 0 && !global.type.is_ptr_to_array &&
      ordinary_no_split &&
      !global.type.is_vector &&
      global.type.array_size_expr == nullptr;
  if (fixed_function_pointer_array) {
    if (global.llvm_type_ref) return std::nullopt;
    const auto return_facts = lower_function_return_facts(global.type);
    if (!return_facts) return std::nullopt;
    std::vector<std::int64_t> dimensions;
    dimensions.reserve(global.type.array_rank);
    for (int i = 0; i < global.type.array_rank; ++i) {
      if (global.type.array_dims[i] < 0) return std::nullopt;
      dimensions.push_back(global.type.array_dims[i]);
    }
    if (dimensions.front() != global.type.array_size) return std::nullopt;
    std::string expected = "ptr";
    for (auto dimension = dimensions.rbegin(); dimension != dimensions.rend();
         ++dimension)
      expected = "[" + std::to_string(*dimension) + " x " + expected + "]";
    if (global.llvm_type != expected) return std::nullopt;
    Type result{TypeKind::Array, 0, expected};
    result.array_facts = ArrayTypeFacts{
        TypeKind::Function, 0, global.type.ptr_level, std::move(dimensions),
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        *return_facts};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool direct_function_pointer =
      (global.is_extern_decl || !global.init_text.empty()) &&
      global.type.is_fn_ptr && global.type.ptr_level > 0 &&
      !global.type.is_lvalue_ref && !global.type.is_rvalue_ref &&
      global.type.array_rank == 0 && !global.type.is_ptr_to_array &&
      ordinary_no_split &&
      !global.type.is_vector &&
      global.type.array_size_expr == nullptr;
  if (direct_function_pointer) {
    if (global.llvm_type != "ptr" || global.llvm_type_ref)
      return std::nullopt;
    const auto return_facts = lower_function_return_facts(global.type);
    if (!return_facts) return std::nullopt;
    Type result{TypeKind::Pointer};
    result.pointer_facts = PointerTypeFacts{
        TypeKind::Function, 0, global.type.ptr_level, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, *return_facts};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool mixed_pointer_to_array_global =
      (global.is_extern_decl || !global.init_text.empty()) &&
      global.type.ptr_level > 0 && !global.type.is_lvalue_ref &&
      !global.type.is_rvalue_ref && global.type.array_rank >= 2 &&
      global.type.array_rank <= kArrayDimensionCapacity &&
      global.type.inner_rank > 0 &&
      global.type.inner_rank < global.type.array_rank &&
      global.type.array_size >= 0 && global.type.is_ptr_to_array &&
      !global.type.is_fn_ptr && !global.type.is_vector &&
      global.type.array_size_expr == nullptr;
  if (mixed_pointer_to_array_global) {
    if (global.llvm_type_ref) return std::nullopt;
    const int outer_rank = global.type.array_rank - global.type.inner_rank;
    std::vector<std::int64_t> outer_dimensions;
    std::vector<std::int64_t> pointee_dimensions;
    outer_dimensions.reserve(outer_rank);
    pointee_dimensions.reserve(global.type.inner_rank);
    for (int i = 0; i < global.type.array_rank; ++i) {
      if (global.type.array_dims[i] < 0) return std::nullopt;
      if (i < outer_rank)
        outer_dimensions.push_back(global.type.array_dims[i]);
      else
        pointee_dimensions.push_back(global.type.array_dims[i]);
    }
    if (outer_dimensions.front() != global.type.array_size)
      return std::nullopt;

    TypeSpec element_spec = global.type;
    element_spec.ptr_level = 0;
    element_spec.array_rank = 0;
    element_spec.array_size = -1;
    for (auto& dimension : element_spec.array_dims) dimension = -1;
    element_spec.is_ptr_to_array = false;
    element_spec.inner_rank = 0;
    const auto element = lower_constant_type(module, element_spec);
    if (!element || (element->kind != TypeKind::Integer &&
                     element->kind != TypeKind::Floating &&
                     element->kind != TypeKind::Complex &&
                     element->kind != TypeKind::VrmRegister))
      return std::nullopt;

    std::string expected = "ptr";
    for (auto dimension = outer_dimensions.rbegin();
         dimension != outer_dimensions.rend(); ++dimension)
      expected = "[" + std::to_string(*dimension) + " x " + expected + "]";
    if (global.llvm_type != expected) return std::nullopt;
    Type result{TypeKind::Array, 0, expected};
    result.array_facts = ArrayTypeFacts{
        element->kind, element->bit_width, global.type.ptr_level,
        std::move(outer_dimensions), element->complex_facts,
        PointerArrayTypeFacts{std::move(pointee_dimensions),
                              global.type.inner_rank}};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool fixed_scalar_base_array =
      global.type.ptr_level >= 0 &&
      !global.type.is_lvalue_ref &&
      !global.type.is_rvalue_ref && global.type.array_rank >= 1 &&
      global.type.array_rank <= kArrayDimensionCapacity &&
      global.type.array_size >= 0 &&
      !global.type.is_ptr_to_array && ordinary_no_split &&
      !global.type.is_fn_ptr && !global.type.is_vector &&
      global.type.array_size_expr == nullptr;
  if (fixed_scalar_base_array) {
    if (global.llvm_type_ref) return std::nullopt;
    std::vector<std::int64_t> dimensions;
    dimensions.reserve(global.type.array_rank);
    for (int i = 0; i < global.type.array_rank; ++i) {
      if (global.type.array_dims[i] < 0) return std::nullopt;
      dimensions.push_back(global.type.array_dims[i]);
    }
    if (dimensions.front() != global.type.array_size) return std::nullopt;

    TypeSpec element_spec = global.type;
    element_spec.array_rank = 0;
    element_spec.array_size = -1;
    for (auto& dimension : element_spec.array_dims) dimension = -1;
    const int element_pointer_depth = element_spec.ptr_level;
    element_spec.ptr_level = 0;
    element_spec.inner_rank = 0;
    const auto element = lower_constant_type(module, element_spec);
    if (!element || (element->kind != TypeKind::Integer &&
                     element->kind != TypeKind::Floating &&
                     element->kind != TypeKind::Complex &&
                     element->kind != TypeKind::VrmRegister &&
                     element->kind != TypeKind::VaList))
      return std::nullopt;

    std::string expected =
        element_pointer_depth > 0 ? "ptr" : element->spelling;
    for (auto dimension = dimensions.rbegin(); dimension != dimensions.rend();
         ++dimension)
      expected = "[" + std::to_string(*dimension) + " x " + expected + "]";
    if (global.llvm_type != expected) return std::nullopt;
    Type result{TypeKind::Array, 0, expected};
    result.array_facts =
        ArrayTypeFacts{element->kind, element->bit_width,
                       element_pointer_depth, std::move(dimensions),
                       element->complex_facts, std::nullopt, std::nullopt,
                       element->va_list_facts};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool pure_pointer_to_array_global =
      (global.is_extern_decl || !global.init_text.empty()) &&
      global.type.ptr_level > 0 && !global.type.is_lvalue_ref &&
      !global.type.is_rvalue_ref && global.type.array_rank >= 1 &&
      global.type.array_rank <= kArrayDimensionCapacity &&
      global.type.array_size >= 0 && global.type.is_ptr_to_array &&
      (global.type.inner_rank == -1 ||
       global.type.inner_rank == global.type.array_rank) &&
      !global.type.is_fn_ptr && !global.type.is_vector &&
      global.type.array_size_expr == nullptr;
  if (pure_pointer_to_array_global) {
    if (global.llvm_type != "ptr" || global.llvm_type_ref)
      return std::nullopt;
    std::vector<std::int64_t> pointee_dimensions;
    pointee_dimensions.reserve(global.type.array_rank);
    for (int i = 0; i < global.type.array_rank; ++i) {
      if (global.type.array_dims[i] < 0) return std::nullopt;
      pointee_dimensions.push_back(global.type.array_dims[i]);
    }
    if (pointee_dimensions.front() != global.type.array_size)
      return std::nullopt;

    TypeSpec pointee_spec = global.type;
    pointee_spec.ptr_level = 0;
    pointee_spec.array_rank = 0;
    pointee_spec.array_size = -1;
    for (auto& dimension : pointee_spec.array_dims) dimension = -1;
    pointee_spec.is_ptr_to_array = false;
    pointee_spec.inner_rank = 0;
    const auto pointee = lower_constant_type(module, pointee_spec);
    if (!pointee || (pointee->kind != TypeKind::Integer &&
                     pointee->kind != TypeKind::Floating &&
                     pointee->kind != TypeKind::Complex &&
                     pointee->kind != TypeKind::VrmRegister))
      return std::nullopt;

    Type result{TypeKind::Pointer};
    result.pointer_facts = PointerTypeFacts{
        pointee->kind, pointee->bit_width, global.type.ptr_level,
        pointee->complex_facts,
        PointerArrayTypeFacts{std::move(pointee_dimensions),
                              global.type.inner_rank}};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool scalar_pointer_global =
      (global.is_extern_decl || !global.init_text.empty()) &&
      global.type.ptr_level > 0 &&
      !global.type.is_lvalue_ref && !global.type.is_rvalue_ref &&
      global.type.array_rank == 0 && !global.type.is_ptr_to_array &&
      ordinary_no_split && !global.type.is_fn_ptr &&
      !global.type.is_vector && global.type.array_size_expr == nullptr;
  if (scalar_pointer_global) {
    if (global.llvm_type != "ptr") return std::nullopt;
    TypeSpec pointee_spec = global.type;
    pointee_spec.ptr_level = 0;
    pointee_spec.inner_rank = 0;
    const auto pointee = lower_constant_type(module, pointee_spec);
    if (!pointee || (pointee->kind != TypeKind::Integer &&
                     pointee->kind != TypeKind::Floating &&
                     pointee->kind != TypeKind::Complex &&
                     pointee->kind != TypeKind::VrmRegister &&
                     pointee->kind != TypeKind::VaList))
      return std::nullopt;

    if (global.llvm_type_ref) {
      const auto mirror = lower_lir_type(module, *global.llvm_type_ref);
      if (!mirror || mirror->kind != TypeKind::Pointer ||
          mirror->spelling != "ptr" ||
          global.llvm_type_ref->str() != "ptr")
        return std::nullopt;
    }

    Type result{TypeKind::Pointer};
    result.pointer_facts = PointerTypeFacts{
        pointee->kind, pointee->bit_width, global.type.ptr_level,
        pointee->complex_facts, std::nullopt, std::nullopt,
        pointee->va_list_facts};
    if (!is_well_formed(result)) return std::nullopt;
    return result;
  }

  const bool direct_aggregate =
      (global.type.base == TB_STRUCT || global.type.base == TB_UNION) &&
      global.type.ptr_level == 0 && !global.type.is_lvalue_ref &&
      !global.type.is_rvalue_ref && global.type.array_rank == 0 &&
      !global.type.is_ptr_to_array && ordinary_no_split &&
      !global.type.is_fn_ptr;
  if (direct_aggregate) {
    if ((!global.is_extern_decl && global.init_text.empty()) ||
        !global.llvm_type_ref)
      return std::nullopt;
    const auto authoritative =
        lower_lir_type(module, *global.llvm_type_ref);
    if (!authoritative || authoritative->kind != TypeKind::Struct ||
        global.llvm_type_ref->str() != global.llvm_type ||
        !is_well_formed(*authoritative))
      return std::nullopt;

    if (authoritative->struct_name_id == c4c::kInvalidStructName) {
      if (global.is_extern_decl || global.type.base != TB_STRUCT ||
          authoritative->spelling.size() < 2 ||
          authoritative->spelling.front() != '{' ||
          authoritative->spelling.back() != '}')
        return std::nullopt;
      return authoritative;
    }

    if (module.find_struct_decl(authoritative->struct_name_id) == nullptr)
      return std::nullopt;
    return authoritative;
  }

  if (!ordinary_no_split) return std::nullopt;
  TypeSpec compatibility_type = global.type;
  compatibility_type.inner_rank = 0;
  const auto authoritative =
      lower_global_compatibility_type(module, compatibility_type);
  if (!authoritative || authoritative->kind == TypeKind::Void ||
      !is_well_formed(*authoritative))
    return std::nullopt;

  if (authoritative->kind == TypeKind::Pointer) {
    if (global.llvm_type != authoritative->spelling) return std::nullopt;
    if (global.llvm_type_ref) {
      const auto mirror = lower_lir_type(module, *global.llvm_type_ref);
      if (!mirror || *mirror != *authoritative ||
          global.llvm_type_ref->str() != authoritative->spelling)
        return std::nullopt;
    }
    return authoritative;
  }

  if (authoritative->kind != TypeKind::Integer &&
      authoritative->kind != TypeKind::Floating &&
      authoritative->kind != TypeKind::Complex &&
      authoritative->kind != TypeKind::VrmRegister &&
      authoritative->kind != TypeKind::VaList)
    return std::nullopt;
  if ((authoritative->kind == TypeKind::VrmRegister ||
       authoritative->kind == TypeKind::VaList) &&
      global.llvm_type_ref)
    return std::nullopt;
  if (global.llvm_type != authoritative->spelling) return std::nullopt;
  if (global.llvm_type_ref) {
    const auto mirror = lower_lir_type(module, *global.llvm_type_ref);
    if (!mirror || *mirror != *authoritative ||
        global.llvm_type_ref->str() != authoritative->spelling)
      return std::nullopt;
  }
  return authoritative;
}

bool same_lir_type(const codegen::lir::LirTypeRef& lhs,
                   const codegen::lir::LirTypeRef& rhs) {
  return lhs.kind() == rhs.kind() && lhs.str() == rhs.str() &&
         lhs.integer_bit_width() == rhs.integer_bit_width() &&
         lhs.vrm_width() == rhs.vrm_width() &&
         lhs.has_struct_name_id() == rhs.has_struct_name_id() &&
         (!lhs.has_struct_name_id() ||
          lhs.struct_name_id() == rhs.struct_name_id());
}

bool same_extern_snapshot(const LirExternDecl& declaration,
                          const LirModule::ExternDeclInfo& evidence) {
  return declaration.name == evidence.name &&
         declaration.return_type_str == evidence.return_type_str &&
         same_lir_type(declaration.return_type, evidence.return_type) &&
         declaration.return_ext_attr == evidence.return_ext_attr &&
         declaration.link_name_id == evidence.link_name_id;
}

std::size_t inline_asm_constraint_count(std::string_view constraints) {
  if (constraints.empty()) return 0;
  std::size_t count = 1;
  unsigned brace_depth = 0;
  for (const char ch : constraints) {
    if (ch == '{') {
      ++brace_depth;
    } else if (ch == '}' && brace_depth != 0) {
      --brace_depth;
    } else if (ch == ',' && brace_depth == 0) {
      ++count;
    }
  }
  return count;
}

bool is_native_inline_asm_output_type(const Type& type) {
  return type == Type{TypeKind::Integer, 32, "i32"} ||
         type == Type{TypeKind::Integer, 64, "i64"};
}

Result<void, ImportError> validate_inline_asm_shape(
    const LirModule& module, const LirInlineAsmOp& inline_asm,
    const std::string& function,
    const std::string& block,
    std::unordered_map<std::string, Type>& ordinary_values,
    std::unordered_map<std::uint32_t, Type>& source_values,
    std::unordered_set<std::uint32_t>& inline_asm_results) {
  if (inline_asm.insn_r)
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmMetadata, function,
                      block,
                      "parsed insn.r metadata is not Raw/Canonical BIR authority");

  // This receiver row deliberately has no text-derived admission facts.  Its
  // result identity and type come only from the structured binding; opaque asm
  // text and constraint spellings are carried later as payload, not decoded to
  // recover a value or type.
  const auto selected_output_type = lower_lir_type(
      module, inline_asm.ordinary_results.empty()
                  ? codegen::lir::LirTypeRef{}
                  : inline_asm.ordinary_results.front().type);
  const bool selected_output = inline_asm.ordinary_inputs.empty() &&
      inline_asm.ordinary_results.size() == 1 &&
      inline_asm.ordinary_results.front().value.kind() ==
          codegen::lir::LirOperandKind::SsaValue &&
      inline_asm.ordinary_results.front().value.value_id() &&
      inline_asm.ordinary_results.front().value.value_id()->valid() &&
      inline_asm.ordinary_results.front().role == LirInlineAsmValueRole::Output &&
      inline_asm.ordinary_results.front().constraint_index == 0 &&
      selected_output_type && is_native_inline_asm_output_type(*selected_output_type);
  if (selected_output) {
    const auto id = inline_asm.ordinary_results.front().value.value_id()->value;
    if (source_values.count(id) != 0 || !inline_asm_results.insert(id).second)
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "native inline-asm result LirValueId is duplicate");
    source_values.emplace(id, *selected_output_type);
    return Result<void, ImportError>::success();
  }

  const bool has_structured_values = !inline_asm.ordinary_inputs.empty() ||
                                     !inline_asm.ordinary_results.empty();
  if (!has_structured_values &&
      (!inline_asm.args_str.empty() || !inline_asm.result.empty() ||
       inline_asm.ret_type.kind() != codegen::lir::LirTypeKind::Void ||
       inline_asm.ret_type.str() != "void"))
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                      "textual LLVM operands/results have no structured LIR value identities");
  if (!inline_asm.result.empty() && inline_asm.ordinary_results.empty())
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                      "LLVM compatibility result lacks a structured result identity");
  const std::size_t constraint_count =
      inline_asm_constraint_count(inline_asm.original_constraint_text);
  if (has_structured_values && constraint_count == 0)
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                      "structured values require original semantic constraints");
  std::optional<std::size_t> previous_input_constraint;
  for (const auto& input : inline_asm.ordinary_inputs) {
    if (input.value.kind() != codegen::lir::LirOperandKind::SsaValue ||
        (input.role != LirInlineAsmValueRole::Input &&
         input.role != LirInlineAsmValueRole::ReadWrite) ||
        input.constraint_index >= constraint_count ||
        (previous_input_constraint && input.constraint_index <= *previous_input_constraint))
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "structured input identity, role, or constraint order is invalid");
    previous_input_constraint = input.constraint_index;
    const auto type = lower_lir_type(module, input.type);
    const auto found = ordinary_values.find(input.value.str());
    if (!type || found == ordinary_values.end() || found->second != *type)
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "structured input does not resolve with its defining type");
  }
  std::optional<std::size_t> previous_result_constraint;
  std::unordered_set<std::string> pending_results;
  for (const auto& result : inline_asm.ordinary_results) {
    if (result.value.kind() != codegen::lir::LirOperandKind::SsaValue ||
        (result.role != LirInlineAsmValueRole::Output &&
         result.role != LirInlineAsmValueRole::ReadWrite) ||
        result.constraint_index >= constraint_count ||
        (previous_result_constraint && result.constraint_index <= *previous_result_constraint))
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "structured result identity, role, or constraint order is invalid");
    previous_result_constraint = result.constraint_index;
    const auto type = lower_lir_type(module, result.type);
    if (!type || ordinary_values.count(result.value.str()) != 0 ||
        !pending_results.insert(result.value.str()).second)
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "structured result is duplicate or has unsupported type");
    const LirInlineAsmValueBinding* matching_input = nullptr;
    for (const auto& input : inline_asm.ordinary_inputs) {
      if (input.value == result.value)
        return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                          "structured result must be distinct from every input");
      if (input.constraint_index == result.constraint_index) matching_input = &input;
    }
    if ((result.role == LirInlineAsmValueRole::ReadWrite &&
         (!matching_input || matching_input->role != LirInlineAsmValueRole::ReadWrite ||
          lower_lir_type(module, matching_input->type) != type)) ||
        (result.role == LirInlineAsmValueRole::Output && matching_input))
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "structured result/input constraint pairing is invalid");
  }
  for (const auto& input : inline_asm.ordinary_inputs) {
    if (input.role == LirInlineAsmValueRole::ReadWrite &&
        std::none_of(inline_asm.ordinary_results.begin(), inline_asm.ordinary_results.end(),
                     [&](const LirInlineAsmValueBinding& result) {
                       return result.role == LirInlineAsmValueRole::ReadWrite &&
                              result.constraint_index == input.constraint_index;
                     }))
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function, block,
                        "read/write input lacks a distinct produced result");
  }
  for (const auto& result : inline_asm.ordinary_results)
    ordinary_values.emplace(result.value.str(), *lower_lir_type(module, result.type));
  return Result<void, ImportError>::success();
}

Result<void, ImportError> validate_module_surface(const LirModule& module) {
  std::unordered_set<std::string> global_names;
  std::unordered_set<c4c::LinkNameId> global_link_ids;
  global_names.reserve(module.globals.size());
  global_link_ids.reserve(module.globals.size());
  for (const auto& global : module.globals) {
    if (global.name.empty() || !global_names.insert(global.name).second)
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "global name and structured type identity must be present and unique");
    const auto linkage = decode_global_linkage(global);
    if (!linkage)
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "global linkage and visibility spelling is malformed or contradicts its independent flags");
    const bool const_pointer_producer_row =
        !global.is_extern_decl && !global.is_internal && global.is_const &&
        !linkage->is_weak && global.qualifier == "global " &&
        !global.init_text.empty();
    const auto type = lower_global_type(module, global);
    if (!type)
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "global structured type authority is malformed or conflicts with compatibility evidence");
    if (global.llvm_type_ref && global.llvm_type_ref->has_struct_name_id() &&
        module.find_struct_decl(global.llvm_type_ref->struct_name_id()) == nullptr)
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "global structured type names an unresolved declaration");
    const bool coherent_external =
        global.is_extern_decl && !global.is_internal &&
        !linkage->is_weak && global.qualifier == "global " &&
        global.init_text.empty() &&
        global.initializer_function_link_name_ids.empty();
    const bool coherent_weak_external =
        global.is_extern_decl && !global.is_internal &&
        linkage->is_weak &&
        global.qualifier == "global " && global.init_text.empty() &&
        global.initializer_function_link_name_ids.empty();
    const bool coherent_ordinary_definition =
        !global.is_extern_decl && !global.is_internal && !global.is_const &&
        !linkage->is_weak && global.qualifier == "global " &&
        !global.init_text.empty();
    const bool coherent_constant_definition =
        !global.is_extern_decl && !global.is_internal && global.is_const &&
        !linkage->is_weak && global.qualifier == "constant " &&
        type->kind != TypeKind::Pointer && !global.init_text.empty();
    const bool coherent_const_pointer_definition =
        const_pointer_producer_row && type->kind == TypeKind::Pointer;
    const bool coherent_internal_const_pointer_definition =
        !global.is_extern_decl && global.is_internal && global.is_const &&
        !linkage->is_weak && global.qualifier == "global " &&
        type->kind == TypeKind::Pointer && !global.init_text.empty();
    const bool coherent_internal_ordinary_definition =
        !global.is_extern_decl && global.is_internal && !global.is_const &&
        !linkage->is_weak && global.qualifier == "global " &&
        !global.init_text.empty();
    const bool coherent_internal_constant_definition =
        !global.is_extern_decl && global.is_internal && global.is_const &&
        !linkage->is_weak && global.qualifier == "constant " &&
        type->kind != TypeKind::Pointer &&
        !global.init_text.empty();
    const bool coherent_weak_ordinary_definition =
        !global.is_extern_decl && !global.is_internal && !global.is_const &&
        linkage->is_weak && global.qualifier == "global " &&
        !global.init_text.empty();
    const bool coherent_weak_constant_definition =
        !global.is_extern_decl && !global.is_internal && global.is_const &&
        linkage->is_weak && global.qualifier == "constant " &&
        type->kind != TypeKind::Pointer && !global.init_text.empty();
    const bool coherent_weak_const_pointer_definition =
        !global.is_extern_decl && !global.is_internal && global.is_const &&
        linkage->is_weak && global.qualifier == "global " &&
        type->kind == TypeKind::Pointer && !global.init_text.empty();
    if (!coherent_external && !coherent_weak_external &&
        !coherent_ordinary_definition &&
        !coherent_constant_definition &&
        !coherent_const_pointer_definition &&
        !coherent_internal_const_pointer_definition &&
        !coherent_internal_ordinary_definition &&
        !coherent_internal_constant_definition &&
        !coherent_weak_ordinary_definition &&
        !coherent_weak_constant_definition &&
        !coherent_weak_const_pointer_definition)
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "only coherent external or weak-external declarations and initialized ordinary, internal, or weak global/constant definitions are admitted");
    if (global.align_bytes < 0 ||
        (global.align_bytes != 0 &&
         (global.align_bytes & (global.align_bytes - 1)) != 0))
      return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                        "global alignment must be zero or a positive power of two");
    if (global.link_name_id != c4c::kInvalidLinkName) {
      if (module.link_names.spelling(global.link_name_id) != global.name ||
          !global_link_ids.insert(global.link_name_id).second)
        return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                          "link-backed global identity is unresolved, mismatched, or duplicated");
    }
    for (const c4c::LinkNameId function_link :
         global.initializer_function_link_name_ids) {
      if (function_link == c4c::kInvalidLinkName ||
          module.link_names.spelling(function_link).empty())
        return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                          "initializer function link is invalid or unresolved");
    }
  }
  if (module.string_pool.empty()) {
    if (!module.str_pool_map.empty() || module.str_pool_idx != 0)
      return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                        "empty string pool requires an empty cache and zero counter");
  } else {
    if (module.str_pool_idx < 0 ||
        module.string_pool.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        static_cast<std::size_t>(module.str_pool_idx) !=
            module.string_pool.size())
      return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                        "string-pool vector and counter disagree");

    std::unordered_set<std::string> ordered_names;
    std::unordered_set<std::string> ordinary_names;
    ordered_names.reserve(module.string_pool.size());
    ordinary_names.reserve(module.string_pool.size());
    for (const auto& string_data : module.string_pool) {
      if (string_data.pool_name.empty() || string_data.byte_length < -1 ||
          !ordered_names.insert(string_data.pool_name).second)
        return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                          "string-pool rows require unique names and supported lengths");
      if (string_data.byte_length >= 0)
        ordinary_names.insert(string_data.pool_name);
    }

    if (module.str_pool_map.size() != ordinary_names.size())
      return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                        "string-pool cache size must match ordinary rows");

    std::unordered_set<std::string> cached_names;
    cached_names.reserve(module.str_pool_map.size());
    for (const auto& cache_entry : module.str_pool_map)
      if (cache_entry.second.empty() ||
          !cached_names.insert(cache_entry.second).second)
        return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                          "string-pool cache values must be unique names");
    if (cached_names != ordinary_names)
      return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                        "cache values must resolve one-to-one to ordinary rows");
  }
  const auto valid_name_table = [](const auto& table, auto invalid) {
    if (table.ids_.key_by_id_.size() != table.ids_.id_by_key_.size())
      return false;
    for (std::size_t index = 0; index < table.size(); ++index) {
      const auto id = static_cast<decltype(invalid)>(index + 1);
      const auto text_id = table.text_id(id);
      const auto spelling = table.spelling(id);
      if (id == invalid || text_id == c4c::kInvalidText || spelling.empty() ||
          table.find(spelling) != id)
        return false;
    }
    return true;
  };
  if (!valid_name_table(module.link_names, c4c::kInvalidLinkName) ||
      !valid_name_table(module.struct_names, c4c::kInvalidStructName))
    return fail<void>(ImportErrorCode::UnsupportedTypeDeclarations, {}, {},
                      "module semantic name table caches are inconsistent");

  if (module.struct_decl_index.size() != module.struct_decls.size())
    return fail<void>(ImportErrorCode::UnsupportedTypeDeclarations, {}, {},
                      "struct declaration index size does not match source order");
  std::unordered_set<c4c::StructNameId> declared_names;
  for (std::size_t index = 0; index < module.struct_decls.size(); ++index) {
    const auto& decl = module.struct_decls[index];
    const auto cached = module.struct_decl_index.find(decl.name_id);
    if (decl.name_id == c4c::kInvalidStructName ||
        module.struct_names.spelling(decl.name_id).empty() ||
        !declared_names.insert(decl.name_id).second ||
        cached == module.struct_decl_index.end() || cached->second != index ||
        (decl.is_opaque && (!decl.fields.empty() || decl.is_packed)))
      return fail<void>(ImportErrorCode::UnsupportedTypeDeclarations, {}, {},
                        "struct declaration identity, shape, or index is invalid");
    for (const auto& field : decl.fields) {
      const auto type = lower_lir_type(module, field.type);
      if (!type || type->kind == TypeKind::Void ||
          (field.type.has_struct_name_id() &&
           module.find_struct_decl(field.type.struct_name_id()) == nullptr))
        return fail<void>(ImportErrorCode::UnsupportedTypeDeclarations, {}, {},
                          "struct declaration contains a malformed or unresolved field type");
    }
  }

  if (module.extern_decl_link_name_map.size() +
          module.extern_decl_name_map.size() !=
      module.extern_decls.size())
    return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                      "external declaration vector and parity maps differ in size");

  std::unordered_set<c4c::LinkNameId> external_link_ids;
  std::unordered_set<std::string> external_names;
  external_link_ids.reserve(module.extern_decls.size());
  external_names.reserve(module.extern_decls.size());
  for (const auto& declaration : module.extern_decls) {
    const auto type = lower_lir_type(module, declaration.return_type);
    const auto extension = lower_return_extension(declaration.return_ext_attr);
    if (declaration.name.empty() || !type || !extension ||
        declaration.return_type_str != declaration.return_type.str() ||
        (declaration.return_type.has_struct_name_id() &&
         module.find_struct_decl(declaration.return_type.struct_name_id()) ==
             nullptr) ||
        (*extension != ReturnExtension::None && !is_integer_type(*type)) ||
        !external_names.insert(declaration.name).second)
      return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                        "external declaration name, type, extension, or identity is malformed");

    if (declaration.link_name_id != c4c::kInvalidLinkName) {
      if (module.link_names.spelling(declaration.link_name_id) !=
              declaration.name ||
          !external_link_ids.insert(declaration.link_name_id).second)
        return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                          "link-backed external identity is unresolved or duplicated");
      const auto evidence =
          module.extern_decl_link_name_map.find(declaration.link_name_id);
      if (evidence == module.extern_decl_link_name_map.end() ||
          !same_extern_snapshot(declaration, evidence->second))
        return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                          "link-backed external parity evidence is missing or conflicting");
    } else {
      const auto evidence = module.extern_decl_name_map.find(declaration.name);
      if (evidence == module.extern_decl_name_map.end() ||
          !same_extern_snapshot(declaration, evidence->second))
        return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                          "fallback external parity evidence is missing or conflicting");
    }
  }
  for (const auto& entry : module.extern_decl_link_name_map) {
    if (entry.first == c4c::kInvalidLinkName ||
        entry.second.link_name_id != entry.first || entry.second.name.empty() ||
        module.link_names.spelling(entry.first) != entry.second.name)
      return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                        "external link map contains malformed parity evidence");
  }
  for (const auto& entry : module.extern_decl_name_map) {
    if (entry.first.empty() || entry.second.name != entry.first ||
        entry.second.link_name_id != c4c::kInvalidLinkName)
      return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                        "external fallback map contains malformed parity evidence");
  }
  std::unordered_set<c4c::LinkNameId> specialization_link_ids;
  std::unordered_set<std::string> specialization_semantic_keys;
  for (const auto& entry : module.spec_entries) {
    std::string semantic_key = std::to_string(entry.template_origin.size());
    semantic_key.push_back(':');
    semantic_key += entry.template_origin;
    semantic_key += entry.spec_key;
    if (entry.spec_key.empty() || entry.template_origin.empty() ||
        entry.mangled_name.empty() ||
        entry.mangled_link_name_id == c4c::kInvalidLinkName ||
        module.link_names.spelling(entry.mangled_link_name_id) !=
            entry.mangled_name ||
        !specialization_link_ids.insert(entry.mangled_link_name_id).second ||
        !specialization_semantic_keys.insert(std::move(semantic_key)).second)
      return fail<void>(ImportErrorCode::UnsupportedSpecializations, {}, {},
                        "specialization fields, link identity, or uniqueness are malformed");
  }
  return Result<void, ImportError>::success();
}

Result<void, ImportError> validate_function(const LirModule& module,
                                            const LirFunction& function) {
  const std::string name = function_link_name(module, function);
  if (name.empty())
    return fail<void>(ImportErrorCode::EmptyFunctionLinkName, function.name, {},
                      "function has no resolvable link-visible name");
  if ((function.is_internal && !function.can_elide_if_unreferenced) ||
      (function.is_declaration &&
       (function.is_internal || function.can_elide_if_unreferenced)))
    return fail<void>(ImportErrorCode::UnsupportedFunctionMetadata, name, {},
                      "function linkage/elision metadata violates producer invariants");
  if (function.signature_is_variadic && !function.is_declaration)
    return fail<void>(ImportErrorCode::UnsupportedVariadicFunction, name, {},
                      "variadic functions require explicit signature lowering");
  const bool admitted_variadic_declaration =
      function.signature_is_variadic &&
      variadic_declaration_signature_store_matches(module, function);
  const bool admitted_byval_declaration =
      byval_declaration_signature_store_matches(module, function);
  if (function.signature_is_variadic && !admitted_variadic_declaration)
    return fail<void>(ImportErrorCode::UnsupportedVariadicFunction, name, {},
                      "variadic declarations require matching module "
                      "function-signature store facts");
  if (!admitted_variadic_declaration && !admitted_byval_declaration &&
      !lower_function_parameter_types(module, function))
    return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                      "function parameters are outside exact zero, void-list, "
                      "or target-stable plain scalar receipt");
  const auto signature_return_type =
      lower_signature_type(module, function.return_type,
                           function.signature_return_type_ref);
  if (!signature_return_type)
    return fail<void>(ImportErrorCode::UnsupportedReturnType, name, {},
                      "structured return TypeSpec is malformed, outside direct "
                      "scalar receipt, or conflicts with its optional mirror");

  const bool has_body_state = !function.blocks.empty() ||
                              !function.stack_objects.empty() ||
                              !function.alloca_insts.empty();
  if (function.is_declaration && has_body_state)
    return fail<void>(ImportErrorCode::DeclarationHasBody, name, {},
                      "a declaration cannot silently discard body state");
  if (function.is_declaration) return Result<void, ImportError>::success();
  if (function.blocks.empty())
    return fail<void>(ImportErrorCode::DefinitionHasNoBlocks, name, {},
                      "a definition must contain at least one block");
  if (!function.stack_objects.empty())
    return fail<void>(ImportErrorCode::UnsupportedStackObjects, name, {},
                      "stack objects require the memory family");
  const auto direct_pointer_parameter_count = std::count_if(
      function.native_body_parameter_definitions.begin(),
      function.native_body_parameter_definitions.end(), [](const auto& parameter) {
        return parameter.abi ==
            codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
      });
  if (direct_pointer_parameter_count > 1)
    return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                      "only one selected direct-pointer body parameter is receivable");
  const codegen::lir::LirReturnValueParameterAuthority*
      selected_return_value_parameter_authority = nullptr;
  const codegen::lir::LirSwitchSelectorParameterAuthority*
      selected_switch_selector_parameter_authority = nullptr;
  const codegen::lir::LirTruthinessComparisonLhsParameterAuthority*
      selected_truthiness_lhs_parameter_authority = nullptr;
  const codegen::lir::LirPointerTruthinessParameterAuthority*
      selected_pointer_truthiness_parameter_authority = nullptr;
  const codegen::lir::LirFixedDirectCallArgumentParameterAuthority*
      selected_fixed_direct_call_argument_authority = nullptr;
  const codegen::lir::LirDirectZeroArgScalarFloatingCallAuthority*
      selected_direct_floating_call_result_authority = nullptr;
  for (const auto& block : function.blocks) {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    if (!ret) continue;
    const auto returned_definition =
        ret->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                ret->value_str &&
                ret->value_str->kind() == codegen::lir::LirOperandKind::SsaValue &&
                ret->value_str->value_id() && function.signature_return_type_ref &&
                *function.signature_return_type_ref == ret->type_str
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& definition) {
                             return definition.value == *ret->value_str->value_id() &&
                                    definition.type == ret->type_str &&
                                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                           })
            : function.native_body_parameter_definitions.end();
    if (!ret->return_value_parameter_authority) {
      if (returned_definition != function.native_body_parameter_definitions.end())
        return fail<void>(ImportErrorCode::UnsupportedTerminator, name, block.label,
                          "direct scalar return parameter requires its one typed authority row");
      continue;
    }
    const auto& authority = *ret->return_value_parameter_authority;
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (selected_return_value_parameter_authority || !authority.value.valid() ||
        authority.owner != function.link_name_id ||
        authority.parameter_index >= function.params.size() ||
        authority.parameter_index >= function.signature_param_type_refs.size() ||
        authority.abi != codegen::lir::LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != codegen::lir::LirReturnValueParameterRole::ReturnValue ||
        ret->type_str.kind() != codegen::lir::LirTypeKind::Integer || !ret->value_str ||
        ret->value_str->kind() != codegen::lir::LirOperandKind::SsaValue ||
        !ret->value_str->value_id() || *ret->value_str->value_id() != authority.value ||
        ret->type_str != authority.type || !function.signature_return_type_ref ||
        *function.signature_return_type_ref != ret->type_str ||
        function.signature_param_type_refs[authority.parameter_index] != authority.type ||
        matches != 1)
      return fail<void>(ImportErrorCode::UnsupportedTerminator, name, block.label,
                        "direct scalar return parameter requires one exact typed current-function authority row");
    selected_return_value_parameter_authority = &authority;
  }
  for (const auto& block : function.blocks) {
    const auto* sw = std::get_if<LirSwitch>(&block.terminator);
    if (!sw) continue;
    const auto selected_definition = std::find_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == sw->selector &&
                 definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
        });
    if (!sw->selector_parameter_authority) {
      if (selected_definition != function.native_body_parameter_definitions.end())
        return fail<void>(ImportErrorCode::UnsupportedTerminator, name, block.label,
                          "direct scalar switch selector parameter requires its one typed authority row");
      continue;
    }
    const auto& authority = *sw->selector_parameter_authority;
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (selected_switch_selector_parameter_authority || !authority.value.valid() ||
        authority.owner != function.link_name_id ||
        authority.parameter_index >= function.params.size() ||
        authority.parameter_index >= function.signature_param_type_refs.size() ||
        authority.abi != codegen::lir::LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != codegen::lir::LirSwitchSelectorParameterRole::SwitchSelector ||
        sw->selector != authority.value || sw->selector_type_ref != authority.type ||
        authority.type.kind() != codegen::lir::LirTypeKind::Integer ||
        function.signature_param_type_refs[authority.parameter_index] != authority.type ||
        matches != 1)
      return fail<void>(ImportErrorCode::UnsupportedTerminator, name, block.label,
                        "direct scalar switch selector parameter requires one exact typed current-function authority row");
    selected_switch_selector_parameter_authority = &authority;
  }
  for (const auto& block : function.blocks) for (const auto& instruction : block.insts) {
    const auto* compare = std::get_if<LirCmpOp>(&instruction);
    if (!compare) continue;
    const auto selected_definition =
        !compare->is_float && compare->predicate.typed() ==
                std::optional{codegen::lir::LirCmpPredicate::Ne} &&
                compare->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                compare->lhs.kind() == codegen::lir::LirOperandKind::SsaValue &&
                compare->lhs.value_id() && compare->rhs.integer_immediate() &&
                compare->rhs.integer_immediate()->value == 0
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& definition) {
                             return definition.value == *compare->lhs.value_id() &&
                                    definition.type == compare->type_str &&
                                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                           })
            : function.native_body_parameter_definitions.end();
    if (!compare->truthiness_lhs_parameter_authority) {
      if (selected_definition != function.native_body_parameter_definitions.end())
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                          "direct scalar truthiness comparison requires its one typed authority row");
      continue;
    }
    const auto& authority = *compare->truthiness_lhs_parameter_authority;
    const auto matches = std::count_if(function.native_body_parameter_definitions.begin(),
                                       function.native_body_parameter_definitions.end(),
        [&](const auto& definition) {
          return definition.value == authority.value &&
              definition.parameter_index == authority.parameter_index &&
              definition.type == authority.type && definition.owner == authority.owner &&
              definition.abi == authority.abi;
        });
    if (selected_truthiness_lhs_parameter_authority || !authority.value.valid() ||
        authority.owner != function.link_name_id || authority.parameter_index >= function.params.size() ||
        authority.parameter_index >= function.signature_param_type_refs.size() ||
        authority.abi != codegen::lir::LirNativeBodyParameterAbi::DirectScalar ||
        authority.role != codegen::lir::LirTruthinessComparisonLhsParameterRole::TruthinessComparisonLhs ||
        compare->is_float || compare->predicate.typed() != std::optional{codegen::lir::LirCmpPredicate::Ne} ||
        compare->type_str.kind() != codegen::lir::LirTypeKind::Integer ||
        compare->lhs.kind() != codegen::lir::LirOperandKind::SsaValue || !compare->lhs.value_id() ||
        *compare->lhs.value_id() != authority.value || compare->type_str != authority.type ||
        !compare->rhs.integer_immediate() || compare->rhs.integer_immediate()->value != 0 ||
        function.signature_param_type_refs[authority.parameter_index] != authority.type || matches != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                        "direct scalar truthiness comparison requires one exact typed current-function authority row");
    selected_truthiness_lhs_parameter_authority = &authority;
  }
  for (const auto& block : function.blocks) for (std::size_t index = 0; index < block.insts.size(); ++index) {
    const auto* cast = std::get_if<codegen::lir::LirCastOp>(&block.insts[index]);
    if (!cast || cast->kind != codegen::lir::LirCastKind::PtrToInt ||
        cast->from_type.kind() != codegen::lir::LirTypeKind::Pointer ||
        cast->to_type != codegen::lir::LirTypeRef::integer(64) ||
        cast->operand.kind() != codegen::lir::LirOperandKind::SsaValue ||
        !cast->operand.value_id() || !cast->result.value_id()) {
      continue;
    }
    const auto* compare = index + 1 < block.insts.size()
        ? std::get_if<LirCmpOp>(&block.insts[index + 1])
        : nullptr;
    const auto selected_definition = std::find_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(),
        [&](const auto& definition) {
          return definition.value == *cast->operand.value_id() &&
                 definition.type == cast->from_type &&
                 definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
        });
    if (!compare || compare->is_float ||
        compare->predicate.typed() != std::optional{codegen::lir::LirCmpPredicate::Ne} ||
        compare->type_str != codegen::lir::LirTypeRef::integer(64) ||
        compare->lhs.kind() != codegen::lir::LirOperandKind::SsaValue ||
        !compare->lhs.value_id() || *compare->lhs.value_id() != *cast->result.value_id() ||
        !compare->rhs.integer_immediate() ||
        compare->rhs.integer_immediate()->value != 0) {
      if (selected_definition != function.native_body_parameter_definitions.end())
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                          "direct pointer truthiness parameter requires its one typed PtrToInt comparison authority row");
      continue;
    }
    if (!compare->pointer_truthiness_parameter_authority) {
      if (selected_definition != function.native_body_parameter_definitions.end())
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                          "direct pointer truthiness parameter requires its one typed PtrToInt comparison authority row");
      continue;
    }
    const auto& authority = *compare->pointer_truthiness_parameter_authority;
    const auto matches = std::count_if(
        function.native_body_parameter_definitions.begin(),
        function.native_body_parameter_definitions.end(), [&](const auto& definition) {
          return definition.value == authority.value &&
                 definition.parameter_index == authority.parameter_index &&
                 definition.type == authority.type && definition.owner == authority.owner &&
                 definition.abi == authority.abi;
        });
    if (selected_pointer_truthiness_parameter_authority || !authority.value.valid() ||
        authority.owner != function.link_name_id ||
        authority.parameter_index >= function.params.size() ||
        authority.parameter_index >= function.signature_param_type_refs.size() ||
        authority.abi != codegen::lir::LirNativeBodyParameterAbi::DirectPointer ||
        authority.role != codegen::lir::LirPointerTruthinessParameterRole::PointerTruthiness ||
        authority.type.kind() != codegen::lir::LirTypeKind::Pointer ||
        *cast->operand.value_id() != authority.value ||
        cast->from_type != authority.type ||
        function.signature_param_type_refs[authority.parameter_index] != authority.type ||
        matches != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                        "direct pointer truthiness parameter requires one exact typed current-function authority row");
    selected_pointer_truthiness_parameter_authority = &authority;
  }
  for (const auto& block : function.blocks) for (const auto& instruction : block.insts) {
    const auto* call = std::get_if<LirCallOp>(&instruction);
    if (!call) continue;
    const auto* floating_authority =
        call->direct_zero_arg_scalar_floating_call_authority
            ? &*call->direct_zero_arg_scalar_floating_call_authority
            : nullptr;
    const auto* call_result = call->result.value_id();
    const auto direct_floating_result_type =
        native_floating_call_type(module, call->return_type);
    const bool direct_zero_arg_floating_call =
        call_result && call_result->valid() && direct_floating_result_type &&
        call->direct_callee_link_name_id != c4c::kInvalidLinkName &&
        call->structured_args.empty() && call->arg_type_refs.empty() &&
        call->return_ext_attr == LirExtAttr::None;
    bool has_floating_lhs_consumer = false;
    if (direct_zero_arg_floating_call) {
      for (const auto& consumer_block : function.blocks) {
        for (const auto& consumer_inst : consumer_block.insts) {
          const auto* bin = std::get_if<LirBinOp>(&consumer_inst);
          if (bin && bin->lhs.kind() == codegen::lir::LirOperandKind::SsaValue &&
              bin->lhs.value_id() && *bin->lhs.value_id() == *call_result &&
              (bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FAdd} ||
               bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FSub} ||
               bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FMul}) &&
              bin->type_str == call->return_type) {
            has_floating_lhs_consumer = true;
          }
        }
      }
    }
    if (!floating_authority && has_floating_lhs_consumer)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                        block.label,
                        "direct zero-argument scalar floating call result requires its one typed authority row");
    if (floating_authority) {
      const auto result = call_result;
      const auto return_type = direct_floating_result_type;
      const auto* stored_signature = resolved_call_signature(module, *call);
      const auto return_ref = stored_signature
                                  ? stored_signature->return_type_ref
                                  : call->callee_signature
                                        ? call->callee_signature->return_type_ref
                                        : std::optional<
                                              codegen::lir::LirTypeRef>{};
      const auto return_ext_attr = stored_signature
                                       ? stored_signature->return_ext_attr
                                       : call->callee_signature
                                             ? call->callee_signature
                                                   ->return_ext_attr
                                             : LirExtAttr::None;
      const auto is_variadic = stored_signature
                                   ? stored_signature->is_variadic
                                   : call->callee_signature &&
                                         call->callee_signature->is_variadic;
      const auto has_unspecified_params =
          !stored_signature && call->callee_signature &&
          call->callee_signature->has_unspecified_params;
      const bool zero_fixed_params =
          stored_signature ? stored_signature->fixed_param_type_refs.empty()
                           : call->callee_signature &&
                                 call->callee_signature->fixed_param_type_refs
                                     .empty();
      std::size_t consumer_count = 0;
      for (const auto& consumer_block : function.blocks) {
        for (const auto& consumer_inst : consumer_block.insts) {
          const auto* bin = std::get_if<LirBinOp>(&consumer_inst);
          if (!bin || bin->lhs.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !bin->lhs.value_id() || *bin->lhs.value_id() != floating_authority->result) {
            continue;
          }
          const bool floating_binary_lhs =
              (bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FAdd} ||
               bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FSub} ||
               bin->opcode.typed() ==
                   std::optional{codegen::lir::LirBinaryOpcode::FMul}) &&
              bin->type_str == floating_authority->return_type;
          if (floating_binary_lhs) ++consumer_count;
        }
      }
      if (selected_direct_floating_call_result_authority || !result ||
          !result->valid() || !return_type ||
          floating_authority->result != *result ||
          !floating_authority->result.valid() ||
          floating_authority->owner != function.link_name_id ||
          floating_authority->callee != call->direct_callee_link_name_id ||
          floating_authority->return_type != call->return_type ||
          floating_authority->role !=
              codegen::lir::LirDirectZeroArgScalarFloatingCallRole::
                  ResultIntoFloatingBinaryLhs ||
          call->direct_callee_link_name_id == c4c::kInvalidLinkName ||
          !call->structured_args.empty() || !call->arg_type_refs.empty() ||
          call->return_ext_attr != LirExtAttr::None ||
          call->return_type != floating_authority->return_type ||
          !return_ref || !zero_fixed_params || has_unspecified_params ||
          *return_ref != floating_authority->return_type ||
          return_ext_attr != LirExtAttr::None || is_variadic ||
          consumer_count != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "direct zero-argument scalar floating call result requires one exact typed authority row consumed as floating binary LHS");
      selected_direct_floating_call_result_authority = floating_authority;
    }
    if (call->direct_one_double_arg_scalar_floating_call_authority) {
      const auto& authority =
          *call->direct_one_double_arg_scalar_floating_call_authority;
      if (!exact_direct_one_double_arg_scalar_floating_call(module, *call) ||
          !exact_direct_one_double_arg_scalar_floating_call_authority(
              function, *call, authority)) {
        return fail<void>(
            ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
            "direct one-double-argument scalar floating call result requires one exact double(double) authority row");
      }
    }
    for (std::size_t index = 0; index < call->structured_args.size(); ++index) {
      const auto& argument = call->structured_args[index];
      const auto* authority = argument.fixed_direct_call_argument_parameter_authority
          ? &*argument.fixed_direct_call_argument_parameter_authority
          : nullptr;
      const auto* argument_value = argument.operand.value_id();
      const auto matching_definition_count = authority ? std::count_if(
          function.native_body_parameter_definitions.begin(),
          function.native_body_parameter_definitions.end(), [&](const auto& definition) {
            return definition.value == authority->value &&
                definition.parameter_index == authority->parameter_index &&
                definition.type == authority->type && definition.owner == authority->owner &&
                definition.abi == authority->abi;
          }) : 0;
      const auto selected_definition = argument_value
          ? std::find_if(function.native_body_parameter_definitions.begin(),
                         function.native_body_parameter_definitions.end(), [&](const auto& definition) {
                           return definition.value == *argument_value &&
                               definition.type == argument.type_ref &&
                               definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                         }) : function.native_body_parameter_definitions.end();
      if (!authority) {
        if ((index == 0 || index == 1) &&
            selected_definition != function.native_body_parameter_definitions.end())
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "direct scalar fixed direct-call argument requires its one typed authority row");
        continue;
      }
      const auto expected_role = index == 0
          ? codegen::lir::LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument0
          : index == 1
              ? codegen::lir::LirFixedDirectCallArgumentParameterRole::FixedDirectCallArgument1
              : codegen::lir::LirFixedDirectCallArgumentParameterRole::Invalid;
      const auto* call_fixed_params = call_fixed_param_type_refs(module, *call);
      if (selected_fixed_direct_call_argument_authority || index > 1 ||
          !authority->value.valid() ||
          authority->owner != function.link_name_id ||
          authority->parameter_index >= function.params.size() ||
          authority->parameter_index >= function.signature_param_type_refs.size() ||
          authority->abi != codegen::lir::LirNativeBodyParameterAbi::DirectScalar ||
          authority->role != expected_role ||
          !argument_value || *argument_value != authority->value ||
          argument.type_ref != authority->type ||
          function.signature_param_type_refs[authority->parameter_index] != authority->type ||
          !call_fixed_params || call_fixed_params->size() <= index ||
          (*call_fixed_params)[index] != authority->type ||
          matching_definition_count != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                          "direct scalar fixed direct-call argument requires one exact typed authority row");
      selected_fixed_direct_call_argument_authority = authority;
    }
  }
  const LirMemcpyOp* overflow_memcpy = nullptr;
  const codegen::lir::LirAmd64SysVOverflowAggregateCarrier* overflow_carrier = nullptr;
  for (const auto& block : function.blocks) for (const auto& instruction : block.insts) {
    const auto* memcpy = std::get_if<LirMemcpyOp>(&instruction);
    if (!memcpy || !memcpy->amd64_sysv_overflow_aggregate_carrier) continue;
    if (overflow_memcpy)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                        "current function may publish exactly one selected AMD64 SysV overflow aggregate memcpy");
    overflow_memcpy = memcpy;
    overflow_carrier = &*memcpy->amd64_sysv_overflow_aggregate_carrier;
  }
  const bool has_overflow_carrier = overflow_carrier != nullptr;

  if (!function.alloca_insts.empty()) {
    if (function.alloca_insts.size() != (has_overflow_carrier ? 2u : 1u))
      return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                        "only the selected hoisted alloca authority rows are receivable");
    bool saw_overflow_va_list = false;
    bool saw_overflow_destination = false;
    for (const auto& instruction : function.alloca_insts) {
      const auto* alloca = std::get_if<LirAllocaOp>(&instruction);
      const auto* authority = alloca && alloca->local_object_authority
          ? &*alloca->local_object_authority : nullptr;
      const auto pointee = alloca ? lower_lir_type(module, alloca->type_str) : std::optional<Type>{};
      const auto pointer = authority ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
      const auto authority_pointee = authority ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
      const auto* result = alloca ? alloca->result.value_id() : nullptr;
      const bool matches_overflow_local = has_overflow_carrier && authority && result &&
          *result == overflow_carrier->va_list_object.pointer_definition &&
          authority->object == overflow_carrier->va_list_object.object &&
          authority->owner == overflow_carrier->va_list_object.owner &&
          authority->pointer_type == overflow_carrier->va_list_object.pointer_type &&
          authority->pointee_type == overflow_carrier->va_list_object.pointee_type &&
          authority->live == overflow_carrier->va_list_object.live;
      const bool matches_destination_local = has_overflow_carrier && authority && result &&
          *result == overflow_carrier->destination.pointer_definition &&
          authority->object == overflow_carrier->destination.object &&
          authority->owner == overflow_carrier->destination.owner &&
          authority->pointer_type == overflow_carrier->destination.pointer_type &&
          authority->pointee_type == overflow_carrier->destination.pointee_type &&
          authority->live == overflow_carrier->destination.live;
      const bool selected_overflow_local =
          matches_overflow_local || matches_destination_local;
      if (!alloca || !authority || !alloca->count.str().empty() ||
          alloca->result.kind() != codegen::lir::LirOperandKind::SsaValue || !result || !result->valid() ||
          authority->pointer_definition != *result || !authority->object.valid() ||
          function.link_name_id == c4c::kInvalidLinkName || authority->owner != function.link_name_id ||
          !pointer || pointer->kind != TypeKind::Pointer || !pointee || !authority_pointee ||
          *pointee != *authority_pointee || !authority->live ||
          (has_overflow_carrier && !selected_overflow_local))
        return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                          "alloca requires the selected live typed current-function authority binding");
      saw_overflow_va_list = saw_overflow_va_list || matches_overflow_local;
      saw_overflow_destination = saw_overflow_destination || matches_destination_local;
    }
    if (has_overflow_carrier && (!saw_overflow_va_list || !saw_overflow_destination))
      return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                        "AMD64 SysV overflow aggregate receipt requires both exact local authorities");
  }
  if (has_overflow_carrier && function.alloca_insts.empty())
    return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                      "AMD64 SysV overflow aggregate receipt requires both local authorities");

  std::size_t selected_stack_save_count = 0;
  std::size_t selected_stack_restore_count = 0;
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.insts) {
      if (const auto* stack_save =
              std::get_if<codegen::lir::LirStackSaveOp>(&instruction);
          stack_save && stack_save->requires_native_stack_save_authority)
        ++selected_stack_save_count;
      if (const auto* stack_restore =
              std::get_if<codegen::lir::LirStackRestoreOp>(&instruction);
          stack_restore && stack_restore->requires_native_stack_restore_authority)
        ++selected_stack_restore_count;
    }
  }
  if (selected_stack_save_count > 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "current function may publish exactly one selected VLA stack-save authority");
  if (selected_stack_restore_count > 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "current function may publish exactly one selected VLA stack-restore authority");

  std::size_t selected_memcpy_count = 0;
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.insts) {
      const auto* memcpy = std::get_if<LirMemcpyOp>(&instruction);
      if (!memcpy) continue;
      ++selected_memcpy_count;
      if (memcpy->amd64_sysv_overflow_aggregate_carrier) {
        const auto& carrier = *memcpy->amd64_sysv_overflow_aggregate_carrier;
        const auto payload_type = lower_lir_type(module, carrier.payload_type);
        if (memcpy != overflow_memcpy || !memcpy->requires_native_memory_va_authority ||
            memcpy->is_volatile || memcpy->selected_authority || memcpy->dst_authority ||
            memcpy->src_authority || memcpy->size_authority || !payload_type ||
            payload_type->kind != TypeKind::Struct ||
            carrier.payload_size_type != codegen::lir::LirTypeRef::integer(64) ||
            carrier.payload_size.value <= 0 || !memcpy->dst.value_id() ||
            !memcpy->src.value_id() || !memcpy->size.integer_immediate() ||
            *memcpy->dst.value_id() != carrier.destination.pointer_definition ||
            *memcpy->src.value_id() != carrier.overflow_pointer_load ||
            memcpy->size.integer_immediate()->value != carrier.payload_size.value ||
            carrier.va_list_object.owner != function.link_name_id ||
            carrier.destination.owner != function.link_name_id ||
            !carrier.va_list_object.live || !carrier.destination.live)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "memcpy requires the exact selected AMD64 SysV aggregate overflow authority row");
        continue;
      }
      const auto* authority = memcpy->selected_authority
                                  ? &*memcpy->selected_authority
                                  : nullptr;
      const auto* pointers = function.selected_memcpy_pointer_authority
                                 ? &*function.selected_memcpy_pointer_authority
                                 : nullptr;
      if (!authority || !pointers || memcpy->is_volatile ||
          function.link_name_id == c4c::kInvalidLinkName ||
          authority->destination == authority->source ||
          !authority->destination.valid() || !authority->source.valid() ||
          authority->size_type.kind() != codegen::lir::LirTypeKind::Integer ||
          authority->size_type.integer_bit_width() != std::optional<unsigned>{64} ||
          authority->size.value <= 0 ||
          !authority->destination_object.valid() || !authority->source_object.valid() ||
          authority->destination_object == authority->source_object ||
          authority->destination_object_owner != function.link_name_id ||
          authority->source_object_owner != function.link_name_id ||
          !authority->destination_live_at_site || !authority->source_live_at_site ||
          pointers->byval_parameter.role !=
              codegen::lir::LirSelectedMemcpyPointerRole::ByvalParameter ||
          pointers->destination_alloca.role !=
              codegen::lir::LirSelectedMemcpyPointerRole::DestinationAlloca ||
          pointers->byval_parameter.value != authority->source ||
          pointers->destination_alloca.value != authority->destination ||
          pointers->byval_parameter.object != authority->source_object ||
          pointers->destination_alloca.object != authority->destination_object ||
          pointers->byval_parameter.object_owner != function.link_name_id ||
          pointers->destination_alloca.object_owner != function.link_name_id ||
          pointers->byval_parameter.pointer_type.kind() !=
              codegen::lir::LirTypeKind::Pointer ||
          pointers->destination_alloca.pointer_type.kind() !=
              codegen::lir::LirTypeKind::Pointer ||
          !pointers->byval_parameter.live_at_selected_site ||
          !pointers->destination_alloca.live_at_selected_site)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "memcpy requires the one exact selected current-function authority row");
    }
  }
  if (function.selected_memcpy_pointer_authority && selected_memcpy_count != 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "selected current-function pointer authority requires exactly one memcpy row");
  if (selected_memcpy_count > 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "duplicate selected memcpy authority rows are unsupported");

  std::unordered_set<std::string> labels;
  std::unordered_set<std::uint32_t> block_ids;
  std::unordered_map<std::uint32_t, std::string> block_labels_by_id;
  std::unordered_map<std::string, Type> ordinary_values;
  std::unordered_map<std::uint32_t, Type> source_values;
  for (const auto& parameter : function.native_body_parameter_definitions) {
    // Direct-scalar rows are producer bookkeeping until the one supported
    // scalar-LHS authority selects one.  Do not make an unselected row a
    // generic SSA definition: that would accidentally admit its consumers.
    if (parameter.abi != codegen::lir::LirNativeBodyParameterAbi::DirectPointer)
      continue;
    const auto type = lower_lir_type(module, parameter.type);
    if (!type || !source_values.emplace(parameter.value.value, *type).second)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "body parameter identity collided in the current-function source registry");
  }
  if (selected_return_value_parameter_authority) {
    const auto type = lower_lir_type(module,
                                     selected_return_value_parameter_authority->type);
    if (!type || !source_values.emplace(
                      selected_return_value_parameter_authority->value.value,
                      *type).second)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "direct scalar return parameter identity collided in the current-function source registry");
  }
  if (selected_switch_selector_parameter_authority) {
    const auto type = lower_lir_type(module,
                                     selected_switch_selector_parameter_authority->type);
    if (!type || !source_values.emplace(
                      selected_switch_selector_parameter_authority->value.value,
                      *type).second)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "direct scalar switch selector parameter identity collided in the current-function source registry");
  }
  if (selected_truthiness_lhs_parameter_authority) {
    const auto type = lower_lir_type(module, selected_truthiness_lhs_parameter_authority->type);
    if (!type || !source_values.emplace(selected_truthiness_lhs_parameter_authority->value.value,
                                        *type).second)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "direct scalar truthiness parameter identity collided in the current-function source registry");
  }
  if (selected_pointer_truthiness_parameter_authority) {
    const auto type = lower_lir_type(module, selected_pointer_truthiness_parameter_authority->type);
    const auto existing =
        source_values.find(selected_pointer_truthiness_parameter_authority->value.value);
    if (!type || existing == source_values.end() || existing->second != *type)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "direct pointer truthiness parameter identity collided in the current-function source registry");
  }
  if (selected_fixed_direct_call_argument_authority) {
    const auto type = lower_lir_type(module,
                                     selected_fixed_direct_call_argument_authority->type);
    if (!type || !source_values.emplace(
                      selected_fixed_direct_call_argument_authority->value.value,
                      *type).second)
      return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                        "direct scalar fixed direct-call parameter identity collided in the current-function source registry");
  }
  for (const auto& instruction : function.alloca_insts) {
    const auto& alloca = std::get<LirAllocaOp>(instruction);
    if (!source_values.emplace(alloca.result.value_id()->value,
                               Type{TypeKind::Pointer}).second)
      return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                        "alloca result collided in the current-function source registry");
  }
  std::unordered_set<std::uint32_t> inline_asm_results;
  std::unordered_map<std::uint32_t, std::size_t> inline_asm_store_uses;
  std::unordered_set<std::uint32_t> intrinsic_results;
  std::unordered_set<std::uint32_t> native_i32_cttz_results;
  std::unordered_set<std::uint32_t> builtin_ctz_results;
  std::unordered_set<std::uint32_t> builtin_ctz_trunc_results;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctz_direct_add_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctz_trunc_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctz_trunc_add_uses;
  std::unordered_set<std::uint32_t> builtin_clz_results;
  std::unordered_set<std::uint32_t> builtin_clz_trunc_results;
  std::unordered_map<std::uint32_t, std::size_t> builtin_clz_direct_add_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_clz_trunc_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_clz_trunc_add_uses;
  std::unordered_set<std::uint32_t> builtin_ctpop_results;
  std::unordered_set<std::uint32_t> builtin_ctpop_trunc_results;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctpop_direct_add_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctpop_trunc_uses;
  std::unordered_map<std::uint32_t, std::size_t> builtin_ctpop_trunc_add_uses;
  std::unordered_set<std::uint32_t> native_ffs_cttz_results;
  std::unordered_set<std::uint32_t> builtin_ffs_add_results;
  std::unordered_set<std::uint32_t> builtin_ffs_zero_compare_results;
  std::unordered_set<std::uint32_t> native_floating_call_results;
  std::unordered_map<
      std::uint32_t,
      const codegen::lir::LirDirectZeroArgScalarFloatingCallAuthority*>
      direct_floating_call_result_authorities;
  std::unordered_map<std::uint32_t, std::size_t>
      direct_floating_call_result_authority_uses;
  std::unordered_set<std::uint32_t> downstream_double_fadd_results;
  std::unordered_set<std::uint32_t> downstream_double_fmul_results;
  std::unordered_set<std::uint32_t> scalar_fptrunc_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_fptrunc_fmul_uses;
  std::unordered_set<std::uint32_t> scalar_fpext_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_fpext_fmul_uses;
  std::unordered_set<std::uint32_t> scalar_sitofp_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_sitofp_fmul_uses;
  std::unordered_set<std::uint32_t> scalar_uitofp_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_uitofp_fmul_uses;
  std::unordered_set<std::uint32_t> scalar_fptosi_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_fptosi_add_uses;
  std::unordered_set<std::uint32_t> scalar_fptoui_results;
  std::unordered_map<std::uint32_t, std::size_t> scalar_fptoui_add_uses;
  std::unordered_set<std::uint32_t> wide_ffs_select_results;
  std::unordered_set<std::uint32_t> wide_ffs_trunc_results;
  std::unordered_map<std::uint32_t, std::size_t> wide_ffs_trunc_add_uses;
  std::unordered_set<std::uint32_t> downstream_double_olt_compare_results;
  std::unordered_map<std::uint32_t, std::size_t> downstream_double_olt_zext_uses;
  std::unordered_set<std::uint32_t> selected_global_i32_load_results;
  std::unordered_set<std::uint32_t> selected_global_i32_abs_results;
  std::unordered_set<std::uint32_t> normalized_i32_add_results;
  std::unordered_set<std::uint32_t> scalar_sext_results;
  std::size_t selected_body_parameter_gep_count = 0;
  std::size_t selected_scalar_body_parameter_lhs_count = 0;
  std::size_t selected_scalar_body_parameter_rhs_count = 0;
  std::size_t selected_pointer_truthiness_parameter_count = 0;
  labels.reserve(function.blocks.size());
  block_ids.reserve(function.blocks.size());
  block_labels_by_id.reserve(function.blocks.size());
  for (const auto& constant : function.direct_label_address_constants) {
    const auto type = lower_lir_type(module, constant.type);
    const bool target_is_current = constant.target.valid() && std::any_of(
        function.blocks.begin(), function.blocks.end(), [&](const LirBlock& block) {
          return block.id == constant.target;
        });
    if (!constant.value.valid() || !type || type->kind != TypeKind::Pointer ||
        constant.owner != function.link_name_id || !target_is_current ||
        !source_values.emplace(constant.value.value, *type).second)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "direct label-address constant must own a unique pointer value and current-function target");
  }
  for (const auto& block : function.blocks) {
    if (block.label.empty())
      return fail<void>(ImportErrorCode::EmptyBlockLabel, name, {},
                        "every imported block needs a nonempty adapter label");
    if (!labels.insert(block.label).second)
      return fail<void>(ImportErrorCode::DuplicateBlockLabel, name, block.label,
                        "block labels must be unique within a function");
    if (!block_ids.insert(block.id.value).second)
      return fail<void>(ImportErrorCode::DuplicateBlockId, name, block.label,
                        "LirBlockId values must be unique within a function");
    block_labels_by_id.emplace(block.id.value, block.label);
    for (const auto& instruction : block.insts) {
      const auto* phi = std::get_if<LirPhiOp>(&instruction);
      if (!phi) continue;
      const auto type = lower_lir_type(module, phi->type_str);
      const auto* result = phi->result.value_id();
      if (!type || !result || !result->valid() || phi->result.kind() !=
              codegen::lir::LirOperandKind::SsaValue || phi->incoming.empty() ||
          !source_values.emplace(result->value, *type).second)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "phi requires one unique authoritative typed SSA result and incoming rows");
      for (const auto& incoming : phi->incoming) {
        const bool ssa_value = incoming.value.kind() ==
                codegen::lir::LirOperandKind::SsaValue &&
            incoming.value.value_id() && incoming.value.value_id()->valid();
        const bool special_value = incoming.value.kind() ==
                codegen::lir::LirOperandKind::SpecialToken &&
            incoming.value.special_token();
        if (!incoming.predecessor.valid() || (!ssa_value && !special_value))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                            block.label,
                            "phi incoming requires current-function SSA value and predecessor authority");
      }
    }
    for (std::size_t index = 0; index < block.insts.size(); ++index) {
      if (!std::get_if<LirIndirectBrOp>(&block.insts[index])) continue;
      if (index + 1 != block.insts.size() ||
          !std::holds_alternative<LirUnreachable>(block.terminator))
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                          name, block.label,
                          "LirIndirectBrOp must be the final instruction carrier with an unreachable terminator sentinel");
    }
    for (const auto& instruction : block.insts) {
      if (std::get_if<LirPhiOp>(&instruction)) continue;
      if (std::get_if<LirMemcpyOp>(&instruction)) continue;
      if (const auto* indirect_br = std::get_if<LirIndirectBrOp>(&instruction)) {
        const auto address = indirect_br->addr_value && indirect_br->addr_value->valid()
                                 ? source_values.find(indirect_br->addr_value->value)
                                 : source_values.end();
        if (address == source_values.end() ||
            address->second.kind != TypeKind::Pointer)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "LirIndirectBrOp.addr_value must resolve to a current-function pointer value");
        if (indirect_br->successors.empty())
          return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                            block.label,
                            "LirIndirectBrOp.successors must not be empty");
        if (indirect_br->addr.kind() == codegen::lir::LirOperandKind::DirectConstant &&
            (!indirect_br->addr.value_id() || !indirect_br->addr_value ||
             *indirect_br->addr.value_id() != *indirect_br->addr_value ||
             std::none_of(function.direct_label_address_constants.begin(),
                          function.direct_label_address_constants.end(),
                          [&](const auto& constant) {
                            return constant.value == *indirect_br->addr_value;
                          })))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "direct label-address operand must match its current-function address value");
        continue;
      }
      if (const auto* constant = std::get_if<LirConstInt>(&instruction)) {
        const auto type = lower_constant_type(module, constant->type);
        if (!constant->result.valid())
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "integer constant has an invalid LirValueId");
        if (!type || (type->kind != TypeKind::Integer &&
                      type->kind != TypeKind::I1 &&
                      type->kind != TypeKind::I8 &&
                      type->kind != TypeKind::I16 &&
                      type->kind != TypeKind::I32 &&
                      type->kind != TypeKind::I64))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "integer constant has a malformed or non-integer type");
        if (!source_values.emplace(constant->result.value, *type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "duplicate authoritative LirValueId definition");
        continue;
      }
      if (const auto* constant = std::get_if<LirConstFloat>(&instruction)) {
        const auto type = lower_constant_type(module, constant->type);
        if (!constant->result.valid())
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "floating constant has an invalid LirValueId");
        if (!type || (type->kind != TypeKind::Floating &&
                      type->kind != TypeKind::F32 &&
                      type->kind != TypeKind::F64))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "floating constant has a malformed or non-floating type");
        if (!source_values.emplace(constant->result.value, *type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "duplicate authoritative LirValueId definition");
        continue;
      }
      if (const auto* store = std::get_if<LirStoreOp>(&instruction)) {
        const auto type = lower_lir_type(module, store->type_str);
        const auto* immediate = store->val.integer_immediate();
        const auto* destination = store->ptr.link_name_id();
        const auto* source = store->val.value_id();
        const bool immediate_store =
            store->val.kind() == codegen::lir::LirOperandKind::Immediate &&
            immediate && store->type_str.integer_bit_width() &&
            integer_immediate_representable(
                immediate->value, *store->type_str.integer_bit_width());
        const bool inline_asm_store =
            store->val.kind() == codegen::lir::LirOperandKind::SsaValue &&
            source && source->valid() && type &&
            is_native_inline_asm_output_type(*type) &&
            inline_asm_results.count(source->value) != 0 &&
            source_values.find(source->value) != source_values.end() &&
            source_values.at(source->value) == *type;
        const auto* authority = store->local_object_authority
            ? &*store->local_object_authority : nullptr;
        const auto* pointer = store->ptr.value_id();
        const auto pointer_type = authority ? lower_lir_type(module, authority->pointer_type)
                                             : std::optional<Type>{};
        const auto pointee_type = authority ? lower_lir_type(module, authority->pointee_type)
                                             : std::optional<Type>{};
        const bool local_store = store->requires_native_store_authority && authority && type &&
            is_integer_type(*type) && store->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
            immediate && store->type_str.integer_bit_width() &&
            integer_immediate_representable(immediate->value, *store->type_str.integer_bit_width()) &&
            store->ptr.kind() == codegen::lir::LirOperandKind::SsaValue && pointer && pointer->valid() &&
            authority->pointer_definition == *pointer && authority->object.valid() &&
            authority->owner == function.link_name_id && pointer_type &&
            *pointer_type == Type{TypeKind::Pointer} && pointee_type && *pointee_type == *type && authority->live;
        if (local_store) continue;
        if (store->requires_native_store_authority || authority)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "local store authority must form the one selected native local-scalar store shape");
        if (!type || !is_integer_type(*type) ||
            store->type_str.kind() !=
                codegen::lir::LirTypeKind::Integer ||
            (!immediate_store && !inline_asm_store) ||
            store->ptr.kind() != codegen::lir::LirOperandKind::Global ||
            !destination || *destination == c4c::kInvalidLinkName) {
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "store requires an authoritative integer immediate and direct-global LinkNameId");
        }
        const LirGlobal* selected = nullptr;
        for (const auto& global : module.globals) {
          if (global.link_name_id != *destination) continue;
          if (selected)
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "store destination LinkNameId has ambiguous global ownership");
          selected = &global;
        }
        const auto global_type =
            selected ? lower_global_type(module, *selected) : std::nullopt;
        if (!selected || !global_type || *global_type != *type)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "store destination must resolve to one exactly typed global object");
        if (inline_asm_store && ++inline_asm_store_uses[source->value] != 1)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "inline-asm result must have exactly one Store use");
        continue;
      }
      if (const auto* load = std::get_if<LirLoadOp>(&instruction)) {
        const auto type = lower_lir_type(module, load->type_str);
        const auto* result = load->result.value_id();
        const bool overflow_source_load = has_overflow_carrier && result &&
            *result == overflow_carrier->overflow_pointer_load;
        const bool overflow_final_load = has_overflow_carrier && result &&
            *result == overflow_carrier->final_load;
        if (overflow_source_load || overflow_final_load) {
          const auto expected_type = overflow_source_load
              ? std::optional<Type>{Type{TypeKind::Pointer}}
              : lower_lir_type(module, overflow_carrier->payload_type);
          const auto expected_pointer = overflow_source_load
              ? overflow_carrier->overflow_field_address
              : overflow_carrier->destination.pointer_definition;
          if (!type || !expected_type || *type != *expected_type ||
              load->result.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !result->valid() || load->ptr.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !load->ptr.value_id() || *load->ptr.value_id() != expected_pointer ||
              load->local_object_authority || load->requires_native_result_authority ||
              !source_values.emplace(result->value, *type).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                              "AMD64 SysV overflow aggregate loads require their exact selected native chain");
          continue;
        }
        const auto* source = load->ptr.link_name_id();
        const auto* pointer = load->ptr.value_id();
        const auto* authority = load->local_object_authority
            ? &*load->local_object_authority : nullptr;
        const auto pointer_type = authority
            ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
        const auto pointee_type = authority
            ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
        const auto* selected_alloca = function.alloca_insts.size() == 1
            ? std::get_if<LirAllocaOp>(&function.alloca_insts.front()) : nullptr;
        const auto* alloca_authority = selected_alloca &&
                selected_alloca->local_object_authority
            ? &*selected_alloca->local_object_authority : nullptr;
        const bool authority_coherent = authority && alloca_authority &&
            authority->pointer_definition == alloca_authority->pointer_definition &&
            authority->object == alloca_authority->object &&
            authority->owner == alloca_authority->owner &&
            authority->pointer_type == alloca_authority->pointer_type &&
            authority->pointee_type == alloca_authority->pointee_type &&
            authority->live == alloca_authority->live;
        const bool local_scalar = authority && load->requires_native_result_authority &&
            type && is_local_scalar_load_type(*type) &&
            load->result.kind() == codegen::lir::LirOperandKind::SsaValue &&
            result && result->valid() &&
            load->ptr.kind() == codegen::lir::LirOperandKind::SsaValue &&
            pointer && pointer->valid() &&
            authority->pointer_definition == *pointer && authority->object.valid() &&
            function.link_name_id != c4c::kInvalidLinkName &&
            authority->owner == function.link_name_id && authority_coherent && pointer_type &&
            *pointer_type == Type{TypeKind::Pointer} && pointee_type &&
            *pointee_type == *type && authority->live;
        if (local_scalar) {
          if (!source_values.emplace(result->value, *type).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                              "duplicate authoritative LirValueId definition");
          continue;
        }
        if (authority)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "local load authority must form the one selected native local-scalar load shape");
        if (!type || !is_integer_type(*type) ||
            load->type_str.kind() != codegen::lir::LirTypeKind::Integer ||
            load->result.kind() != codegen::lir::LirOperandKind::SsaValue ||
            !result || !result->valid() ||
            load->ptr.kind() != codegen::lir::LirOperandKind::Global ||
            !source || *source == c4c::kInvalidLinkName) {
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "load requires an authoritative LirValueId result and direct-global LinkNameId");
        }
        const LirGlobal* selected = nullptr;
        for (const auto& global : module.globals) {
          if (global.link_name_id != *source) continue;
          if (selected)
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "load source LinkNameId has ambiguous global ownership");
          selected = &global;
        }
        const auto global_type =
            selected ? lower_global_type(module, *selected) : std::nullopt;
        if (!selected || !global_type || *global_type != *type)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "load source must resolve to one exactly typed global object");
        if (!source_values.emplace(result->value, *type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "duplicate authoritative LirValueId definition");
        if (*type == Type{TypeKind::Integer, 32, "i32"})
          selected_global_i32_load_results.insert(result->value);
        continue;
      }
      if (const auto* stack_save = std::get_if<codegen::lir::LirStackSaveOp>(&instruction)) {
        const auto* authority = stack_save->local_object_authority
            ? &*stack_save->local_object_authority : nullptr;
        const auto* result = stack_save->result.value_id();
        const auto pointer_type = authority
            ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
        const auto pointee_type = authority
            ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
        const bool selected = stack_save->requires_native_stack_save_authority && authority &&
            stack_save->result.kind() == codegen::lir::LirOperandKind::SsaValue &&
            result && result->valid() && *result == authority->pointer_definition &&
            authority->object.valid() && function.link_name_id != c4c::kInvalidLinkName &&
            authority->owner == function.link_name_id &&
            pointer_type == Type{TypeKind::Pointer} &&
            pointee_type == Type{TypeKind::Pointer} && authority->live;
        if (!selected)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "stack save requires the one selected native VLA saved-pointer and live current-function authority shape");
        if (!source_values.emplace(result->value, Type{TypeKind::Pointer}).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                            block.label, "duplicate authoritative LirValueId definition");
        continue;
      }
      if (const auto* stack_restore = std::get_if<codegen::lir::LirStackRestoreOp>(&instruction)) {
        const auto* authority = stack_restore->local_object_authority
            ? &*stack_restore->local_object_authority : nullptr;
        const auto* saved = stack_restore->saved_ptr.value_id();
        const auto pointer_type = authority
            ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
        const auto pointee_type = authority
            ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
        const auto transition = stack_restore->lifetime_transition
            ? &*stack_restore->lifetime_transition : nullptr;
        const bool selected = stack_restore->requires_native_stack_restore_authority && authority &&
            stack_restore->saved_ptr.kind() == codegen::lir::LirOperandKind::SsaValue &&
            saved && saved->valid() && *saved == authority->pointer_definition &&
            authority->object.valid() && function.link_name_id != c4c::kInvalidLinkName &&
            authority->owner == function.link_name_id && pointer_type == Type{TypeKind::Pointer} &&
            pointee_type == Type{TypeKind::Pointer} && authority->live && transition &&
            transition->kind == codegen::lir::LirStackRestoreOp::LirStackRestoreLifetimeTransition::Kind::RestoreSavedVlaStackCheckpoint &&
            transition->saved_pointer_definition == *saved &&
            source_values.count(saved->value) == 1;
        if (!selected)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "stack restore requires the one selected native live VLA checkpoint transition and matching saved-pointer authority");
        continue;
      }
      if (const auto* va_start = std::get_if<codegen::lir::LirVaStartOp>(&instruction)) {
        const auto* authority = va_start->ap_authority
            ? &va_start->ap_authority->local_pointer : nullptr;
        const auto* ap = va_start->ap_ptr.value_id();
        const auto pointer_type = authority
            ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
        const auto pointee_type = authority
            ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
        const bool selected = va_start->requires_native_memory_va_authority && authority &&
            va_start->ap_ptr.kind() == codegen::lir::LirOperandKind::SsaValue &&
            ap && ap->valid() && *ap == authority->pointer_definition &&
            authority->object.valid() && function.link_name_id != c4c::kInvalidLinkName &&
            authority->owner == function.link_name_id &&
            pointer_type == Type{TypeKind::Pointer} && pointee_type &&
            is_well_formed(*pointee_type) && pointee_type->kind != TypeKind::Void &&
            authority->live && source_values.count(ap->value) == 1;
        if (!selected)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "va_start requires the selected direct-local live va_list pointer authority shape");
        continue;
      }
      if (const auto* gep = std::get_if<LirGepOp>(&instruction)) {
        const auto* overflow_result = gep->result.value_id();
        if (has_overflow_carrier && overflow_result &&
            *overflow_result == overflow_carrier->overflow_field_address) {
          const auto typed_index = [](const codegen::lir::LirGepIndex& index,
                                      long long expected) {
            return index.is_authoritative() &&
                index.type_ref() == codegen::lir::LirTypeRef::integer(32) &&
                index.value().integer_immediate() &&
                index.value().integer_immediate()->value == expected;
          };
          if (gep->element_type.kind() != codegen::lir::LirTypeKind::Struct ||
              gep->result.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !overflow_result->valid() || gep->ptr.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !gep->ptr.value_id() ||
              *gep->ptr.value_id() != overflow_carrier->va_list_object.pointer_definition ||
              gep->indices.size() != 2 || !typed_index(gep->indices[0], 0) ||
              !typed_index(gep->indices[1], 2) || gep->local_object_authority ||
              gep->requires_native_local_gep_authority || gep->requires_native_result_authority ||
              source_values.count(overflow_carrier->va_list_object.pointer_definition.value) != 1 ||
              !source_values.emplace(overflow_result->value, Type{TypeKind::Pointer}).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                              "AMD64 SysV overflow aggregate field address requires the direct typed field-2 chain");
          continue;
        }
        if (gep->requires_native_local_gep_authority) {
          const auto* authority = gep->local_object_authority
              ? &*gep->local_object_authority : nullptr;
          const auto* result = gep->result.value_id();
          const auto* base = gep->ptr.value_id();
          const auto index_type = gep->indices.size() == 1
              ? lower_lir_type(module, gep->indices.front().type_ref())
              : std::optional<Type>{};
          const auto element_type = lower_lir_type(module, gep->element_type);
          const auto indexed_element_type = authority && authority->indexed_element_type
              ? lower_lir_type(module, *authority->indexed_element_type)
              : std::optional<Type>{};
          const auto pointer_type = authority
              ? lower_lir_type(module, authority->pointer_type) : std::optional<Type>{};
          const auto pointee_type = authority
              ? lower_lir_type(module, authority->pointee_type) : std::optional<Type>{};
          const auto* selected_alloca = function.alloca_insts.size() == 1
              ? std::get_if<LirAllocaOp>(&function.alloca_insts.front()) : nullptr;
          const auto* alloca_authority = selected_alloca &&
                  selected_alloca->local_object_authority
              ? &*selected_alloca->local_object_authority : nullptr;
          const auto base_value = base ? source_values.find(base->value) : source_values.end();
          const auto immediate = gep->indices.size() == 1 &&
                  gep->indices.front().is_authoritative()
              ? gep->indices.front().value().integer_immediate() : nullptr;
          const bool authority_coherent = authority && alloca_authority &&
              authority->pointer_definition == alloca_authority->pointer_definition &&
              authority->object == alloca_authority->object &&
              authority->owner == alloca_authority->owner &&
              authority->pointer_type == alloca_authority->pointer_type &&
              authority->pointee_type == alloca_authority->pointee_type &&
              authority->live == alloca_authority->live;
          const bool selected_local = gep->requires_native_result_authority && authority &&
              gep->result.kind() == codegen::lir::LirOperandKind::SsaValue &&
              result && result->valid() && gep->ptr.kind() ==
                  codegen::lir::LirOperandKind::SsaValue && base && base->valid() &&
              base->value == authority->pointer_definition.value &&
              base_value != source_values.end() &&
              base_value->second.kind == TypeKind::Pointer && gep->indices.size() == 1 &&
              gep->indices.front().is_authoritative() &&
              gep->indices.front().type_ref() == codegen::lir::LirTypeRef::integer(64) &&
              index_type == Type{TypeKind::Integer, 64, "i64"} && immediate &&
              element_type && indexed_element_type && *element_type == *indexed_element_type &&
              pointer_type == Type{TypeKind::Pointer} && pointee_type &&
              pointee_type->kind == TypeKind::Array && authority->object.valid() &&
              function.link_name_id != c4c::kInvalidLinkName &&
              authority->owner == function.link_name_id && authority_coherent && authority->live;
          if (!selected_local)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                              block.label,
                              "local-array getelementptr requires the one selected native result, SSA base, i64 immediate, and live current-function authority shape");
          if (!source_values.emplace(result->value, Type{TypeKind::Pointer}).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                              block.label, "duplicate authoritative LirValueId definition");
          continue;
        }
        const auto* result = gep->result.value_id();
        const auto* parameter_base = gep->ptr.value_id();
        const auto selected_parameter = parameter_base
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(),
                           [&](const auto& parameter) {
                             return parameter.value == *parameter_base &&
                                 parameter.abi ==
                                     codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
                           })
            : function.native_body_parameter_definitions.end();
        if (selected_parameter != function.native_body_parameter_definitions.end()) {
          const auto element_type = lower_lir_type(module, gep->element_type);
          if (gep->result.kind() != codegen::lir::LirOperandKind::SsaValue || !result ||
              !result->valid() || gep->ptr.kind() != codegen::lir::LirOperandKind::SsaValue ||
              !parameter_base->valid() || selected_parameter->parameter_index != 0 ||
              selected_parameter->type.kind() != codegen::lir::LirTypeKind::Pointer ||
              selected_parameter->owner != function.link_name_id ||
              selected_parameter->abi != codegen::lir::LirNativeBodyParameterAbi::DirectPointer ||
              !element_type || gep->indices.empty() ||
              ++selected_body_parameter_gep_count != 1 ||
              !source_values.emplace(result->value, Type{TypeKind::Pointer}).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                              "direct body-parameter getelementptr requires the one selected typed parameter authority row");
          continue;
        }
        const auto* global_base = gep->ptr.link_name_id();
        const auto* direct_base = gep->ptr.value_id();
        const bool is_global_base =
            gep->ptr.kind() == codegen::lir::LirOperandKind::Global &&
            global_base && *global_base != c4c::kInvalidLinkName;
        const bool is_direct_label_base =
            gep->ptr.kind() == codegen::lir::LirOperandKind::DirectConstant &&
            direct_base && direct_base->valid() &&
            source_values.find(direct_base->value) != source_values.end() &&
            source_values.at(direct_base->value).kind == TypeKind::Pointer &&
            std::any_of(function.direct_label_address_constants.begin(),
                        function.direct_label_address_constants.end(),
                        [&](const auto& constant) {
                          return constant.value == *direct_base;
                        });
        if (gep->element_type.kind() !=
                codegen::lir::LirTypeKind::Array ||
            gep->result.kind() != codegen::lir::LirOperandKind::SsaValue ||
            !result || !result->valid() ||
            (!is_global_base && !is_direct_label_base) || gep->indices.empty()) {
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "getelementptr requires an authoritative result, a global or current-function direct label-address base, and nonempty typed array indices");
        }
        if (is_global_base) {
        const LirGlobal* selected = nullptr;
        for (const auto& global : module.globals) {
          if (global.link_name_id != *global_base) continue;
          if (selected)
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "getelementptr base LinkNameId has ambiguous global ownership");
          selected = &global;
        }
        const auto global_type =
            selected ? lower_global_type(module, *selected) : std::nullopt;
        if (!selected || !global_type || global_type->kind != TypeKind::Array ||
            gep->element_type.str() != global_type->spelling)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "getelementptr element type must exactly identify its selected global array object");
        }
        for (const auto& index : gep->indices) {
          if (!index.is_authoritative())
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "getelementptr cannot mix raw and authoritative indices");
          const auto index_type = lower_lir_type(module, index.type_ref());
          if (!index_type || !is_integer_type(*index_type) ||
              index.type_ref().kind() !=
                  codegen::lir::LirTypeKind::Integer)
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "getelementptr index type must be an exact integer type");
          const auto& operand = index.value();
          if (operand.kind() == codegen::lir::LirOperandKind::Immediate) {
            const auto* immediate = operand.integer_immediate();
            if (!immediate || !index.type_ref().integer_bit_width() ||
                !integer_immediate_representable(
                    immediate->value,
                    *index.type_ref().integer_bit_width()))
              return fail<void>(
                  ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                  block.label,
                  "getelementptr immediate index lacks exact in-range authority");
          } else if (operand.kind() ==
                     codegen::lir::LirOperandKind::SsaValue) {
            const auto* value_id = operand.value_id();
            const auto found = value_id
                                   ? source_values.find(value_id->value)
                                   : source_values.end();
            if (!value_id || !value_id->valid() ||
                found == source_values.end() || found->second != *index_type)
              return fail<void>(
                  ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                  block.label,
                  "getelementptr SSA index must resolve in the current-function source registry with exact type");
          } else {
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "getelementptr index has an unsupported authority alternative");
          }
        }
        if (!source_values
                 .emplace(result->value, Type{TypeKind::Pointer})
                 .second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "duplicate authoritative LirValueId definition");
        continue;
      }
      if (const auto* call = std::get_if<LirCallOp>(&instruction)) {
        if (call->intrinsic_kind) {
          if (!exact_integer_intrinsic_call(module, *call, source_values))
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                              "intrinsic call requires one native exact integer shape with LinkNameId, immediate/current-function SSA value, and matching i1 behavior flag");
          const auto result_type = lower_lir_type(module, call->return_type);
          if (!source_values.emplace(call->result.value_id()->value, *result_type).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                            "duplicate authoritative LirValueId definition");
          intrinsic_results.insert(call->result.value_id()->value);
          if (exact_builtin_ctz_call(module, *call, source_values))
            builtin_ctz_results.insert(call->result.value_id()->value);
          if (exact_builtin_clz_call(module, *call, source_values))
            builtin_clz_results.insert(call->result.value_id()->value);
          if (exact_builtin_popcount_call(module, *call, source_values))
            builtin_ctpop_results.insert(call->result.value_id()->value);
          if (*call->intrinsic_kind == LirIntrinsicKind::Cttz &&
              *result_type == Type{TypeKind::Integer, 32, "i32"})
            native_i32_cttz_results.insert(call->result.value_id()->value);
          if (*call->intrinsic_kind == LirIntrinsicKind::Cttz &&
              call->zero_count_behavior == codegen::lir::LirZeroCountBehavior::Defined &&
              (*result_type == Type{TypeKind::Integer, 32, "i32"} ||
               *result_type == Type{TypeKind::Integer, 64, "i64"}))
            native_ffs_cttz_results.insert(call->result.value_id()->value);
          continue;
        }
        if (exact_direct_void_call(module, *call)) continue;
        if (exact_direct_native_floating_call(module, *call)) {
          if (call->direct_zero_arg_scalar_floating_call_authority) {
            const auto& authority =
                *call->direct_zero_arg_scalar_floating_call_authority;
            if (!exact_direct_zero_arg_scalar_floating_call_authority(
                    module, function, *call, authority))
              return fail<void>(
                  ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                  block.label,
                  "direct native floating call-result authority tuple is incoherent");
            if (!direct_floating_call_result_authorities
                     .emplace(authority.result.value, &authority)
                     .second)
              return fail<void>(
                  ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                  block.label,
                  "duplicate direct native floating call-result authority");
          }
          const auto result_type = native_floating_call_type(module, call->return_type);
          if (!source_values.emplace(call->result.value_id()->value, *result_type).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                            "duplicate authoritative LirValueId definition");
          native_floating_call_results.insert(call->result.value_id()->value);
          continue;
        }
        if (exact_direct_one_double_arg_scalar_floating_call(module, *call)) {
          if (!call->direct_one_double_arg_scalar_floating_call_authority) {
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "direct one-double-argument scalar floating call result requires its one typed authority row");
          }
          const auto& authority =
              *call->direct_one_double_arg_scalar_floating_call_authority;
          if (!exact_direct_one_double_arg_scalar_floating_call_authority(
                  function, *call, authority))
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "direct one-double-argument scalar floating call-result authority tuple is incoherent");
          const auto result_type = native_floating_call_type(module, call->return_type);
          if (!source_values.emplace(call->result.value_id()->value, *result_type).second)
            return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                            "duplicate authoritative LirValueId definition");
          native_floating_call_results.insert(call->result.value_id()->value);
          continue;
        }
        if (!exact_direct_integer_call(module, *call, source_values))
          return fail<void>(
              ImportErrorCode::UnsupportedOrdinaryInstruction, name,
              block.label,
              "call requires a structured direct LinkNameId target, exact fixed nonvariadic integer signature, and immediate or current-function SSA arguments");
        const auto result_type = lower_lir_type(module, call->return_type);
        if (!source_values.emplace(call->result.value_id()->value, *result_type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "duplicate authoritative LirValueId definition");
        continue;
      }
      if (const auto* bin = std::get_if<LirBinOp>(&instruction)) {
        const auto* scalar_authority = bin->scalar_lhs_parameter_authority
            ? &*bin->scalar_lhs_parameter_authority : nullptr;
        const auto* scalar_rhs_authority = bin->scalar_rhs_parameter_authority
            ? &*bin->scalar_rhs_parameter_authority : nullptr;
        const auto scalar_definition = scalar_authority
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(), [&](const auto& definition) {
                return definition.value == scalar_authority->value &&
                    definition.parameter_index == scalar_authority->parameter_index &&
                    definition.type == scalar_authority->type && definition.owner == scalar_authority->owner &&
                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
              }) : function.native_body_parameter_definitions.end();
        const auto scalar_definition_count = scalar_authority
            ? std::count_if(function.native_body_parameter_definitions.begin(),
                            function.native_body_parameter_definitions.end(), [&](const auto& definition) {
                return definition.value == scalar_authority->value &&
                    definition.parameter_index == scalar_authority->parameter_index &&
                    definition.type == scalar_authority->type && definition.owner == scalar_authority->owner &&
                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
              }) : 0;
        const bool selected_scalar_lhs_add = scalar_authority &&
            scalar_definition != function.native_body_parameter_definitions.end() &&
            scalar_definition_count == 1 &&
            scalar_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_authority->role == codegen::lir::LirScalarBinaryParameterRole::Lhs &&
            scalar_authority->owner == function.link_name_id &&
            scalar_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_authority->parameter_index] == scalar_authority->type &&
            bin->lhs.value_id() && *bin->lhs.value_id() == scalar_authority->value &&
            bin->type_str == scalar_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::Add} &&
            bin->type_str == codegen::lir::LirTypeRef::integer(32) &&
            bin->rhs.integer_immediate() && bin->rhs.integer_immediate()->value == 1;
        const auto selected_scalar_lhs_type = scalar_authority
            ? lower_lir_type(module, scalar_authority->type)
            : std::nullopt;
        const bool selected_scalar_lhs_fneg = scalar_authority &&
            scalar_definition != function.native_body_parameter_definitions.end() &&
            scalar_definition_count == 1 &&
            scalar_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_authority->role == codegen::lir::LirScalarBinaryParameterRole::Lhs &&
            scalar_authority->owner == function.link_name_id &&
            scalar_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_authority->parameter_index] == scalar_authority->type &&
            bin->lhs.value_id() && *bin->lhs.value_id() == scalar_authority->value &&
            bin->type_str == scalar_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FNeg} &&
            selected_scalar_lhs_type && is_local_scalar_load_type(*selected_scalar_lhs_type) &&
            selected_scalar_lhs_type->kind != TypeKind::Integer &&
            bin->rhs.empty() && !bin->rhs.has_authority();
        const auto* scalar_lhs_fmul_rhs = bin->rhs.value_id();
        const auto scalar_lhs_fmul_rhs_value = scalar_lhs_fmul_rhs
            ? source_values.find(scalar_lhs_fmul_rhs->value)
            : source_values.end();
        const bool selected_scalar_lhs_fadd = scalar_authority &&
            scalar_definition != function.native_body_parameter_definitions.end() &&
            scalar_definition_count == 1 &&
            scalar_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_authority->role == codegen::lir::LirScalarBinaryParameterRole::Lhs &&
            scalar_authority->owner == function.link_name_id &&
            scalar_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_authority->parameter_index] == scalar_authority->type &&
            bin->lhs.value_id() && *bin->lhs.value_id() == scalar_authority->value &&
            bin->type_str == scalar_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FAdd} &&
            selected_scalar_lhs_type && is_local_scalar_load_type(*selected_scalar_lhs_type) &&
            selected_scalar_lhs_type->kind != TypeKind::Integer &&
            scalar_lhs_fmul_rhs && scalar_lhs_fmul_rhs_value != source_values.end() &&
            scalar_lhs_fmul_rhs_value->second == *selected_scalar_lhs_type;
        const bool selected_scalar_lhs_fsub = scalar_authority &&
            scalar_definition != function.native_body_parameter_definitions.end() &&
            scalar_definition_count == 1 &&
            scalar_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_authority->role == codegen::lir::LirScalarBinaryParameterRole::Lhs &&
            scalar_authority->owner == function.link_name_id &&
            scalar_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_authority->parameter_index] == scalar_authority->type &&
            bin->lhs.value_id() && *bin->lhs.value_id() == scalar_authority->value &&
            bin->type_str == scalar_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FSub} &&
            selected_scalar_lhs_type && is_local_scalar_load_type(*selected_scalar_lhs_type) &&
            selected_scalar_lhs_type->kind != TypeKind::Integer &&
            scalar_lhs_fmul_rhs && scalar_lhs_fmul_rhs_value != source_values.end() &&
            scalar_lhs_fmul_rhs_value->second == *selected_scalar_lhs_type;
        const bool selected_scalar_lhs_fmul = scalar_authority &&
            scalar_definition != function.native_body_parameter_definitions.end() &&
            scalar_definition_count == 1 &&
            scalar_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_authority->role == codegen::lir::LirScalarBinaryParameterRole::Lhs &&
            scalar_authority->owner == function.link_name_id &&
            scalar_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_authority->parameter_index] == scalar_authority->type &&
            bin->lhs.value_id() && *bin->lhs.value_id() == scalar_authority->value &&
            bin->type_str == scalar_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
            selected_scalar_lhs_type && is_local_scalar_load_type(*selected_scalar_lhs_type) &&
            selected_scalar_lhs_type->kind != TypeKind::Integer &&
            scalar_lhs_fmul_rhs && scalar_lhs_fmul_rhs_value != source_values.end() &&
            scalar_lhs_fmul_rhs_value->second == *selected_scalar_lhs_type;
        const bool selected_scalar_lhs =
            selected_scalar_lhs_add || selected_scalar_lhs_fneg ||
            selected_scalar_lhs_fadd || selected_scalar_lhs_fsub ||
            selected_scalar_lhs_fmul;
        const auto selected_scalar_lhs_fmul_type = [&]() -> std::optional<Type> {
          if (!selected_scalar_lhs_fmul || !selected_scalar_lhs_type)
            return std::nullopt;
          if (selected_scalar_lhs_type->spelling == "float")
            return Type{TypeKind::F32, 32, "float"};
          if (selected_scalar_lhs_type->spelling == "double")
            return Type{TypeKind::F64, 64, "double"};
          return *selected_scalar_lhs_type;
        }();
        const auto scalar_rhs_definition = scalar_rhs_authority
            ? std::find_if(function.native_body_parameter_definitions.begin(),
                           function.native_body_parameter_definitions.end(), [&](const auto& definition) {
                return definition.value == scalar_rhs_authority->value &&
                    definition.parameter_index == scalar_rhs_authority->parameter_index &&
                    definition.type == scalar_rhs_authority->type && definition.owner == scalar_rhs_authority->owner &&
                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
              }) : function.native_body_parameter_definitions.end();
        const auto scalar_rhs_definition_count = scalar_rhs_authority
            ? std::count_if(function.native_body_parameter_definitions.begin(),
                            function.native_body_parameter_definitions.end(), [&](const auto& definition) {
                return definition.value == scalar_rhs_authority->value &&
                    definition.parameter_index == scalar_rhs_authority->parameter_index &&
                    definition.type == scalar_rhs_authority->type && definition.owner == scalar_rhs_authority->owner &&
                    definition.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
              }) : 0;
        const auto selected_scalar_rhs_type = scalar_rhs_authority
            ? lower_lir_type(module, scalar_rhs_authority->type)
            : std::nullopt;
        const auto* scalar_rhs_fmul_lhs = bin->lhs.value_id();
        const auto scalar_rhs_fmul_lhs_value = scalar_rhs_fmul_lhs
            ? source_values.find(scalar_rhs_fmul_lhs->value)
            : source_values.end();
        const bool selected_scalar_rhs_add = scalar_rhs_authority &&
            scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
            scalar_rhs_definition_count == 1 &&
            scalar_rhs_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_rhs_authority->role == codegen::lir::LirScalarBinaryParameterRole::Rhs &&
            scalar_rhs_authority->owner == function.link_name_id &&
            scalar_rhs_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_rhs_authority->parameter_index] == scalar_rhs_authority->type &&
            bin->rhs.value_id() && *bin->rhs.value_id() == scalar_rhs_authority->value &&
            bin->type_str == scalar_rhs_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::Add} &&
            bin->type_str == codegen::lir::LirTypeRef::integer(32) &&
            bin->lhs.integer_immediate() && bin->lhs.integer_immediate()->value == 1;
        const bool selected_scalar_rhs_fmul = scalar_rhs_authority &&
            scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
            scalar_rhs_definition_count == 1 &&
            scalar_rhs_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_rhs_authority->role == codegen::lir::LirScalarBinaryParameterRole::Rhs &&
            scalar_rhs_authority->owner == function.link_name_id &&
            scalar_rhs_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_rhs_authority->parameter_index] ==
                scalar_rhs_authority->type &&
            bin->rhs.value_id() && *bin->rhs.value_id() == scalar_rhs_authority->value &&
            bin->type_str == scalar_rhs_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
            selected_scalar_rhs_type && is_local_scalar_load_type(*selected_scalar_rhs_type) &&
            selected_scalar_rhs_type->kind != TypeKind::Integer &&
            scalar_rhs_fmul_lhs && scalar_rhs_fmul_lhs_value != source_values.end() &&
            scalar_rhs_fmul_lhs_value->second == *selected_scalar_rhs_type;
        const bool selected_scalar_rhs_fadd = scalar_rhs_authority &&
            scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
            scalar_rhs_definition_count == 1 &&
            scalar_rhs_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_rhs_authority->role == codegen::lir::LirScalarBinaryParameterRole::Rhs &&
            scalar_rhs_authority->owner == function.link_name_id &&
            scalar_rhs_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_rhs_authority->parameter_index] ==
                scalar_rhs_authority->type &&
            bin->rhs.value_id() && *bin->rhs.value_id() == scalar_rhs_authority->value &&
            bin->type_str == scalar_rhs_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FAdd} &&
            selected_scalar_rhs_type && is_local_scalar_load_type(*selected_scalar_rhs_type) &&
            selected_scalar_rhs_type->kind != TypeKind::Integer &&
            scalar_rhs_fmul_lhs && scalar_rhs_fmul_lhs_value != source_values.end() &&
            scalar_rhs_fmul_lhs_value->second == *selected_scalar_rhs_type;
        const bool selected_scalar_rhs_fsub = scalar_rhs_authority &&
            scalar_rhs_definition != function.native_body_parameter_definitions.end() &&
            scalar_rhs_definition_count == 1 &&
            scalar_rhs_authority->abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar &&
            scalar_rhs_authority->role == codegen::lir::LirScalarBinaryParameterRole::Rhs &&
            scalar_rhs_authority->owner == function.link_name_id &&
            scalar_rhs_authority->parameter_index < function.signature_param_type_refs.size() &&
            function.signature_param_type_refs[scalar_rhs_authority->parameter_index] ==
                scalar_rhs_authority->type &&
            bin->rhs.value_id() && *bin->rhs.value_id() == scalar_rhs_authority->value &&
            bin->type_str == scalar_rhs_authority->type &&
            bin->opcode.typed() == std::optional{codegen::lir::LirBinaryOpcode::FSub} &&
            selected_scalar_rhs_type && is_local_scalar_load_type(*selected_scalar_rhs_type) &&
            selected_scalar_rhs_type->kind != TypeKind::Integer &&
            scalar_rhs_fmul_lhs && scalar_rhs_fmul_lhs_value != source_values.end() &&
            scalar_rhs_fmul_lhs_value->second == *selected_scalar_rhs_type;
        const bool selected_scalar_rhs =
            selected_scalar_rhs_add || selected_scalar_rhs_fadd ||
            selected_scalar_rhs_fsub || selected_scalar_rhs_fmul;
        if (scalar_authority && (!selected_scalar_lhs || ++selected_scalar_body_parameter_lhs_count != 1))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "direct scalar body-parameter binary requires the one exact typed LHS authority row");
        if (scalar_rhs_authority && (!selected_scalar_rhs || ++selected_scalar_body_parameter_rhs_count != 1))
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "direct scalar body-parameter binary requires the one exact typed RHS authority row");
        const bool fadd = exact_downstream_double_fadd(
            *bin, source_values, native_floating_call_results);
        if (fadd) {
          const auto* lhs = bin->lhs.value_id();
          const auto authority = lhs
              ? direct_floating_call_result_authorities.find(lhs->value)
              : direct_floating_call_result_authorities.end();
          if (!lhs ||
              authority == direct_floating_call_result_authorities.end() ||
              ++direct_floating_call_result_authority_uses[lhs->value] != 1)
            return fail<void>(
                ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                block.label,
                "floating binary LHS requires the one exact direct call-result authority row");
        }
        const bool fmul = exact_downstream_double_fmul(
            *bin, source_values, downstream_double_fadd_results);
        const bool float_fmul = exact_downstream_float_fmul(
            *bin, source_values, scalar_fptrunc_results);
        const bool fpext_fmul = exact_downstream_double_fpext_fmul(
            *bin, source_values, scalar_fpext_results);
        const bool sitofp_fmul = exact_downstream_double_sitofp_fmul(
            *bin, source_values, scalar_sitofp_results);
        const bool uitofp_fmul = exact_downstream_double_uitofp_fmul(
            *bin, source_values, scalar_uitofp_results);
        const bool fptosi_add = exact_downstream_i32_fptosi_add(
            *bin, source_values, scalar_fptosi_results);
        const bool fptoui_add = exact_downstream_i32_fptoui_add(
            *bin, source_values, scalar_fptoui_results);
        const bool wide_ffs_trunc_add = exact_downstream_i32_fptosi_add(
            *bin, source_values, wide_ffs_trunc_results);
        const bool fneg = selected_scalar_lhs_fneg;
        const bool direct_scalar_lhs_fadd = selected_scalar_lhs_fadd;
        const bool direct_scalar_lhs_fsub = selected_scalar_lhs_fsub;
        const bool direct_scalar_lhs_fmul = selected_scalar_lhs_fmul;
        const bool direct_scalar_rhs_fadd = selected_scalar_rhs_fadd;
        const bool direct_scalar_rhs_fsub = selected_scalar_rhs_fsub;
        const bool direct_scalar_rhs_fmul = selected_scalar_rhs_fmul;
        const bool add = selected_scalar_lhs_add || selected_scalar_rhs_add || exact_normalized_i32_add(
            *bin, source_values, selected_global_i32_load_results) ||
            exact_native_i32_cttz_add(
                *bin, source_values, native_i32_cttz_results) ||
            [&] {
              const auto* lhs = bin->lhs.value_id();
              return lhs && selected_global_i32_abs_results.count(lhs->value) == 1 &&
                  exact_normalized_i32_add(*bin, source_values,
                      selected_global_i32_abs_results);
            }();
        const auto* add_lhs = bin->lhs.value_id();
        const bool ctz_direct_add = add_lhs &&
            builtin_ctz_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_ctz_results);
        const bool ctz_trunc_add = add_lhs &&
            builtin_ctz_trunc_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_ctz_trunc_results);
        const bool clz_direct_add = add_lhs &&
            builtin_clz_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_clz_results);
        const bool clz_trunc_add = add_lhs &&
            builtin_clz_trunc_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_clz_trunc_results);
        const bool ctpop_direct_add = add_lhs &&
            builtin_ctpop_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_ctpop_results);
        const bool ctpop_trunc_add = add_lhs &&
            builtin_ctpop_trunc_results.count(add_lhs->value) == 1 &&
            exact_native_i32_cttz_add(*bin, source_values, builtin_ctpop_trunc_results);
        const bool ffs_add = exact_builtin_ffs_add(*bin, source_values,
                                                    native_ffs_cttz_results);
        const bool mul = exact_normalized_i32_mul(
            *bin, source_values, normalized_i32_add_results);
        const bool sext_add = exact_downstream_i64_sext_add(
            *bin, source_values, scalar_sext_results);
        const Type result_type = fneg ? *selected_scalar_lhs_type
            : direct_scalar_lhs_fadd ? *selected_scalar_lhs_type
            : direct_scalar_lhs_fsub ? *selected_scalar_lhs_type
            : direct_scalar_lhs_fmul ? *selected_scalar_lhs_fmul_type
            : direct_scalar_rhs_fadd ? *selected_scalar_rhs_type
            : direct_scalar_rhs_fsub ? *selected_scalar_rhs_type
            : direct_scalar_rhs_fmul ? *selected_scalar_rhs_type
            : (fadd || fmul || fpext_fmul || sitofp_fmul || uitofp_fmul) ? Type{TypeKind::F64, 64, "double"}
            : float_fmul ? Type{TypeKind::F32, 32, "float"}
            : (sext_add || (ffs_add && bin->type_str.integer_bit_width() == 64)) ? Type{TypeKind::Integer, 64, "i64"}
                       : Type{TypeKind::Integer, 32, "i32"};
        if ((!fneg && !direct_scalar_lhs_fadd && !direct_scalar_lhs_fsub && !direct_scalar_lhs_fmul && !direct_scalar_rhs_fadd && !direct_scalar_rhs_fsub && !direct_scalar_rhs_fmul && !fadd && !fmul && !fpext_fmul && !sitofp_fmul && !uitofp_fmul && !fptosi_add && !fptoui_add && !wide_ffs_trunc_add && !float_fmul && !add && !ctz_direct_add && !ctz_trunc_add && !clz_direct_add && !clz_trunc_add && !ctpop_direct_add && !ctpop_trunc_add && !mul && !sext_add && !ffs_add) ||
            !source_values.emplace(bin->result.value_id()->value, result_type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "binary requires one exact admitted source-authorized operand shape");
        if (add) normalized_i32_add_results.insert(bin->result.value_id()->value);
        if (ctz_direct_add) ++builtin_ctz_direct_add_uses[add_lhs->value];
        if (ctz_trunc_add) ++builtin_ctz_trunc_add_uses[add_lhs->value];
        if (clz_direct_add) ++builtin_clz_direct_add_uses[add_lhs->value];
        if (clz_trunc_add) ++builtin_clz_trunc_add_uses[add_lhs->value];
        if (ctpop_direct_add) ++builtin_ctpop_direct_add_uses[add_lhs->value];
        if (ctpop_trunc_add) ++builtin_ctpop_trunc_add_uses[add_lhs->value];
        if (ffs_add) builtin_ffs_add_results.insert(bin->result.value_id()->value);
        if (fadd) downstream_double_fadd_results.insert(bin->result.value_id()->value);
        if (fmul) downstream_double_fmul_results.insert(bin->result.value_id()->value);
        if (float_fmul) {
          ++scalar_fptrunc_fmul_uses[bin->lhs.value_id()->value];
        }
        if (fpext_fmul) ++scalar_fpext_fmul_uses[bin->lhs.value_id()->value];
        if (sitofp_fmul) ++scalar_sitofp_fmul_uses[bin->lhs.value_id()->value];
        if (uitofp_fmul) ++scalar_uitofp_fmul_uses[bin->lhs.value_id()->value];
        if (fptosi_add) ++scalar_fptosi_add_uses[bin->lhs.value_id()->value];
        if (fptoui_add) ++scalar_fptoui_add_uses[bin->lhs.value_id()->value];
        if (wide_ffs_trunc_add) ++wide_ffs_trunc_add_uses[bin->lhs.value_id()->value];
        continue;
      }
      if (const auto* abs = std::get_if<LirAbsOp>(&instruction)) {
        if (!exact_selected_global_i32_abs(
                *abs, source_values, selected_global_i32_load_results) ||
            !source_values.emplace(abs->result.value_id()->value,
                                   Type{TypeKind::Integer, 32, "i32"}).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "abs requires one exact selected-global i32 Load source and a current-function i32 result");
        selected_global_i32_abs_results.insert(abs->result.value_id()->value);
        continue;
      }
      if (const auto* compare = std::get_if<LirCmpOp>(&instruction)) {
        const bool slt = exact_selected_global_i32_slt_compare(
            *compare, source_values, selected_global_i32_load_results);
        const bool olt = exact_downstream_double_olt_compare(
            *compare, source_values, downstream_double_fmul_results);
        const bool ffs_zero = exact_builtin_ffs_zero_compare(*compare);
        const bool truthiness_ne = compare->truthiness_lhs_parameter_authority &&
            !compare->is_float &&
            compare->predicate.typed() == std::optional{codegen::lir::LirCmpPredicate::Ne} &&
            compare->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
            compare->lhs.kind() == codegen::lir::LirOperandKind::SsaValue &&
            compare->lhs.value_id() && compare->rhs.integer_immediate() &&
            compare->rhs.integer_immediate()->value == 0 &&
            source_values.count(compare->lhs.value_id()->value) == 1;
        const bool pointer_truthiness_ne = compare->pointer_truthiness_parameter_authority &&
            !compare->is_float &&
            compare->predicate.typed() == std::optional{codegen::lir::LirCmpPredicate::Ne} &&
            compare->type_str == codegen::lir::LirTypeRef::integer(64) &&
            compare->lhs.kind() == codegen::lir::LirOperandKind::SsaValue &&
            compare->lhs.value_id() && compare->rhs.integer_immediate() &&
            compare->rhs.integer_immediate()->value == 0 &&
            source_values.count(compare->lhs.value_id()->value) == 1;
        if (pointer_truthiness_ne && ++selected_pointer_truthiness_parameter_count != 1)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "direct pointer truthiness parameter requires the one exact typed authority row");
        if ((!slt && !olt && !ffs_zero && !truthiness_ne && !pointer_truthiness_ne) ||
            !source_values.emplace(compare->result.value_id()->value,
                                   Type{TypeKind::I1, 1, "i1"}).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "compare requires one exact admitted source-authorized operand shape");
        if (olt) downstream_double_olt_compare_results.insert(compare->result.value_id()->value);
        if (ffs_zero) builtin_ffs_zero_compare_results.insert(compare->result.value_id()->value);
        continue;
      }
      if (const auto* select = std::get_if<LirSelectOp>(&instruction)) {
        const auto* condition_id = select->cond.value_id();
        const auto* false_id = select->false_val.value_id();
        const auto false_value = false_id ? source_values.find(false_id->value) : source_values.end();
        const bool ffs_false_arm = select->false_val.kind() == codegen::lir::LirOperandKind::SsaValue &&
            false_id && false_id->valid() && false_value != source_values.end() &&
            builtin_ffs_add_results.count(false_id->value) == 1 &&
            select->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
            select->type_str.integer_bit_width() == false_value->second.bit_width;
        const bool ffs_condition = select->cond.kind() == codegen::lir::LirOperandKind::SsaValue &&
            condition_id && condition_id->valid() &&
            builtin_ffs_zero_compare_results.count(condition_id->value) == 1;
        const auto select_type = ffs_false_arm ? false_value->second : Type{TypeKind::Integer, 64, "i64"};
        if ((ffs_false_arm && !ffs_condition) || (!ffs_false_arm && !exact_wide_ffs_select(*select)) ||
            !source_values.emplace(select->result.value_id()->value, select_type).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                            "select requires the one admitted typed wide ffs result receipt");
        wide_ffs_select_results.insert(select->result.value_id()->value);
        continue;
      }
      if (const auto* cast = std::get_if<LirCastOp>(&instruction)) {
        if (exact_double_olt_zext_use(*cast, downstream_double_olt_compare_results)) {
          ++downstream_double_olt_zext_uses[cast->operand.value_id()->value];
          continue;
        }
        const bool intrinsic_trunc = exact_i64_intrinsic_trunc_cast(
            module, *cast, source_values, intrinsic_results);
        const auto* trunc_operand = cast->operand.value_id();
        const bool ctz_trunc = intrinsic_trunc && trunc_operand &&
            builtin_ctz_results.count(trunc_operand->value) == 1;
        const bool clz_trunc = intrinsic_trunc && trunc_operand &&
            builtin_clz_results.count(trunc_operand->value) == 1;
        const bool ctpop_trunc = intrinsic_trunc && trunc_operand &&
            builtin_ctpop_results.count(trunc_operand->value) == 1;
        const bool scalar_sext = exact_scalar_i32_to_i64_sext(
            module, *cast, source_values);
        const bool scalar_fptrunc = exact_double_to_float_fptrunc(
            *cast, source_values, downstream_double_fmul_results);
        const bool scalar_fpext = exact_float_to_double_fpext(*cast, source_values);
        const bool scalar_sitofp = exact_signed_i32_to_double_sitofp(module, *cast, source_values);
        const bool scalar_uitofp = exact_unsigned_i32_to_double_uitofp(module, *cast, source_values);
        const bool scalar_fptosi = exact_double_to_signed_i32_fptosi(
            module, *cast, source_values, downstream_double_fadd_results);
        const bool scalar_fptoui = exact_double_to_unsigned_i32_fptoui(
            module, *cast, source_values, downstream_double_fadd_results);
        const bool wide_ffs_trunc = exact_wide_ffs_trunc(
            module, *cast, source_values, wide_ffs_select_results);
        const auto* cast_operand = cast->operand.value_id();
        const auto cast_operand_value = cast_operand
            ? source_values.find(cast_operand->value)
            : source_values.end();
        const bool pointer_truthiness_ptrtoint =
            cast->kind == codegen::lir::LirCastKind::PtrToInt &&
            cast->from_type.kind() == codegen::lir::LirTypeKind::Pointer &&
            cast->to_type == codegen::lir::LirTypeRef::integer(64) &&
            cast_operand && cast_operand_value != source_values.end() &&
            cast_operand_value->second == Type{TypeKind::Pointer};
        if ((!intrinsic_trunc && !scalar_sext && !scalar_fptrunc && !scalar_fpext && !scalar_sitofp && !scalar_uitofp && !scalar_fptosi && !scalar_fptoui && !wide_ffs_trunc && !pointer_truthiness_ptrtoint) ||
            !source_values.emplace(cast->result.value_id()->value,
                                   scalar_sext ? Type{TypeKind::Integer, 64, "i64"}
                                   : scalar_fptrunc ? Type{TypeKind::F32, 32, "float"}
                                   : scalar_fpext ? Type{TypeKind::F64, 64, "double"}
                                   : scalar_sitofp ? Type{TypeKind::F64, 64, "double"}
                                   : scalar_uitofp ? Type{TypeKind::F64, 64, "double"}
                                   : scalar_fptosi ? Type{TypeKind::Integer, 32, "i32"}
                                   : scalar_fptoui ? Type{TypeKind::Integer, 32, "i32"}
                                   : wide_ffs_trunc ? Type{TypeKind::Integer, 32, "i32"}
                                    : pointer_truthiness_ptrtoint ? Type{TypeKind::Integer, 64, "i64"}
                                                                 : Type{TypeKind::Integer, 32, "i32"}).second)
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "cast requires an exact admitted current-function typed receipt");
        if (scalar_sext) scalar_sext_results.insert(cast->result.value_id()->value);
        if (scalar_fptrunc) scalar_fptrunc_results.insert(cast->result.value_id()->value);
        if (scalar_fpext) scalar_fpext_results.insert(cast->result.value_id()->value);
        if (scalar_sitofp) scalar_sitofp_results.insert(cast->result.value_id()->value);
        if (scalar_uitofp) scalar_uitofp_results.insert(cast->result.value_id()->value);
        if (scalar_fptosi) scalar_fptosi_results.insert(cast->result.value_id()->value);
        if (scalar_fptoui) scalar_fptoui_results.insert(cast->result.value_id()->value);
        if (wide_ffs_trunc) wide_ffs_trunc_results.insert(cast->result.value_id()->value);
        if (ctz_trunc) {
          builtin_ctz_trunc_results.insert(cast->result.value_id()->value);
          ++builtin_ctz_trunc_uses[trunc_operand->value];
        }
        if (clz_trunc) {
          builtin_clz_trunc_results.insert(cast->result.value_id()->value);
          ++builtin_clz_trunc_uses[trunc_operand->value];
        }
        if (ctpop_trunc) {
          builtin_ctpop_trunc_results.insert(cast->result.value_id()->value);
          ++builtin_ctpop_trunc_uses[trunc_operand->value];
        }
        continue;
      }
      const auto* inline_asm = std::get_if<LirInlineAsmOp>(&instruction);
      if (!inline_asm)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "instruction family is outside the bounded constant/value slice");
      auto checked = validate_inline_asm_shape(
          module, *inline_asm, name, block.label, ordinary_values, source_values,
          inline_asm_results);
      if (!checked) return checked;
    }
  }

  for (const auto result : downstream_double_olt_compare_results) {
    if (downstream_double_olt_zext_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "double OLt result lacks its one exact compatibility ZExt use");
  }

  for (const auto result : builtin_ctz_results) {
    const auto type = source_values.find(result);
    if (type == source_values.end())
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin ctz result disappeared from the current-function registry");
    if (type->second == Type{TypeKind::Integer, 32, "i32"}) {
      if (builtin_ctz_direct_add_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin ctz i32 result lacks its one exact later i32 Add use");
    } else if (type->second == Type{TypeKind::Integer, 64, "i64"}) {
      if (builtin_ctz_trunc_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin ctz i64 result lacks its one exact i64-to-i32 Trunc use");
    }
  }
  for (const auto result : builtin_ctz_trunc_results) {
    if (builtin_ctz_trunc_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin ctz Trunc result lacks its one exact later i32 Add use");
  }
  for (const auto result : builtin_clz_results) {
    const auto type = source_values.find(result);
    if (type == source_values.end())
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin clz result disappeared from the current-function registry");
    if (type->second == Type{TypeKind::Integer, 32, "i32"}) {
      if (builtin_clz_direct_add_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin clz i32 result lacks its one exact later i32 Add use");
    } else if (type->second == Type{TypeKind::Integer, 64, "i64"}) {
      if (builtin_clz_trunc_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin clz i64 result lacks its one exact i64-to-i32 Trunc use");
    }
  }
  for (const auto result : builtin_clz_trunc_results) {
    if (builtin_clz_trunc_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin clz Trunc result lacks its one exact later i32 Add use");
  }
  for (const auto result : builtin_ctpop_results) {
    const auto type = source_values.find(result);
    if (type == source_values.end())
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin popcount result disappeared from the current-function registry");
    if (type->second == Type{TypeKind::Integer, 32, "i32"}) {
      if (builtin_ctpop_direct_add_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin popcount i32 result lacks its one exact later i32 Add use");
    } else if (type->second == Type{TypeKind::Integer, 64, "i64"}) {
      if (builtin_ctpop_trunc_uses[result] != 1)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                          "builtin popcount i64 result lacks its one exact i64-to-i32 Trunc use");
    }
  }
  for (const auto result : builtin_ctpop_trunc_results) {
    if (builtin_ctpop_trunc_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "builtin popcount Trunc result lacks its one exact later i32 Add use");
  }

  for (const auto result : scalar_fptrunc_results) {
    if (scalar_fptrunc_fmul_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar FPTrunc result lacks its one exact float FMul use");
  }
  for (const auto result : scalar_fpext_results) {
    if (scalar_fpext_fmul_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar FPExt result lacks its one exact double FMul use");
  }
  for (const auto result : scalar_sitofp_results) {
    if (scalar_sitofp_fmul_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar SIToFP result lacks its one exact double FMul use");
  }
  for (const auto result : scalar_uitofp_results) {
    if (scalar_uitofp_fmul_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar UIToFP result lacks its one exact double FMul use");
  }
  for (const auto result : scalar_fptosi_results) {
    if (scalar_fptosi_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar FPToSI result lacks its one exact i32 Add use");
  }
  for (const auto result : scalar_fptoui_results) {
    if (scalar_fptoui_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "scalar FPToUI result lacks its one exact i32 Add use");
  }
  for (const auto result : wide_ffs_trunc_results) {
    if (wide_ffs_trunc_add_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                        "wide ffs trunc result lacks its one exact i32 Add use");
  }
  for (const auto& [result, authority] :
       direct_floating_call_result_authorities) {
    (void)authority;
    if (direct_floating_call_result_authority_uses[result] != 1)
      return fail<void>(
          ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
          "direct native floating call-result authority must feed exactly one selected floating binary LHS");
  }

  for (const auto result : inline_asm_results) {
    if (inline_asm_store_uses[result] != 1)
      return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                        {}, "inline-asm result lacks its one exact Store use");
  }

  if (!function.entry.valid() ||
      block_ids.find(function.entry.value) == block_ids.end())
    return fail<void>(ImportErrorCode::InvalidEntryBlock, name, {},
                      "function.entry must resolve to an existing LIR block");
  if (function.entry.value != function.blocks.front().id.value)
    return fail<void>(
        ImportErrorCode::UnsupportedEntryBlock, name,
        function.blocks.front().label,
        "bootstrap BIR represents entry by first block order; LIR entry must "
        "already be first");

  for (const auto& block : function.blocks) {
    if (!block.insts.empty()) {
      if (const auto* indirect_br =
              std::get_if<LirIndirectBrOp>(&block.insts.back())) {
        std::unordered_set<std::uint32_t> targets;
        for (const auto successor : indirect_br->successors) {
          if (!successor.valid() ||
              block_labels_by_id.find(successor.value) ==
                  block_labels_by_id.end() ||
              !targets.insert(successor.value).second)
            return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                              block.label,
                              "LirIndirectBrOp.successors must resolve uniquely in current-function order");
        }
      }
    }
    auto checked = std::visit(
        [&](const auto& terminator) -> Result<void, ImportError> {
          using Term = std::decay_t<decltype(terminator)>;
          if constexpr (std::is_same_v<Term, LirRet>) {
            if (signature_return_type->kind == TypeKind::Void) {
              if (terminator.value_str ||
                  terminator.type_str.kind() !=
                      codegen::lir::LirTypeKind::Void)
                return fail<void>(ImportErrorCode::InvalidVoidReturn, name,
                                  block.label,
                                  "void LirRet must carry structured void type and no value");
            } else {
              const auto return_type =
                  lower_lir_type(module, terminator.type_str);
              if (!is_integer_type(*signature_return_type) ||
                  !return_type || !is_integer_type(*return_type) ||
                  terminator.type_str.kind() !=
                      codegen::lir::LirTypeKind::Integer ||
                  !terminator.type_str.integer_bit_width() ||
                  *return_type != *signature_return_type ||
                  !terminator.value_str)
                return fail<void>(
                    ImportErrorCode::UnsupportedTerminator, name,
                    block.label,
                    "scalar return requires one authoritative integer value with exact signature type");
              const auto& value = *terminator.value_str;
              if (value.kind() == codegen::lir::LirOperandKind::Immediate) {
                const auto* immediate = value.integer_immediate();
                if (!immediate || !integer_immediate_representable(
                                      immediate->value,
                                      *terminator.type_str.integer_bit_width()))
                  return fail<void>(
                      ImportErrorCode::UnsupportedTerminator, name,
                      block.label,
                      "scalar return immediate lacks exact in-range authority");
              } else if (value.kind() ==
                         codegen::lir::LirOperandKind::SsaValue) {
                const auto* value_id = value.value_id();
                const auto found = value_id
                                       ? source_values.find(value_id->value)
                                       : source_values.end();
                if (!value_id || !value_id->valid() ||
                    found == source_values.end() ||
                    found->second != *signature_return_type)
                  return fail<void>(
                      ImportErrorCode::UnsupportedTerminator, name,
                      block.label,
                      "scalar return SSA value must resolve in the current-function registry with exact signature type");
              } else {
                return fail<void>(
                    ImportErrorCode::UnsupportedTerminator, name,
                    block.label,
                    "scalar return value has an unsupported authority alternative");
              }
            }
          } else if constexpr (std::is_same_v<Term, LirBr>) {
            const auto target = terminator.successor.valid()
                                    ? block_labels_by_id.find(
                                          terminator.successor.value)
                                    : block_labels_by_id.end();
            if (target == block_labels_by_id.end())
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirBr.successor must resolve exactly once in its current function");
            if (terminator.target_label != target->second)
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirBr target_label must remain the selected successor's display shadow");
          } else if constexpr (std::is_same_v<Term, LirUnreachable>) {
            // Supported directly.
          } else if constexpr (std::is_same_v<Term, LirCondBr>) {
            const auto condition = terminator.condition.valid()
                                       ? source_values.find(
                                             terminator.condition.value)
                                       : source_values.end();
            if (condition == source_values.end() ||
                condition->second != Type{TypeKind::I1})
              return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                                block.label,
                                "LirCondBr.condition must resolve to a current-function boolean value");
            const auto true_target = terminator.true_successor.valid()
                                         ? block_labels_by_id.find(
                                               terminator.true_successor.value)
                                         : block_labels_by_id.end();
            const auto false_target = terminator.false_successor.valid()
                                          ? block_labels_by_id.find(
                                                terminator.false_successor.value)
                                          : block_labels_by_id.end();
            if (true_target == block_labels_by_id.end() ||
                false_target == block_labels_by_id.end())
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirCondBr successors must resolve exactly once in their current function");
          } else if constexpr (std::is_same_v<Term, LirSwitch>) {
            const auto selector = terminator.selector.valid()
                                      ? source_values.find(terminator.selector.value)
                                      : source_values.end();
            if (selector == source_values.end() || !is_integer_type(selector->second))
              return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                                block.label,
                                "LirSwitch.selector must resolve to a current-function integer value");
            if (terminator.case_successors.size() != terminator.cases.size())
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirSwitch ordered case successor authority is incoherent");
            const auto validate_target = [&](codegen::lir::LirBlockId target_id) {
              const auto target = target_id.valid()
                                      ? block_labels_by_id.find(target_id.value)
                                      : block_labels_by_id.end();
              return target != block_labels_by_id.end();
            };
            if (!validate_target(terminator.default_successor))
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirSwitch.default_successor must resolve in its current function");
            for (const auto target : terminator.case_successors)
              if (!validate_target(target))
                return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                  block.label,
                                  "LirSwitch.case_successors must resolve in current-function order");
          } else if constexpr (std::is_same_v<Term, LirIndirectBr>) {
            const auto address = terminator.addr.valid()
                                     ? source_values.find(terminator.addr.value)
                                     : source_values.end();
            if (address == source_values.end() ||
                address->second.kind != TypeKind::Pointer)
              return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                                block.label,
                                "LirIndirectBr.addr must resolve to a current-function pointer value");
            if (terminator.targets.empty())
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "LirIndirectBr.targets must not be empty");
            std::unordered_set<std::uint32_t> targets;
            for (const auto target_id : terminator.targets) {
              const auto target = target_id.valid()
                                      ? block_labels_by_id.find(target_id.value)
                                      : block_labels_by_id.end();
              if (target == block_labels_by_id.end() ||
                  !targets.insert(target_id.value).second)
                return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                  block.label,
                                  "LirIndirectBr.targets must resolve uniquely in its current function");
            }
          }
          return Result<void, ImportError>::success();
        },
        block.terminator);
    if (!checked) return checked;
  }
  if (direct_pointer_parameter_count != 0 &&
      selected_body_parameter_gep_count + selected_pointer_truthiness_parameter_count != 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "direct body-parameter authority requires exactly one selected pointer receipt");
  if (selected_body_parameter_gep_count + selected_scalar_body_parameter_lhs_count +
          selected_scalar_body_parameter_rhs_count +
          selected_pointer_truthiness_parameter_count > 1)
    return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
                      "only one selected body-parameter authority receipt is supported");
  return Result<void, ImportError>::success();
}

ImportError builder_failure(std::string function, std::string block,
                            std::string detail, BuildError error) {
  ImportError result{ImportErrorCode::BuilderFailure, std::move(function),
                     std::move(block), std::move(detail)};
  result.build_error = error;
  return result;
}

Result<Terminator, ImportError> lower_terminator(
    const LirModule& module, const Type& signature_return_type,
    const codegen::lir::LirTerminator& terminator,
    const std::unordered_map<std::uint32_t, ValueId>& source_values,
    FunctionBuilder& function_builder,
    const std::unordered_map<std::uint32_t, BlockId>& blocks,
    const std::string& function, const std::string& block) {
  return std::visit(
      [&](const auto& lir_terminator) -> Result<Terminator, ImportError> {
        using Term = std::decay_t<decltype(lir_terminator)>;
        if constexpr (std::is_same_v<Term, LirBr>) {
          const auto target = blocks.find(lir_terminator.successor.value);
          if (target == blocks.end())
            return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                    function, block, "LirBr.successor");
          return Result<Terminator, ImportError>::success(
              JumpTerm{target->second});
        } else if constexpr (std::is_same_v<Term, LirRet>) {
          if (signature_return_type.kind == TypeKind::Void)
            return Result<Terminator, ImportError>::success(ReturnTerm{});
          const auto return_type =
              lower_lir_type(module, lir_terminator.type_str);
          if (!return_type || *return_type != signature_return_type ||
              !lir_terminator.value_str)
            return fail<Terminator>(
                ImportErrorCode::UnsupportedTerminator, function, block,
                "validated scalar return type or value disappeared");
          const auto& value = *lir_terminator.value_str;
          if (const auto* immediate = value.integer_immediate()) {
            auto reserved = function_builder.reserve_value(*return_type);
            if (!reserved)
              return Result<Terminator, ImportError>::failure(builder_failure(
                  function, block, "reserve return immediate",
                  reserved.error()));
            auto defined = function_builder.define_int_constant(
                reserved.value(), static_cast<std::int64_t>(immediate->value));
            if (!defined)
              return Result<Terminator, ImportError>::failure(builder_failure(
                  function, block, "define return immediate",
                  defined.error()));
            return Result<Terminator, ImportError>::success(
                ReturnTerm{reserved.value()});
          }
          const auto* value_id = value.value_id();
          const auto found =
              value_id ? source_values.find(value_id->value)
                       : source_values.end();
          if (found == source_values.end())
            return fail<Terminator>(
                ImportErrorCode::UnsupportedTerminator, function, block,
                "validated scalar return SSA value disappeared from the current-function registry");
          return Result<Terminator, ImportError>::success(
              ReturnTerm{found->second});
        } else if constexpr (std::is_same_v<Term, LirUnreachable>) {
          return Result<Terminator, ImportError>::success(UnreachableTerm{});
        } else if constexpr (std::is_same_v<Term, LirCondBr>) {
          const auto condition = lir_terminator.condition.valid()
                                     ? source_values.find(
                                           lir_terminator.condition.value)
                                     : source_values.end();
          if (condition == source_values.end())
            return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                    function, block,
                                    "validated conditional branch condition disappeared from the current-function boolean registry");
          const auto true_target = lir_terminator.true_successor.valid()
                                       ? blocks.find(
                                             lir_terminator.true_successor.value)
                                       : blocks.end();
          const auto false_target = lir_terminator.false_successor.valid()
                                        ? blocks.find(
                                              lir_terminator.false_successor.value)
                                        : blocks.end();
          if (true_target == blocks.end() || false_target == blocks.end())
            return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                    function, block,
                                    "validated conditional branch successor disappeared from the current-function registry");
          return Result<Terminator, ImportError>::success(CondJumpTerm{
              condition->second, true_target->second, false_target->second});
        } else if constexpr (std::is_same_v<Term, LirSwitch>) {
          const auto selector = lir_terminator.selector.valid()
                                    ? source_values.find(lir_terminator.selector.value)
                                    : source_values.end();
          if (selector == source_values.end())
            return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                    function, block,
                                    "validated switch selector disappeared from the current-function registry");
          const auto default_target = lir_terminator.default_successor.valid()
                                          ? blocks.find(lir_terminator.default_successor.value)
                                          : blocks.end();
          if (default_target == blocks.end())
            return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                    function, block,
                                    "validated switch default successor disappeared from the current-function registry");
          std::vector<BlockId> case_targets;
          case_targets.reserve(lir_terminator.case_successors.size());
          for (const auto source_target : lir_terminator.case_successors) {
            const auto target = source_target.valid()
                                    ? blocks.find(source_target.value)
                                    : blocks.end();
            if (target == blocks.end())
              return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                      function, block,
                                      "validated switch case successor disappeared from the current-function registry");
            case_targets.push_back(target->second);
          }
          return Result<Terminator, ImportError>::success(SwitchTerm{
              selector->second, default_target->second, std::move(case_targets)});
        } else {
          static_assert(std::is_same_v<Term, LirIndirectBr>);
          const auto address = lir_terminator.addr.valid()
                                   ? source_values.find(lir_terminator.addr.value)
                                   : source_values.end();
          if (address == source_values.end())
            return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                    function, block,
                                    "validated indirect branch address disappeared from the current-function registry");
          std::vector<BlockId> targets;
          targets.reserve(lir_terminator.targets.size());
          for (const auto source_target : lir_terminator.targets) {
            const auto target = source_target.valid()
                                    ? blocks.find(source_target.value)
                                    : blocks.end();
            if (target == blocks.end())
              return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                      function, block,
                                      "validated indirect branch target disappeared from the current-function registry");
            targets.push_back(target->second);
          }
          return Result<Terminator, ImportError>::success(
              IndirectJumpTerm{address->second, std::move(targets)});
        }
      },
      terminator);
}

}  // namespace

Result<RawBir, ImportError> lower_lir_to_raw_bir(const LirModule& module,
                                                 ImportOptions) {
  auto module_check = validate_module_surface(module);
  if (!module_check)
    return Result<RawBir, ImportError>::failure(std::move(module_check.error()));
  for (const auto& function : module.functions) {
    auto function_check = validate_function(module, function);
    if (!function_check)
      return Result<RawBir, ImportError>::failure(
          std::move(function_check.error()));
  }

  ModuleBuilder builder;
  std::unordered_map<c4c::LinkNameId, GlobalObjectId> global_objects;
  std::unordered_map<c4c::LinkNameId, LinkNameId> imported_link_names;
  auto requirements = builder.set_intrinsic_requirements(
      IntrinsicRequirements{module.need_va_start, module.need_va_end,
                            module.need_va_copy, module.need_memcpy,
                            module.need_memset, module.need_stacksave,
                            module.need_stackrestore, module.need_abs,
                            module.need_ptrmask,
                            module.prefer_semantic_va_ops});
  if (!requirements)
    return Result<RawBir, ImportError>::failure(builder_failure(
        {}, {}, "import intrinsic requirements", requirements.error()));
  for (std::size_t index = 0; index < module.link_names.size(); ++index) {
    const auto source_id = static_cast<c4c::LinkNameId>(index + 1);
    auto added = builder.add_link_name(
        source_id, std::string(module.link_names.spelling(source_id)));
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import link-name table", added.error()));
    imported_link_names.emplace(source_id, added.value());
  }
  for (std::size_t index = 0; index < module.struct_names.size(); ++index) {
    const auto source_id = static_cast<c4c::StructNameId>(index + 1);
    auto added = builder.add_struct_name(
        source_id, std::string(module.struct_names.spelling(source_id)));
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import struct-name table", added.error()));
  }
  const auto struct_declarations = authoritative_struct_decls(module);
  if (!struct_declarations.has_value()) {
    return fail<RawBir>(
        ImportErrorCode::UnsupportedGlobals, {}, {},
        "canonical aggregate store facts must match structured declaration facts");
  }
  for (const auto* declaration : *struct_declarations) {
    std::vector<StructField> fields;
    fields.reserve(declaration->fields.size());
    for (const auto& field : declaration->fields)
      fields.push_back(StructField{*lower_lir_type(module, field.type)});
    auto added = builder.add_struct_declaration(
        declaration->name_id, std::move(fields), declaration->is_packed,
        declaration->is_opaque);
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import struct declaration", added.error()));
  }
  for (const auto& string_data : module.string_pool) {
    auto added = builder.add_string_data(
        string_data.pool_name, string_data.raw_bytes,
        static_cast<std::int64_t>(string_data.byte_length));
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import string-pool row", added.error()));
  }
  for (const auto& declaration : module.extern_decls) {
    auto added = builder.add_external_declaration(
        declaration.name, *lower_lir_type(module, declaration.return_type),
        *lower_return_extension(declaration.return_ext_attr),
        declaration.link_name_id == c4c::kInvalidLinkName
            ? std::nullopt
            : std::optional<c4c::LinkNameId>{declaration.link_name_id});
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import external declaration", added.error()));
  }
  for (const auto& global : module.globals) {
    const auto linkage = decode_global_linkage(global);
    const auto type = lower_global_type(module, global);
    auto added = builder.add_global_object(
        global.name, *type,
        global.align_bytes, global.is_internal,
        linkage->is_weak,
        global.is_const, global.is_extern_decl,
        global.link_name_id == c4c::kInvalidLinkName
            ? std::nullopt
            : std::optional<c4c::LinkNameId>{global.link_name_id},
        global.is_extern_decl
            ? std::nullopt
            : std::optional<std::string>{global.init_text},
        global.initializer_function_link_name_ids,
        linkage->visibility);
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import global object", added.error()));
    if (global.link_name_id != c4c::kInvalidLinkName)
      global_objects.emplace(global.link_name_id, added.value());
  }
  for (const auto& specialization : module.spec_entries) {
    auto added = builder.add_specialization(
        specialization.spec_key, specialization.template_origin,
        specialization.mangled_name,
        specialization.mangled_link_name_id);
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import specialization metadata", added.error()));
  }
  std::vector<FunctionId> function_ids;
  function_ids.reserve(module.functions.size());
  std::unordered_map<c4c::LinkNameId, FunctionId> functions_by_link_name_id;
  for (const auto& function : module.functions) {
    const std::string name = function_link_name(module, function);
    const Type imported_return_type = function.signature_return_type_ref
        ? native_floating_call_type(module, *function.signature_return_type_ref)
              .value_or(*lower_signature_type(module, function.return_type,
                                               function.signature_return_type_ref))
        : *lower_signature_type(module, function.return_type,
                                function.signature_return_type_ref);
    FunctionSignature signature;
    signature.return_type = imported_return_type;
    const auto* stored_function_signature =
        module.find_function_signature(function.function_signature_ref);
    auto parameter_types = stored_function_signature != nullptr
        ? lower_store_backed_function_parameter_types(module,
                                                      *stored_function_signature)
        : std::optional<std::vector<Type>>{};
    if (!parameter_types) {
      parameter_types = lower_function_parameter_types(module, function);
    }
    if (!parameter_types) {
      return fail<RawBir>(ImportErrorCode::UnsupportedFunctionParameters,
                          name, {},
                          "function parameters are outside store-backed byval or scalar receipt");
    }
    signature.parameter_types = std::move(*parameter_types);
    auto created = builder.create_function(
        std::move(signature), name, function.is_declaration,
        FunctionMetadata{function.is_internal,
                         function.can_elide_if_unreferenced});
    if (!created)
      return Result<RawBir, ImportError>::failure(builder_failure(
          name, {}, "create function", created.error()));
    function_ids.push_back(created.value());
    if (function.link_name_id != c4c::kInvalidLinkName) {
      const auto registered = functions_by_link_name_id.emplace(
          function.link_name_id, created.value());
      if (!registered.second && registered.first->second != created.value())
        return fail<RawBir>(
            ImportErrorCode::UnsupportedOrdinaryInstruction, name, {},
            "function LinkNameId resolves to conflicting BIR identities");
    }
  }
  for (const auto& declaration : module.extern_decls) {
    if (declaration.link_name_id == c4c::kInvalidLinkName ||
        functions_by_link_name_id.find(declaration.link_name_id) !=
            functions_by_link_name_id.end() ||
        !declaration.function_signature_ref.valid()) {
      continue;
    }
    const auto* signature_entry =
        module.find_function_signature(declaration.function_signature_ref);
    if (!signature_entry || !signature_entry->return_type_ref.has_value()) {
      continue;
    }
    FunctionSignature signature;
    const auto return_type =
        lower_lir_type(module, *signature_entry->return_type_ref);
    if (!return_type) {
      return fail<RawBir>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                          declaration.name, {},
                          "extern declaration signature adapter return type is unsupported");
    }
    signature.return_type = *return_type;
    signature.is_variadic = signature_entry->is_variadic;
    signature.parameter_types.reserve(
        signature_entry->fixed_param_type_refs.size());
    const auto extern_params =
        lower_store_backed_function_parameter_types(module, *signature_entry);
    if (extern_params) {
      signature.parameter_types = *extern_params;
    } else {
      for (const auto& param_ref : signature_entry->fixed_param_type_refs) {
        const auto param_type = lower_lir_type(module, param_ref);
        if (!param_type) {
          return fail<RawBir>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                              declaration.name, {},
                              "extern declaration signature adapter parameter type is unsupported");
        }
        signature.parameter_types.push_back(*param_type);
      }
    }
    auto created = builder.create_function(std::move(signature),
                                           declaration.name, true,
                                           FunctionMetadata{});
    if (!created) {
      return Result<RawBir, ImportError>::failure(builder_failure(
          declaration.name, {}, "create extern declaration function",
          created.error()));
    }
    functions_by_link_name_id.emplace(declaration.link_name_id,
                                      created.value());
  }

  for (std::size_t function_index = 0;
       function_index < module.functions.size(); ++function_index) {
    const auto& function = module.functions[function_index];
    const std::string name = function_link_name(module, function);
    const Type imported_return_type = function.signature_return_type_ref
        ? native_floating_call_type(module, *function.signature_return_type_ref)
              .value_or(*lower_signature_type(module, function.return_type,
                                               function.signature_return_type_ref))
        : *lower_signature_type(module, function.return_type,
                                function.signature_return_type_ref);
    if (function.is_declaration) continue;

    const codegen::lir::LirAmd64SysVOverflowAggregateCarrier* overflow_carrier = nullptr;
    for (const auto& block : function.blocks) for (const auto& instruction : block.insts) {
      const auto* memcpy = std::get_if<LirMemcpyOp>(&instruction);
      if (memcpy && memcpy->amd64_sysv_overflow_aggregate_carrier)
        overflow_carrier = &*memcpy->amd64_sysv_overflow_aggregate_carrier;
    }
    const bool has_overflow_carrier = overflow_carrier != nullptr;

    std::optional<ImportError> edit_error;
    auto edited = builder.with_function(
        function_ids[function_index], [&](FunctionBuilder& function_builder) {
          std::unordered_map<std::uint32_t, BlockId> blocks;
          std::unordered_map<std::string, std::pair<ValueId, Type>> ordinary_values;
          std::unordered_map<std::uint32_t, ValueId> source_values;
          std::unordered_set<std::uint32_t> native_floating_call_results;
          std::unordered_set<std::uint32_t> downstream_double_fadd_results;
          std::unordered_set<std::uint32_t> downstream_double_fmul_results;
          std::unordered_set<std::uint32_t> scalar_fptrunc_results;
          std::unordered_set<std::uint32_t> scalar_fpext_results;
          std::unordered_set<std::uint32_t> scalar_sitofp_results;
          std::unordered_set<std::uint32_t> scalar_uitofp_results;
          std::unordered_set<std::uint32_t> scalar_fptosi_results;
          std::unordered_set<std::uint32_t> scalar_fptoui_results;
          std::unordered_set<std::uint32_t> wide_ffs_select_results;
          std::unordered_set<std::uint32_t> wide_ffs_trunc_results;
          std::unordered_set<std::uint32_t> downstream_double_olt_compare_results;
          std::unordered_set<std::uint32_t> native_i32_cttz_results;
          std::unordered_set<std::uint32_t> builtin_ctz_results;
          std::unordered_set<std::uint32_t> builtin_ctz_trunc_results;
          std::unordered_set<std::uint32_t> builtin_clz_results;
          std::unordered_set<std::uint32_t> builtin_clz_trunc_results;
          std::unordered_set<std::uint32_t> builtin_ctpop_results;
          std::unordered_set<std::uint32_t> builtin_ctpop_trunc_results;
          std::unordered_set<std::uint32_t> builtin_ffs_zero_compare_results;
          std::unordered_set<std::uint32_t> selected_global_i32_abs_results;
          std::unordered_set<std::uint32_t> normalized_i32_add_results;
          std::unordered_set<std::uint32_t> scalar_sext_results;
          blocks.reserve(function.blocks.size());
          for (const LirBlock& block : function.blocks) {
            auto created_block = function_builder.create_block(block.label);
            if (!created_block) {
              edit_error = builder_failure(name, block.label, "create block",
                                           created_block.error());
              return Result<void, BuildError>::failure(created_block.error());
            }
            blocks.emplace(block.id.value, created_block.value());
          }

          for (const auto& constant : function.direct_label_address_constants) {
            auto reserved = function_builder.reserve_source_value(
                constant.value.value, *lower_lir_type(module, constant.type));
            if (!reserved) {
              edit_error = builder_failure(name, {}, "reserve direct label-address value",
                                           reserved.error());
              return Result<void, BuildError>::failure(reserved.error());
            }
            source_values.emplace(constant.value.value, reserved.value());
          }

          const auto selected_body_parameter = std::find_if(
              function.native_body_parameter_definitions.begin(),
              function.native_body_parameter_definitions.end(), [](const auto& parameter) {
                return parameter.abi ==
                    codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
              });
          const auto selected_scalar_authority = [&]() {
            for (const auto& block : function.blocks)
              for (const auto& instruction : block.insts)
                if (const auto* bin = std::get_if<LirBinOp>(&instruction);
                    bin && bin->scalar_lhs_parameter_authority)
                  return &*bin->scalar_lhs_parameter_authority;
            return static_cast<const codegen::lir::LirScalarBinaryLhsParameterAuthority*>(nullptr);
          }();
          const auto selected_scalar_rhs_authority = [&]() {
            for (const auto& block : function.blocks)
              for (const auto& instruction : block.insts)
                if (const auto* bin = std::get_if<LirBinOp>(&instruction);
                    bin && bin->scalar_rhs_parameter_authority)
                  return &*bin->scalar_rhs_parameter_authority;
            return static_cast<const codegen::lir::LirScalarBinaryRhsParameterAuthority*>(nullptr);
          }();
          const auto selected_return_value_authority = [&]() {
            for (const auto& block : function.blocks)
              if (const auto* ret = std::get_if<LirRet>(&block.terminator);
                  ret && ret->return_value_parameter_authority)
                return &*ret->return_value_parameter_authority;
            return static_cast<const codegen::lir::LirReturnValueParameterAuthority*>(nullptr);
          }();
          const auto selected_switch_selector_authority = [&]() {
            for (const auto& block : function.blocks)
              if (const auto* sw = std::get_if<LirSwitch>(&block.terminator);
                  sw && sw->selector_parameter_authority)
                return &*sw->selector_parameter_authority;
            return static_cast<const codegen::lir::LirSwitchSelectorParameterAuthority*>(nullptr);
          }();
          const auto selected_truthiness_lhs_authority = [&]() {
            for (const auto& block : function.blocks)
              for (const auto& instruction : block.insts)
                if (const auto* compare = std::get_if<LirCmpOp>(&instruction);
                    compare && compare->truthiness_lhs_parameter_authority)
                  return &*compare->truthiness_lhs_parameter_authority;
            return static_cast<const codegen::lir::LirTruthinessComparisonLhsParameterAuthority*>(nullptr);
          }();
          const auto selected_pointer_truthiness_authority = [&]() {
            for (const auto& block : function.blocks)
              for (const auto& instruction : block.insts)
                if (const auto* compare = std::get_if<LirCmpOp>(&instruction);
                    compare && compare->pointer_truthiness_parameter_authority)
                  return &*compare->pointer_truthiness_parameter_authority;
            return static_cast<const codegen::lir::LirPointerTruthinessParameterAuthority*>(nullptr);
          }();
          const auto selected_fixed_direct_call_argument_authority = [&]()
              -> std::pair<
                  const codegen::lir::LirFixedDirectCallArgumentParameterAuthority*,
                  std::size_t> {
            for (const auto& block : function.blocks)
              for (const auto& instruction : block.insts)
                if (const auto* call = std::get_if<LirCallOp>(&instruction);
                    call)
                  for (std::size_t index = 0; index < call->structured_args.size(); ++index)
                    if (call->structured_args[index]
                            .fixed_direct_call_argument_parameter_authority)
                      return {&*call->structured_args[index]
                                    .fixed_direct_call_argument_parameter_authority,
                              index};
            return {nullptr, 0};
          }();
          const auto selected_scalar_body_parameter = selected_scalar_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_scalar_authority->value &&
                                   parameter.parameter_index == selected_scalar_authority->parameter_index &&
                                   parameter.type == selected_scalar_authority->type &&
                                   parameter.owner == selected_scalar_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_scalar_rhs_body_parameter = selected_scalar_rhs_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_scalar_rhs_authority->value &&
                                   parameter.parameter_index == selected_scalar_rhs_authority->parameter_index &&
                                   parameter.type == selected_scalar_rhs_authority->type &&
                                   parameter.owner == selected_scalar_rhs_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_return_value_body_parameter = selected_return_value_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_return_value_authority->value &&
                                   parameter.parameter_index == selected_return_value_authority->parameter_index &&
                                   parameter.type == selected_return_value_authority->type &&
                                   parameter.owner == selected_return_value_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_switch_selector_body_parameter = selected_switch_selector_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_switch_selector_authority->value &&
                                   parameter.parameter_index == selected_switch_selector_authority->parameter_index &&
                                   parameter.type == selected_switch_selector_authority->type &&
                                   parameter.owner == selected_switch_selector_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_truthiness_lhs_body_parameter = selected_truthiness_lhs_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_truthiness_lhs_authority->value &&
                                   parameter.parameter_index == selected_truthiness_lhs_authority->parameter_index &&
                                   parameter.type == selected_truthiness_lhs_authority->type &&
                                   parameter.owner == selected_truthiness_lhs_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_pointer_truthiness_body_parameter =
              selected_pointer_truthiness_authority
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               return parameter.value == selected_pointer_truthiness_authority->value &&
                                   parameter.parameter_index == selected_pointer_truthiness_authority->parameter_index &&
                                   parameter.type == selected_pointer_truthiness_authority->type &&
                                   parameter.owner == selected_pointer_truthiness_authority->owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectPointer;
                             })
              : function.native_body_parameter_definitions.end();
          const auto selected_fixed_direct_call_argument_body_parameter =
              selected_fixed_direct_call_argument_authority.first
              ? std::find_if(function.native_body_parameter_definitions.begin(),
                             function.native_body_parameter_definitions.end(),
                             [&](const auto& parameter) {
                               const auto& authority =
                                   *selected_fixed_direct_call_argument_authority.first;
                               return parameter.value == authority.value &&
                                   parameter.parameter_index == authority.parameter_index &&
                                   parameter.type == authority.type &&
                                   parameter.owner == authority.owner &&
                                   parameter.abi == codegen::lir::LirNativeBodyParameterAbi::DirectScalar;
                             })
              : function.native_body_parameter_definitions.end();
          if (selected_scalar_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(selected_scalar_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(selected_scalar_body_parameter->value.value,
                                                     parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar body parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_scalar_rhs_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(selected_scalar_rhs_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(selected_scalar_rhs_body_parameter->value.value,
                                                     parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar RHS body parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_return_value_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(
                selected_return_value_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(
                                  selected_return_value_body_parameter->value.value,
                                  parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar return parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_switch_selector_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(
                selected_switch_selector_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(
                                  selected_switch_selector_body_parameter->value.value,
                                  parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar switch selector parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_truthiness_lhs_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(
                selected_truthiness_lhs_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(
                                  selected_truthiness_lhs_body_parameter->value.value,
                                  parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar truthiness parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_pointer_truthiness_body_parameter != function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(
                selected_pointer_truthiness_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(
                                  selected_pointer_truthiness_body_parameter->value.value,
                                  parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct pointer truthiness parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }
          if (selected_fixed_direct_call_argument_body_parameter !=
              function.native_body_parameter_definitions.end()) {
            auto parameter = function_builder.parameter(
                selected_fixed_direct_call_argument_body_parameter->parameter_index);
            if (!parameter || !source_values.emplace(
                                  selected_fixed_direct_call_argument_body_parameter->value.value,
                                  parameter.value()).second) {
              edit_error = ImportError{ImportErrorCode::UnsupportedFunctionParameters, name, {},
                                       "direct scalar fixed direct-call parameter failed authoritative Raw-BIR receipt"};
              return Result<void, BuildError>::failure(BuildError::InvalidParameter);
            }
          }

          for (const LirBlock& block : function.blocks) {
            (void)block;
            for (const auto& constant : function.direct_label_address_constants) {
              auto defined = function_builder.define_label_address_constant(
                  source_values.at(constant.value.value),
                  blocks.at(constant.target.value));
              if (!defined) {
                edit_error = builder_failure(name, {}, "define direct label-address constant",
                                             defined.error());
                return defined;
              }
            }
            break;
          }

          for (const LirBlock& block : function.blocks) {
            for (const auto& instruction : block.insts) {
              std::optional<std::pair<std::uint32_t, Type>> source;
              if (const auto* constant = std::get_if<LirConstInt>(&instruction))
                source = std::pair{constant->result.value,
                                   *lower_constant_type(module, constant->type)};
              else if (const auto* constant =
                           std::get_if<LirConstFloat>(&instruction))
                source = std::pair{constant->result.value,
                                   *lower_constant_type(module, constant->type)};
              if (!source) continue;
              auto reserved = function_builder.reserve_source_value(
                  source->first, std::move(source->second));
              if (!reserved) {
                edit_error = builder_failure(name, block.label,
                                             "reserve source value",
                                             reserved.error());
                return Result<void, BuildError>::failure(reserved.error());
              }
              source_values.emplace(source->first, reserved.value());
            }
          }

          for (const auto& instruction : function.alloca_insts) {
            const auto& alloca = std::get<LirAllocaOp>(instruction);
            const auto& authority = *alloca.local_object_authority;
            const auto result_id = alloca.result.value_id()->value;
            auto reserved = function_builder.reserve_source_value(
                result_id, Type{TypeKind::Pointer});
            if (!reserved || !source_values.emplace(result_id, reserved.value()).second) {
              edit_error = reserved
                  ? ImportError{ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                                "alloca result collided in the current-function source registry"}
                  : builder_failure(name, {}, "reserve alloca result", reserved.error());
              return Result<void, BuildError>::failure(
                  reserved ? BuildError::DuplicateSourceValue : reserved.error());
            }
            const auto owner = imported_link_names.find(authority.owner);
            if (owner == imported_link_names.end()) {
              edit_error = ImportError{ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                                       "alloca authority owner disappeared from imported name identities"};
              return Result<void, BuildError>::failure(BuildError::InvalidNameId);
            }
            auto appended = function_builder.append(
                blocks.at(function.entry.value), AllocaAuthoritySpec{
                    SourceValueId{function_ids[function_index], result_id},
                    SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                    SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                    Type{TypeKind::Pointer}, *lower_lir_type(module, authority.pointee_type), authority.live});
            if (!appended) {
              edit_error = builder_failure(name, {}, "append alloca authority", appended.error());
              return Result<void, BuildError>::failure(appended.error());
            }
          }

          // Reserve PHI definitions before lowering bodies so a loop backedge
          // can refer to its join result without any name-based recovery.
          for (const LirBlock& block : function.blocks) {
            for (const auto& instruction : block.insts) {
              const auto* phi = std::get_if<LirPhiOp>(&instruction);
              if (!phi) continue;
              const auto* result = phi->result.value_id();
              auto reserved = function_builder.reserve_source_value(
                  result->value, *lower_lir_type(module, phi->type_str));
              if (!reserved || !source_values.emplace(result->value, reserved.value()).second) {
                edit_error = reserved
                    ? ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                  block.label, "phi result registration collided"}
                    : builder_failure(name, block.label, "reserve phi result", reserved.error());
                return Result<void, BuildError>::failure(
                    reserved ? BuildError::DuplicateSourceValue : reserved.error());
              }
            }
          }

          for (const LirBlock& block : function.blocks) {
            for (const auto& instruction : block.insts) {
              if (const auto* constant = std::get_if<LirConstInt>(&instruction)) {
                auto defined = function_builder.define_int_constant(
                    source_values.at(constant->result.value),
                    static_cast<std::int64_t>(constant->value));
                if (!defined) {
                  edit_error = builder_failure(name, block.label,
                                               "define integer constant",
                                               defined.error());
                  return defined;
                }
                continue;
              }
              if (const auto* constant = std::get_if<LirConstFloat>(&instruction)) {
                static_assert(sizeof(constant->value) == sizeof(std::uint64_t));
                std::uint64_t bits = 0;
                std::memcpy(&bits, &constant->value, sizeof(bits));
                auto defined = function_builder.define_float_constant_bits(
                    source_values.at(constant->result.value), bits);
                if (!defined) {
                  edit_error = builder_failure(name, block.label,
                                               "define floating constant",
                                               defined.error());
                  return defined;
                }
                continue;
              }
              if (const auto* phi = std::get_if<LirPhiOp>(&instruction)) {
                PhiSpec spec;
                spec.type = *lower_lir_type(module, phi->type_str);
                spec.source_result_id = phi->result.value_id()->value;
                spec.incoming.reserve(phi->incoming.size());
                for (const auto& incoming : phi->incoming) {
                  const auto predecessor = blocks.find(incoming.predecessor.value);
                  const auto predecessor_source = std::find_if(
                      function.blocks.begin(), function.blocks.end(),
                      [&](const LirBlock& candidate) {
                        return candidate.id == incoming.predecessor;
                      });
                  const auto occurrence =
                      predecessor_source == function.blocks.end() ||
                              !incoming.successor_occurrence
                          ? std::optional<std::uint32_t>{}
                          : phi_edge_occurrence(predecessor_source->terminator,
                                                block.id,
                                                *incoming.successor_occurrence);
                  if (predecessor == blocks.end() || !occurrence) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "phi incoming must select one exact current-function CFG edge occurrence"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  ValueId incoming_value;
                  if (incoming.value.kind() == codegen::lir::LirOperandKind::SpecialToken) {
                    const auto* token = incoming.value.special_token();
                    if (!token || static_cast<unsigned>(*token) >
                                      static_cast<unsigned>(codegen::lir::LirSpecialToken::False)) {
                      edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                                               "phi special token must retain a closed native semantic carrier"};
                      return Result<void, BuildError>::failure(BuildError::InvalidValue);
                    }
                    auto reserved = function_builder.reserve_value(spec.type);
                    auto defined = reserved ? function_builder.define_special_constant(
                        reserved.value(), static_cast<SpecialConstantKind>(*token))
                        : Result<void, BuildError>::failure(reserved.error());
                    if (!reserved || !defined) {
                      edit_error = builder_failure(name, block.label, "materialize phi special token",
                          reserved ? defined.error() : reserved.error());
                      return Result<void, BuildError>::failure(reserved ? defined.error() : reserved.error());
                    }
                    incoming_value = reserved.value();
                  } else {
                    const auto value = source_values.find(incoming.value.value_id()->value);
                    if (value == source_values.end()) {
                      edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                                               "phi SSA incoming authority did not resolve in the current function"};
                      return Result<void, BuildError>::failure(BuildError::InvalidValue);
                    }
                    incoming_value = value->second;
                  }
                  spec.incoming.push_back(PhiIncomingSpec{
                      incoming_value, predecessor->second, blocks.at(block.id.value),
                      *occurrence});
                }
                auto appended = function_builder.append(blocks.at(block.id.value), std::move(spec));
                if (!appended) {
                  edit_error = builder_failure(name, block.label, "append phi", appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (std::get_if<LirIndirectBrOp>(&instruction)) continue;
              if (const auto* memcpy = std::get_if<LirMemcpyOp>(&instruction)) {
                if (memcpy->amd64_sysv_overflow_aggregate_carrier) {
                  const auto& carrier = *memcpy->amd64_sysv_overflow_aggregate_carrier;
                  const auto owner = imported_link_names.find(carrier.va_list_object.owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "AMD64 SysV overflow aggregate owner disappeared from imported name identities"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  auto appended = function_builder.append(
                      blocks.at(block.id.value), Amd64SysVOverflowAggregateMemcpySpec{
                          SourceValueId{function_ids[function_index], carrier.va_list_object.pointer_definition.value},
                          SourceObjectId{function_ids[function_index], carrier.va_list_object.object.value},
                          owner->second,
                          SourceValueId{function_ids[function_index], carrier.overflow_field_address.value},
                          SourceValueId{function_ids[function_index], carrier.overflow_pointer_load.value},
                          SourceValueId{function_ids[function_index], carrier.destination.pointer_definition.value},
                          SourceObjectId{function_ids[function_index], carrier.destination.object.value},
                          SourceValueId{function_ids[function_index], carrier.final_load.value},
                          *lower_lir_type(module, carrier.payload_type),
                          static_cast<std::int64_t>(carrier.payload_size.value),
                          carrier.va_list_object.live, carrier.destination.live});
                  if (!appended) {
                    edit_error = builder_failure(name, block.label,
                                                 "append AMD64 SysV overflow aggregate memcpy authority",
                                                 appended.error());
                    return Result<void, BuildError>::failure(appended.error());
                  }
                  continue;
                }
                const auto& authority = *memcpy->selected_authority;
                const auto destination_owner = imported_link_names.find(
                    authority.destination_object_owner);
                const auto source_owner = imported_link_names.find(
                    authority.source_object_owner);
                if (destination_owner == imported_link_names.end() ||
                    source_owner == imported_link_names.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "selected memcpy owner disappeared from imported name identities"};
                  return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value), SelectedMemcpySpec{
                        SourceValueId{function_ids[function_index], authority.destination.value},
                        SourceValueId{function_ids[function_index], authority.source.value},
                        SourceObjectId{function_ids[function_index], authority.destination_object.value},
                        SourceObjectId{function_ids[function_index], authority.source_object.value},
                        destination_owner->second, source_owner->second,
                        Type{TypeKind::Pointer},
                        static_cast<std::int64_t>(authority.size.value),
                        authority.destination_live_at_site,
                        authority.source_live_at_site});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append selected memcpy authority",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (const auto* store = std::get_if<LirStoreOp>(&instruction)) {
                const Type type = *lower_lir_type(module, store->type_str);
                if (store->requires_native_store_authority) {
                  const auto& authority = *store->local_object_authority;
                  const auto owner = imported_link_names.find(authority.owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                        "validated local store authority owner disappeared from imported name identities"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  auto appended = function_builder.append(blocks.at(block.id.value), LocalStoreAuthoritySpec{
                      SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                      SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                      Type{TypeKind::Pointer}, type, store->val.integer_immediate()->value, authority.live});
                  if (!appended) { edit_error = builder_failure(name, block.label, "append local store authority", appended.error());
                    return Result<void, BuildError>::failure(appended.error()); }
                  continue;
                }
                ValueId stored_value{};
                if (const auto* immediate = store->val.integer_immediate()) {
                  auto reserved = function_builder.reserve_value(type);
                  if (!reserved) {
                    edit_error = builder_failure(name, block.label,
                                                 "reserve store immediate",
                                                 reserved.error());
                    return Result<void, BuildError>::failure(reserved.error());
                  }
                  auto defined = function_builder.define_int_constant(
                      reserved.value(), static_cast<std::int64_t>(immediate->value));
                  if (!defined) {
                    edit_error = builder_failure(name, block.label,
                                                 "define store immediate",
                                                 defined.error());
                    return defined;
                  }
                  stored_value = reserved.value();
                } else {
                  const auto found = source_values.find(
                      store->val.value_id()->value);
                  if (found == source_values.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated inline-asm Store source disappeared from the current-function registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  stored_value = found->second;
                }
                const auto destination =
                    global_objects.find(*store->ptr.link_name_id());
                if (destination == global_objects.end()) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated store destination disappeared from the global registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidGlobalObject);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    StoreSpec{destination->second, type, stored_value});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append store",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (const auto* load = std::get_if<LirLoadOp>(&instruction)) {
                if (has_overflow_carrier && load->result.value_id() &&
                    (*load->result.value_id() == overflow_carrier->overflow_pointer_load ||
                     *load->result.value_id() == overflow_carrier->final_load))
                  continue;
                const Type type = *lower_lir_type(module, load->type_str);
                if (load->local_object_authority) {
                  const auto& authority = *load->local_object_authority;
                  const auto owner = imported_link_names.find(authority.owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "validated local load authority owner disappeared from imported name identities"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  const auto pointer = source_values.find(authority.pointer_definition.value);
                  if (pointer == source_values.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "validated local load pointer disappeared from the current-function source registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  auto appended = function_builder.append(
                      blocks.at(block.id.value), LocalLoadAuthoritySpec{
                          SourceValueId{function_ids[function_index], load->result.value_id()->value},
                          SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                          SourceObjectId{function_ids[function_index], authority.object.value},
                          owner->second, Type{TypeKind::Pointer}, type, authority.live});
                  if (!appended) {
                    edit_error = builder_failure(name, block.label,
                                                 "append local load authority", appended.error());
                    return Result<void, BuildError>::failure(appended.error());
                  }
                  if (appended.value().results.size() != 1 ||
                      !source_values.emplace(load->result.value_id()->value,
                                             appended.value().results[0]).second) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "local load result collided in the current-function source registry"};
                    return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                  }
                  continue;
                }
                const auto source =
                    global_objects.find(*load->ptr.link_name_id());
                if (source == global_objects.end()) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated load source disappeared from the global registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidGlobalObject);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    LoadSpec{source->second, type,
                             load->result.value_id()->value});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append load",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1) {
                  edit_error = ImportError{
                      ImportErrorCode::BuilderFailure, name, block.label,
                      "load builder returned an inconsistent result count"};
                  return Result<void, BuildError>::failure(
                      BuildError::StorageExhausted);
                }
                const auto registered = source_values.emplace(
                    load->result.value_id()->value,
                    appended.value().results[0]);
                if (!registered.second) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "load result collided in the current-function source registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::DuplicateSourceValue);
                }
                continue;
              }
              if (const auto* stack_save = std::get_if<codegen::lir::LirStackSaveOp>(&instruction)) {
                const auto& authority = *stack_save->local_object_authority;
                const auto owner = imported_link_names.find(authority.owner);
                if (owner == imported_link_names.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                      name, block.label, "validated VLA stack-save owner disappeared"};
                  return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value), StackSaveAuthoritySpec{
                        SourceValueId{function_ids[function_index], stack_save->result.value_id()->value},
                        SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                        SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                        *lower_lir_type(module, authority.pointer_type),
                        *lower_lir_type(module, authority.pointee_type), authority.live});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append VLA stack-save authority", appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1 ||
                    !source_values.emplace(stack_save->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                      name, block.label, "VLA stack-save result collided in the current-function source registry"};
                  return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                }
                continue;
              }
              if (const auto* stack_restore = std::get_if<codegen::lir::LirStackRestoreOp>(&instruction)) {
                const auto& authority = *stack_restore->local_object_authority;
                const auto saved = source_values.find(stack_restore->saved_ptr.value_id()->value);
                const auto owner = imported_link_names.find(authority.owner);
                if (saved == source_values.end() || owner == imported_link_names.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                      name, block.label, "validated VLA stack-restore checkpoint disappeared"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value), StackRestoreAuthoritySpec{
                        SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                        SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                        *lower_lir_type(module, authority.pointer_type),
                        *lower_lir_type(module, authority.pointee_type), authority.live});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append VLA stack-restore authority", appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (const auto* va_start = std::get_if<codegen::lir::LirVaStartOp>(&instruction)) {
                if (!va_start->requires_native_memory_va_authority)
                  continue;
                const auto ap = va_start->ap_ptr.value_id();
                if (!ap || !va_start->ap_authority ||
                    *ap != va_start->ap_authority->local_pointer.pointer_definition) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                      name, block.label, "selected va_start authority lost its direct-local pointer identity"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                const auto& authority = va_start->ap_authority->local_pointer;
                const auto pointer = source_values.find(authority.pointer_definition.value);
                const auto owner = imported_link_names.find(authority.owner);
                if (pointer == source_values.end() || owner == imported_link_names.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                      name, block.label, "validated va_start pointer authority disappeared"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value), VaStartAuthoritySpec{
                        SourceValueId{function_ids[function_index], ap->value},
                        SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                        SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                        *lower_lir_type(module, authority.pointer_type),
                        *lower_lir_type(module, authority.pointee_type), authority.live});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append va_start authority", appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (const auto* gep = std::get_if<LirGepOp>(&instruction)) {
                if (has_overflow_carrier && gep->result.value_id() &&
                    *gep->result.value_id() == overflow_carrier->overflow_field_address)
                  continue;
                if (gep->requires_native_local_gep_authority) {
                  const auto& authority = *gep->local_object_authority;
                  const auto base = source_values.find(authority.pointer_definition.value);
                  const auto owner = imported_link_names.find(authority.owner);
                  const auto immediate = gep->indices.front().value().integer_immediate();
                  if (base == source_values.end() || owner == imported_link_names.end() || !immediate) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated local-array getelementptr authority disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  auto appended = function_builder.append(
                      blocks.at(block.id.value), LocalArrayGepAuthoritySpec{
                          SourceValueId{function_ids[function_index], gep->result.value_id()->value},
                          SourceValueId{function_ids[function_index], authority.pointer_definition.value},
                          SourceObjectId{function_ids[function_index], authority.object.value}, owner->second,
                          *lower_lir_type(module, authority.pointer_type),
                          *lower_lir_type(module, authority.pointee_type),
                          *lower_lir_type(module, gep->element_type),
                          static_cast<std::int64_t>(immediate->value), authority.live});
                  if (!appended) {
                    edit_error = builder_failure(name, block.label,
                                                 "append local-array getelementptr", appended.error());
                    return Result<void, BuildError>::failure(appended.error());
                  }
                  if (appended.value().results.size() != 1) {
                    edit_error = ImportError{ImportErrorCode::BuilderFailure, name, block.label,
                                             "local-array getelementptr builder returned an inconsistent result count"};
                    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
                  }
                  const auto registered = source_values.emplace(
                      gep->result.value_id()->value, appended.value().results[0]);
                  if (!registered.second) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name, block.label,
                                             "local-array getelementptr result collided in the current-function source registry"};
                    return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                  }
                  continue;
                }
                if (selected_body_parameter != function.native_body_parameter_definitions.end() &&
                    gep->ptr.value_id() &&
                    *gep->ptr.value_id() == selected_body_parameter->value) {
                  const auto owner = imported_link_names.find(selected_body_parameter->owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated body-parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  std::vector<ValueId> indices;
                  indices.reserve(gep->indices.size());
                  for (const auto& index : gep->indices) {
                    const auto immediate = index.value().integer_immediate();
                    if (!immediate) {
                      edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                               block.label, "validated body-parameter GEP index disappeared"};
                      return Result<void, BuildError>::failure(BuildError::InvalidValue);
                    }
                    auto reserved = function_builder.reserve_value(*lower_lir_type(module, index.type_ref()));
                    if (!reserved) return Result<void, BuildError>::failure(reserved.error());
                    auto defined = function_builder.define_int_constant(
                        reserved.value(), static_cast<std::int64_t>(immediate->value));
                    if (!defined) return defined;
                    indices.push_back(reserved.value());
                  }
                  auto appended = function_builder.append(
                      blocks.at(block.id.value), GetElementPtrSpec{
                          DirectPointerBodyParameterGepBase{
                              selected_body_parameter->value.value,
                              selected_body_parameter->parameter_index,
                              Type{TypeKind::Pointer}, owner->second},
                          *lower_lir_type(module, gep->element_type), gep->inbounds,
                          std::move(indices), gep->result.value_id()->value});
                  if (!appended) {
                    edit_error = builder_failure(name, block.label,
                                                 "append direct body-parameter getelementptr", appended.error());
                    return Result<void, BuildError>::failure(appended.error());
                  }
                  if (!source_values.emplace(gep->result.value_id()->value,
                                             appended.value().results[0]).second) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "body-parameter getelementptr result collided"};
                    return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                  }
                  continue;
                }
                GetElementPtrBase base;
                std::optional<Type> element_type;
                if (gep->ptr.kind() == codegen::lir::LirOperandKind::Global) {
                const auto global =
                    global_objects.find(*gep->ptr.link_name_id());
                if (global == global_objects.end()) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated getelementptr base disappeared from the global registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidGlobalObject);
                }
                const LirGlobal* selected = nullptr;
                for (const auto& global : module.globals)
                  if (global.link_name_id == *gep->ptr.link_name_id()) {
                    selected = &global;
                    break;
                  }
                if (!selected) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated getelementptr global type disappeared"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidGlobalObject);
                }
                base = global->second;
                element_type = lower_global_type(module, *selected);
                } else if (gep->ptr.kind() ==
                           codegen::lir::LirOperandKind::DirectConstant) {
                  const auto* value_id = gep->ptr.value_id();
                  const auto label = value_id
                      ? source_values.find(value_id->value)
                      : source_values.end();
                  if (!value_id || !value_id->valid() ||
                      label == source_values.end()) {
                    edit_error = ImportError{
                        ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                        block.label,
                        "validated direct label-address GEP base disappeared from the current-function registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  base = LabelAddressGepBase{label->second};
                  element_type = lower_lir_type(module, gep->element_type);
                } else {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated getelementptr base has an unsupported authority alternative"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                if (!element_type) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated getelementptr element type disappeared"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                std::vector<ValueId> indices;
                indices.reserve(gep->indices.size());
                for (const auto& index : gep->indices) {
                  const Type type =
                      *lower_lir_type(module, index.type_ref());
                  if (const auto* immediate =
                          index.value().integer_immediate()) {
                    auto reserved = function_builder.reserve_value(type);
                    if (!reserved) {
                      edit_error = builder_failure(
                          name, block.label,
                          "reserve getelementptr immediate index",
                          reserved.error());
                      return Result<void, BuildError>::failure(
                          reserved.error());
                    }
                    auto defined = function_builder.define_int_constant(
                        reserved.value(),
                        static_cast<std::int64_t>(immediate->value));
                    if (!defined) {
                      edit_error = builder_failure(
                          name, block.label,
                          "define getelementptr immediate index",
                          defined.error());
                      return defined;
                    }
                    indices.push_back(reserved.value());
                  } else {
                    const auto found = source_values.find(
                        index.value().value_id()->value);
                    if (found == source_values.end()) {
                      edit_error = ImportError{
                          ImportErrorCode::UnsupportedOrdinaryInstruction,
                          name, block.label,
                          "validated getelementptr SSA index disappeared from the current-function registry"};
                      return Result<void, BuildError>::failure(
                          BuildError::InvalidValue);
                    }
                    indices.push_back(found->second);
                  }
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    GetElementPtrSpec{
                        base, *element_type,
                        gep->inbounds, std::move(indices),
                        gep->result.value_id()->value});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append getelementptr",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1) {
                  edit_error = ImportError{
                      ImportErrorCode::BuilderFailure, name, block.label,
                      "getelementptr builder returned an inconsistent result count"};
                  return Result<void, BuildError>::failure(
                      BuildError::StorageExhausted);
                }
                const auto registered = source_values.emplace(
                    gep->result.value_id()->value,
                    appended.value().results[0]);
                if (!registered.second) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "getelementptr result collided in the current-function source registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::DuplicateSourceValue);
                }
                continue;
              }
              if (const auto* call = std::get_if<LirCallOp>(&instruction)) {
                if (call->intrinsic_kind) {
                  const auto link = imported_link_names.find(
                      call->direct_callee_link_name_id);
                  if (link == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated intrinsic LinkNameId disappeared from the module link registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  std::vector<ValueId> arguments;
                  arguments.reserve(call->structured_args.size());
                  for (const auto& argument : call->structured_args) {
                    const Type type = *lower_lir_type(module, argument.type_ref);
                    if (const auto* immediate = argument.operand.integer_immediate()) {
                      auto reserved = function_builder.reserve_value(type);
                      if (!reserved) {
                        edit_error = builder_failure(name, block.label,
                                                     "reserve intrinsic immediate argument", reserved.error());
                        return Result<void, BuildError>::failure(reserved.error());
                      }
                      auto defined = function_builder.define_int_constant(
                          reserved.value(), static_cast<std::int64_t>(immediate->value));
                      if (!defined) {
                        edit_error = builder_failure(name, block.label,
                                                     "define intrinsic immediate argument", defined.error());
                        return defined;
                      }
                      arguments.push_back(reserved.value());
                    } else {
                      const auto found = source_values.find(argument.operand.value_id()->value);
                      if (found == source_values.end()) {
                        edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                                 name, block.label,
                                                 "validated intrinsic SSA argument disappeared from the current-function registry"};
                        return Result<void, BuildError>::failure(BuildError::InvalidValue);
                      }
                      arguments.push_back(found->second);
                    }
                  }
                  IntrinsicKind kind = IntrinsicKind::Ctpop;
                  if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Cttz)
                    kind = IntrinsicKind::Cttz;
                  else if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Ctlz)
                    kind = IntrinsicKind::Ctlz;
                  const std::optional<bool> behavior = call->zero_count_behavior
                      ? std::optional<bool>{*call->zero_count_behavior ==
                                             codegen::lir::LirZeroCountBehavior::Undefined}
                      : std::nullopt;
                  auto appended = function_builder.append(
                      blocks.at(block.id.value),
                      IntrinsicCallSpec{kind, link->second,
                                        *lower_lir_type(module, call->return_type),
                                        std::move(arguments), behavior,
                                        call->result.value_id()->value});
                  if (!appended) {
                    edit_error = builder_failure(name, block.label,
                                                 "append intrinsic call", appended.error());
                    return Result<void, BuildError>::failure(appended.error());
                  }
                  if (appended.value().results.size() != 1) {
                    edit_error = ImportError{ImportErrorCode::BuilderFailure, name, block.label,
                                             "intrinsic call builder returned an inconsistent result count"};
                    return Result<void, BuildError>::failure(BuildError::StorageExhausted);
                  }
                  const auto registered = source_values.emplace(
                      call->result.value_id()->value, appended.value().results[0]);
                  if (!registered.second) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "intrinsic result collided in the current-function source registry"};
                    return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                  }
                  if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Cttz &&
                      *lower_lir_type(module, call->return_type) ==
                          Type{TypeKind::Integer, 32, "i32"})
                    native_i32_cttz_results.insert(call->result.value_id()->value);
                  if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Cttz &&
                      call->zero_count_behavior ==
                          codegen::lir::LirZeroCountBehavior::Undefined)
                    builtin_ctz_results.insert(call->result.value_id()->value);
                  if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Ctlz &&
                      call->zero_count_behavior ==
                          codegen::lir::LirZeroCountBehavior::Undefined)
                    builtin_clz_results.insert(call->result.value_id()->value);
                  if (*call->intrinsic_kind == codegen::lir::LirIntrinsicKind::Ctpop)
                    builtin_ctpop_results.insert(call->result.value_id()->value);
                  continue;
                }
                const auto callee = functions_by_link_name_id.find(
                    call->direct_callee_link_name_id);
                if (callee == functions_by_link_name_id.end()) {
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                      block.label,
                      "validated call target disappeared from the function LinkNameId registry"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidFunction);
                }
                std::vector<ValueId> arguments;
                arguments.reserve(call->structured_args.size());
                for (const auto& argument : call->structured_args) {
                  const Type type = *lower_lir_type(module, argument.type_ref);
                  if (const auto* immediate = argument.operand.integer_immediate()) {
                    auto reserved = function_builder.reserve_value(type);
                    if (!reserved) {
                      edit_error = builder_failure(name, block.label,
                                                   "reserve call immediate argument",
                                                   reserved.error());
                      return Result<void, BuildError>::failure(reserved.error());
                    }
                    auto defined = function_builder.define_int_constant(
                        reserved.value(), static_cast<std::int64_t>(immediate->value));
                    if (!defined) {
                      edit_error = builder_failure(name, block.label,
                                                   "define call immediate argument",
                                                   defined.error());
                      return defined;
                    }
                    arguments.push_back(reserved.value());
                  } else {
                    const auto found = source_values.find(
                        argument.operand.value_id()->value);
                    if (found == source_values.end()) {
                      edit_error = ImportError{
                          ImportErrorCode::UnsupportedOrdinaryInstruction,
                          name, block.label,
                          "validated call SSA argument disappeared from the current-function registry"};
                      return Result<void, BuildError>::failure(BuildError::InvalidValue);
                    }
                    arguments.push_back(found->second);
                  }
                }
                const bool integer_result =
                    call->return_type.kind() == codegen::lir::LirTypeKind::Integer;
                const bool native_floating_result =
                    call->return_type == codegen::lir::LirTypeRef("double");
                std::optional<DirectScalarBodyParameterFixedDirectCallArgument>
                    direct_scalar_argument;
                std::optional<DirectZeroArgScalarFloatingCallResult>
                    direct_floating_result;
                std::optional<DirectOneDoubleArgScalarFloatingCallResult>
                    direct_one_double_floating_result;
                for (std::size_t index = 0; index < call->structured_args.size(); ++index) {
                  if (!call->structured_args[index]
                           .fixed_direct_call_argument_parameter_authority)
                    continue;
                  const auto& authority = *call->structured_args[index]
                                               .fixed_direct_call_argument_parameter_authority;
                  const auto owner = imported_link_names.find(authority.owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated fixed direct-call parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  direct_scalar_argument =
                      DirectScalarBodyParameterFixedDirectCallArgument{
                          authority.value.value, authority.parameter_index,
                          static_cast<std::uint32_t>(index),
                          *lower_lir_type(module, authority.type), owner->second};
                  break;
                }
                if (call->direct_zero_arg_scalar_floating_call_authority) {
                  const auto& authority =
                      *call->direct_zero_arg_scalar_floating_call_authority;
                  const auto owner = imported_link_names.find(authority.owner);
                  const auto callee_link =
                      imported_link_names.find(authority.callee);
                  if (owner == imported_link_names.end() ||
                      callee_link == imported_link_names.end()) {
                    edit_error = ImportError{
                        ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "validated direct floating call-result authority link name disappeared"};
                    return Result<void, BuildError>::failure(
                        BuildError::InvalidNameId);
                  }
                  direct_floating_result =
                      DirectZeroArgScalarFloatingCallResult{
                          authority.result.value, owner->second,
                          callee_link->second,
                          *lower_lir_type(module, authority.return_type),
                          DirectZeroArgScalarFloatingCallRole::
                              ResultIntoFloatingBinaryLhs};
                }
                if (call->direct_one_double_arg_scalar_floating_call_authority) {
                  const auto& authority =
                      *call->direct_one_double_arg_scalar_floating_call_authority;
                  const auto owner = imported_link_names.find(authority.owner);
                  const auto callee_link =
                      imported_link_names.find(authority.callee);
                  if (owner == imported_link_names.end() ||
                      callee_link == imported_link_names.end()) {
                    edit_error = ImportError{
                        ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "validated direct one-double call-result authority link name disappeared"};
                    return Result<void, BuildError>::failure(
                        BuildError::InvalidNameId);
                  }
                  direct_one_double_floating_result =
                      DirectOneDoubleArgScalarFloatingCallResult{
                          authority.result.value, owner->second,
                          callee_link->second,
                          *lower_lir_type(module, authority.return_type),
                          *lower_lir_type(module, authority.argument_type),
                          DirectOneDoubleArgScalarFloatingCallRole::
                              DirectCallResult};
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    CallSpec{callee->second, std::move(arguments),
                             (integer_result || native_floating_result)
                                 ? std::optional<std::uint32_t>{call->result.value_id()->value}
                                 : std::nullopt,
                             direct_scalar_argument,
                             direct_floating_result,
                             direct_one_double_floating_result});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append call",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (((integer_result || native_floating_result) &&
                     appended.value().results.size() != 1) ||
                    (!integer_result && !native_floating_result &&
                     !appended.value().results.empty())) {
                  edit_error = ImportError{
                      ImportErrorCode::BuilderFailure, name, block.label,
                      "call builder returned an inconsistent result count"};
                  return Result<void, BuildError>::failure(
                      BuildError::StorageExhausted);
                }
                if (integer_result || native_floating_result) {
                  const auto registered = source_values.emplace(
                      call->result.value_id()->value, appended.value().results[0]);
                  if (!registered.second) {
                    edit_error = ImportError{
                        ImportErrorCode::UnsupportedOrdinaryInstruction,
                        name, block.label,
                        "call result collided in the current-function source registry"};
                    return Result<void, BuildError>::failure(
                        BuildError::DuplicateSourceValue);
                  }
                  if (native_floating_result)
                    native_floating_call_results.insert(
                        call->result.value_id()->value);
                }
                continue;
              }
              if (const auto* bin = std::get_if<LirBinOp>(&instruction)) {
                const auto* scalar_authority = bin->scalar_lhs_parameter_authority
                    ? &*bin->scalar_lhs_parameter_authority : nullptr;
                const auto* scalar_rhs_authority = bin->scalar_rhs_parameter_authority
                    ? &*bin->scalar_rhs_parameter_authority : nullptr;
                const auto* lhs_id = bin->lhs.value_id();
                const bool fadd = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FAdd};
                const bool fneg = scalar_authority && bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FNeg};
                const bool fmul = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
                    !scalar_authority && downstream_double_fadd_results.count(bin->lhs.value_id()->value) == 1;
                const bool direct_scalar_lhs_fadd = scalar_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FAdd};
                const bool direct_scalar_lhs_fsub = scalar_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FSub};
                const bool direct_scalar_lhs_fmul = scalar_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul};
                const bool direct_scalar_rhs_fmul = scalar_rhs_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul};
                const bool direct_scalar_rhs_fadd = scalar_rhs_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FAdd};
                const bool direct_scalar_rhs_fsub = scalar_rhs_authority &&
                    bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FSub};
                const bool float_fmul = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
                    scalar_fptrunc_results.count(bin->lhs.value_id()->value) == 1;
                const bool fpext_fmul = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
                    scalar_fpext_results.count(bin->lhs.value_id()->value) == 1;
                const bool sitofp_fmul = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
                    scalar_sitofp_results.count(bin->lhs.value_id()->value) == 1;
                const bool uitofp_fmul = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::FMul} &&
                    bin->type_str.kind() == codegen::lir::LirTypeKind::Floating &&
                    bin->type_str.str() == "double" &&
                    scalar_uitofp_results.count(bin->lhs.value_id()->value) == 1;
                const bool fptosi_add = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::Add} &&
                    bin->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                    bin->type_str.integer_bit_width() == 32 &&
                    lhs_id && scalar_fptosi_results.count(lhs_id->value) == 1;
                const bool fptoui_add = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::Add} &&
                    bin->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                    bin->type_str.integer_bit_width() == 32 &&
                    lhs_id && scalar_fptoui_results.count(lhs_id->value) == 1;
                const bool wide_ffs_trunc_add = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::Add} &&
                    bin->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                    bin->type_str.integer_bit_width() == 32 &&
                    lhs_id && wide_ffs_trunc_results.count(lhs_id->value) == 1;
                const bool add = bin->opcode.typed() ==
                    std::optional{codegen::lir::LirBinaryOpcode::Add};
                const bool abs_add = add && lhs_id && selected_global_i32_abs_results.count(lhs_id->value) == 1;
                const bool cttz_add = add && lhs_id && native_i32_cttz_results.count(lhs_id->value) == 1;
                const bool ctz_trunc_add = add && lhs_id && builtin_ctz_trunc_results.count(lhs_id->value) == 1;
                const bool clz_direct_add = add && lhs_id && builtin_clz_results.count(lhs_id->value) == 1;
                const bool clz_trunc_add = add && lhs_id && builtin_clz_trunc_results.count(lhs_id->value) == 1;
                const bool ctpop_direct_add = add && lhs_id && builtin_ctpop_results.count(lhs_id->value) == 1;
                const bool ctpop_trunc_add = add && lhs_id && builtin_ctpop_trunc_results.count(lhs_id->value) == 1;
                const bool sext_add = add && lhs_id && scalar_sext_results.count(lhs_id->value) == 1;
                const auto lhs = bin->lhs.value_id()
                    ? source_values.find(bin->lhs.value_id()->value) : source_values.end();
                if (((!scalar_rhs_authority || direct_scalar_rhs_fadd ||
                      direct_scalar_rhs_fsub || direct_scalar_rhs_fmul) &&
                     lhs == source_values.end()) ||
                    (fadd && !direct_scalar_lhs_fadd && !direct_scalar_rhs_fadd &&
                     native_floating_call_results.count(
                        bin->lhs.value_id()->value) == 0) ||
                    (!fneg && !direct_scalar_lhs_fadd && !direct_scalar_lhs_fsub && !direct_scalar_lhs_fmul && !direct_scalar_rhs_fadd && !direct_scalar_rhs_fsub && !direct_scalar_rhs_fmul && !fadd && !fmul && !fpext_fmul && !sitofp_fmul && !uitofp_fmul && !fptosi_add && !fptoui_add && !wide_ffs_trunc_add && !float_fmul && !sext_add && !abs_add && !cttz_add && !ctz_trunc_add && !clz_direct_add && !clz_trunc_add && !ctpop_direct_add && !add && normalized_i32_add_results.count(
                        bin->lhs.value_id()->value) == 0)) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "validated binary operands disappeared from the current-function registry"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                ValueId rhs_value{};
                if (!fneg && (scalar_rhs_authority || direct_scalar_lhs_fsub || direct_scalar_lhs_fmul || fadd || fmul || fpext_fmul || sitofp_fmul || uitofp_fmul || float_fmul)) {
                  const auto rhs = source_values.find(bin->rhs.value_id()->value);
                  if (rhs == source_values.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated double FAdd rhs disappeared from the current-function registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  rhs_value = rhs->second;
                } else if (!fneg) {
                  const bool i64_add = add && bin->type_str.kind() == codegen::lir::LirTypeKind::Integer &&
                      bin->type_str.integer_bit_width() == 64;
                  auto reserved = function_builder.reserve_value(
                      (sext_add || i64_add) ? Type{TypeKind::Integer, 64, "i64"}
                                               : Type{TypeKind::Integer, 32, "i32"});
                  if (!reserved) {
                    edit_error = builder_failure(name, block.label,
                                                 add ? "reserve normalized Add immediate"
                                                     : sext_add ? "reserve SExt Add immediate"
                                                                : "reserve normalized Mul immediate",
                                                 reserved.error());
                    return Result<void, BuildError>::failure(reserved.error());
                  }
                  auto defined = function_builder.define_int_constant(
                      reserved.value(), add ? 1 : 2);
                  if (!defined) {
                    edit_error = builder_failure(name, block.label,
                                                 add ? "define normalized Add immediate"
                                                     : sext_add ? "define SExt Add immediate"
                                                                : "define normalized Mul immediate",
                                                 defined.error());
                    return Result<void, BuildError>::failure(defined.error());
                  }
                  rhs_value = reserved.value();
                }
                ValueId lhs_value{};
                if (scalar_rhs_authority && !direct_scalar_rhs_fadd &&
                    !direct_scalar_rhs_fsub && !direct_scalar_rhs_fmul) {
                  auto reserved = function_builder.reserve_value(Type{TypeKind::Integer, 32, "i32"});
                  if (!reserved || !function_builder.define_int_constant(reserved.value(), 1)) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "define direct scalar RHS Add immediate"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  lhs_value = reserved.value();
                } else {
                  lhs_value = lhs->second;
                }
                std::optional<DirectScalarBodyParameterBinaryLhs> direct_scalar_lhs;
                std::optional<DirectScalarBodyParameterBinaryRhs> direct_scalar_rhs;
                if (scalar_authority) {
                  const auto owner = imported_link_names.find(scalar_authority->owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated scalar parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  const auto scalar_type = lower_lir_type(module, scalar_authority->type);
                  const Type retained_type = direct_scalar_lhs_fmul &&
                          scalar_type && scalar_type->spelling == "float"
                      ? Type{TypeKind::F32, 32, "float"}
                      : direct_scalar_lhs_fmul && scalar_type &&
                              scalar_type->spelling == "double"
                          ? Type{TypeKind::F64, 64, "double"}
                          : *scalar_type;
                  direct_scalar_lhs = DirectScalarBodyParameterBinaryLhs{
                      scalar_authority->value.value, scalar_authority->parameter_index,
                      retained_type, owner->second};
                }
                if (scalar_rhs_authority) {
                  const auto owner = imported_link_names.find(scalar_rhs_authority->owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated scalar RHS parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  const auto scalar_type = lower_lir_type(module, scalar_rhs_authority->type);
                  direct_scalar_rhs = DirectScalarBodyParameterBinaryRhs{
                      scalar_rhs_authority->value.value, scalar_rhs_authority->parameter_index,
                      (direct_scalar_rhs_fadd || direct_scalar_rhs_fsub ||
                       direct_scalar_rhs_fmul) ? *scalar_type : Type{TypeKind::Integer, 32, "i32"},
                      owner->second};
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    BinarySpec{fneg ? BinaryOpcode::FNeg
                                    : (fadd || direct_scalar_lhs_fadd) ? BinaryOpcode::FAdd
                                    : direct_scalar_lhs_fsub ? BinaryOpcode::FSub
                                    : direct_scalar_lhs_fmul ? BinaryOpcode::FMul
                                    : direct_scalar_rhs_fadd ? BinaryOpcode::FAdd
                                    : direct_scalar_rhs_fsub ? BinaryOpcode::FSub
                                    : direct_scalar_rhs_fmul ? BinaryOpcode::FMul
                                    : (fmul || fpext_fmul || sitofp_fmul || uitofp_fmul || float_fmul) ? BinaryOpcode::FMul
                                    : add ? BinaryOpcode::Add : BinaryOpcode::Mul,
                               fneg ? *lower_lir_type(module, scalar_authority->type)
                                    : direct_scalar_lhs_fadd ? direct_scalar_lhs->scalar_type
                                    : direct_scalar_lhs_fsub ? direct_scalar_lhs->scalar_type
                                    : direct_scalar_rhs_fadd ? direct_scalar_rhs->scalar_type
                                    : direct_scalar_rhs_fsub ? direct_scalar_rhs->scalar_type
                                    : fadd ? Type{TypeKind::F64, 64, "double"}
                                    : direct_scalar_lhs_fmul ? direct_scalar_lhs->scalar_type
                                    : direct_scalar_rhs_fmul ? direct_scalar_rhs->scalar_type
                                    : fmul ? Type{TypeKind::F64, 64, "double"}
                                    : fpext_fmul ? Type{TypeKind::F64, 64, "double"}
                                    : sitofp_fmul ? Type{TypeKind::F64, 64, "double"}
                                    : uitofp_fmul ? Type{TypeKind::F64, 64, "double"}
                                    : float_fmul ? Type{TypeKind::F32, 32, "float"}
                                    : (sext_add || (add && bin->type_str.integer_bit_width() == 64)) ? Type{TypeKind::Integer, 64, "i64"}
                                               : Type{TypeKind::Integer, 32, "i32"},
                               lhs_value, rhs_value,
                               bin->result.value_id()->value, direct_scalar_lhs, direct_scalar_rhs});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               fadd ? "append double FAdd"
                                                    : fneg ? "append direct scalar FNeg"
                                                    : direct_scalar_lhs_fsub ? "append direct scalar FSub"
                                                    : direct_scalar_lhs_fmul ? "append direct scalar FMul"
                                                    : fmul ? "append double FMul"
                                                    : fpext_fmul ? "append FPExt double FMul"
                                                    : sitofp_fmul ? "append SIToFP double FMul"
                                                    : uitofp_fmul ? "append UIToFP double FMul"
                                                    : float_fmul ? "append float FMul"
                                                    : add ? sext_add ? "append SExt i64 Add"
                                                                     : "append normalized i32 Add"
                                                          : "append normalized i32 Mul",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1 ||
                    !source_values.emplace(bin->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "binary result registration failed"};
                  return Result<void, BuildError>::failure(
                      BuildError::DuplicateSourceValue);
                }
                if (add && !sext_add)
                  normalized_i32_add_results.insert(bin->result.value_id()->value);
                if (fadd)
                  downstream_double_fadd_results.insert(bin->result.value_id()->value);
                if (fmul)
                  downstream_double_fmul_results.insert(bin->result.value_id()->value);
                continue;
              }
              if (const auto* abs = std::get_if<LirAbsOp>(&instruction)) {
                const auto operand = source_values.find(abs->arg.value_id()->value);
                if (operand == source_values.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "validated Abs source disappeared from the current-function registry"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    AbsSpec{Type{TypeKind::Integer, 32, "i32"}, operand->second,
                            abs->result.value_id()->value});
                if (!appended) {
                  edit_error = builder_failure(name, block.label, "append i32 Abs",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1 ||
                    !source_values.emplace(abs->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "Abs result registration failed"};
                  return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                }
                selected_global_i32_abs_results.insert(abs->result.value_id()->value);
                continue;
              }
              if (const auto* compare = std::get_if<LirCmpOp>(&instruction)) {
                const bool olt = compare->is_float;
                const bool ffs_zero = !olt &&
                    compare->predicate.typed() == std::optional{codegen::lir::LirCmpPredicate::Eq};
                const auto* truthiness_authority =
                    compare->truthiness_lhs_parameter_authority
                        ? &*compare->truthiness_lhs_parameter_authority : nullptr;
                const auto* pointer_truthiness_authority =
                    compare->pointer_truthiness_parameter_authority
                        ? &*compare->pointer_truthiness_parameter_authority
                        : nullptr;
                const bool truthiness_ne = truthiness_authority != nullptr;
                const bool pointer_truthiness_ne =
                    pointer_truthiness_authority != nullptr;
                ValueId lhs_value{};
                if (compare->lhs.kind() == codegen::lir::LirOperandKind::SsaValue) {
                  const auto lhs = source_values.find(compare->lhs.value_id()->value);
                  if (lhs == source_values.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated compare lhs disappeared from the current-function registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  lhs_value = lhs->second;
                } else if (ffs_zero) {
                  auto reserved = function_builder.reserve_value(*lower_lir_type(module, compare->type_str));
                  if (!reserved) {
                    edit_error = builder_failure(name, block.label, "reserve ffs compare lhs immediate", reserved.error());
                    return Result<void, BuildError>::failure(reserved.error());
                  }
                  auto defined = function_builder.define_int_constant(reserved.value(), compare->lhs.integer_immediate()->value);
                  if (!defined) {
                    edit_error = builder_failure(name, block.label, "define ffs compare lhs immediate", defined.error());
                    return defined;
                  }
                  lhs_value = reserved.value();
                } else {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "validated compare lhs disappeared from the current-function registry"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                ValueId rhs_value{};
                if (olt) {
                  const auto rhs = source_values.find(compare->rhs.value_id()->value);
                  if (rhs == source_values.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                             name, block.label,
                                             "validated double OLt rhs disappeared from the current-function registry"};
                    return Result<void, BuildError>::failure(BuildError::InvalidValue);
                  }
                  rhs_value = rhs->second;
                } else {
                  auto reserved = function_builder.reserve_value(*lower_lir_type(module, compare->type_str));
                  if (!reserved) {
                    edit_error = builder_failure(name, block.label,
                                                 "reserve compare immediate-seven", reserved.error());
                    return Result<void, BuildError>::failure(reserved.error());
                  }
                  auto defined = function_builder.define_int_constant(reserved.value(),
                                                                       (ffs_zero || truthiness_ne || pointer_truthiness_ne) ? 0 : 7);
                  if (!defined) {
                    edit_error = builder_failure(name, block.label,
                                                 "define compare immediate-seven", defined.error());
                    return defined;
                  }
                  rhs_value = reserved.value();
                }
                std::optional<DirectScalarBodyParameterTruthinessComparisonLhs>
                    direct_scalar_truthiness_lhs;
                std::optional<DirectPointerBodyParameterTruthiness>
                    direct_pointer_truthiness;
                if (truthiness_authority) {
                  const auto owner = imported_link_names.find(truthiness_authority->owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated truthiness parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  direct_scalar_truthiness_lhs =
                      DirectScalarBodyParameterTruthinessComparisonLhs{
                          truthiness_authority->value.value,
                          truthiness_authority->parameter_index,
                          *lower_lir_type(module, truthiness_authority->type), owner->second};
                }
                if (pointer_truthiness_authority) {
                  const auto owner =
                      imported_link_names.find(pointer_truthiness_authority->owner);
                  if (owner == imported_link_names.end()) {
                    edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                             block.label, "validated pointer truthiness parameter owner disappeared"};
                    return Result<void, BuildError>::failure(BuildError::InvalidNameId);
                  }
                  direct_pointer_truthiness =
                      DirectPointerBodyParameterTruthiness{
                          pointer_truthiness_authority->value.value,
                          pointer_truthiness_authority->parameter_index,
                          *lower_lir_type(module, pointer_truthiness_authority->type),
                          owner->second};
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    CompareSpec{olt ? ComparePredicate::OLt : ffs_zero ? ComparePredicate::Eq :
                                                   (truthiness_ne || pointer_truthiness_ne) ? ComparePredicate::Ne : ComparePredicate::Slt,
                                olt ? Type{TypeKind::F64, 64, "double"}
                                    : *lower_lir_type(module, compare->type_str),
                               lhs_value, rhs_value,
                                compare->result.value_id()->value,
                                direct_scalar_truthiness_lhs,
                                direct_pointer_truthiness});
                if (!appended || appended.value().results.size() != 1 ||
                    !source_values.emplace(compare->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = appended
                      ? ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                    block.label, "compare result registration failed"}
                      : builder_failure(name, block.label, "append i32 SLT compare",
                                        appended.error());
                  return Result<void, BuildError>::failure(
                      appended ? BuildError::DuplicateSourceValue : appended.error());
                }
                if (olt) downstream_double_olt_compare_results.insert(
                    compare->result.value_id()->value);
                if (ffs_zero) builtin_ffs_zero_compare_results.insert(
                    compare->result.value_id()->value);
                continue;
              }
              if (const auto* select = std::get_if<LirSelectOp>(&instruction)) {
                const auto condition = select->cond.value_id();
                const auto condition_value = condition ? source_values.find(condition->value)
                                                       : source_values.end();
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    SelectSpec{*lower_lir_type(module, select->type_str),
                               condition_value != source_values.end()
                                   ? std::optional<ValueId>{condition_value->second}
                                   : std::nullopt,
                               select->false_val.kind() == codegen::lir::LirOperandKind::SsaValue
                                   ? std::optional<ValueId>{source_values.at(select->false_val.value_id()->value)}
                                   : std::nullopt,
                               select->result.value_id()->value});
                if (!appended || appended.value().results.size() != 1 ||
                    !source_values.emplace(select->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = appended
                      ? ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                                    block.label, "wide ffs select result registration failed"}
                      : builder_failure(name, block.label, "append wide ffs select", appended.error());
                  return Result<void, BuildError>::failure(
                      appended ? BuildError::DuplicateSourceValue : appended.error());
                }
                wide_ffs_select_results.insert(select->result.value_id()->value);
                continue;
              }
              if (const auto* cast = std::get_if<LirCastOp>(&instruction)) {
                if (exact_double_olt_zext_use(*cast, downstream_double_olt_compare_results))
                  continue;
                const auto operand = source_values.find(cast->operand.value_id()->value);
                if (operand == source_values.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "validated intrinsic cast operand disappeared from the current-function registry"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                auto appended = function_builder.append(
                    blocks.at(block.id.value),
                    CastSpec{cast->kind == codegen::lir::LirCastKind::SExt
                                 ? CastKind::SExt
                                 : cast->kind == codegen::lir::LirCastKind::PtrToInt
                                     ? CastKind::PtrToInt
                                 : cast->kind == codegen::lir::LirCastKind::FPTrunc
                                     ? CastKind::FPTrunc
                                     : cast->kind == codegen::lir::LirCastKind::FPExt
                                         ? CastKind::FPExt
                                         : cast->kind == codegen::lir::LirCastKind::SIToFP
                                             ? CastKind::SIToFP
                                             : cast->kind == codegen::lir::LirCastKind::UIToFP
                                                 ? CastKind::UIToFP
                                                 : cast->kind == codegen::lir::LirCastKind::FPToSI
                                                     ? CastKind::FPToSI
                                                     : cast->kind == codegen::lir::LirCastKind::FPToUI
                                                         ? CastKind::FPToUI : CastKind::Trunc,
                             *lower_lir_type(module, cast->from_type),
                             *lower_lir_type(module, cast->to_type), operand->second,
                             cast->result.value_id()->value});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               cast->kind == codegen::lir::LirCastKind::FPExt
                                                   ? "append scalar FPExt cast"
                                                   : cast->kind == codegen::lir::LirCastKind::SIToFP
                                                       ? "append scalar SIToFP cast"
                                                   : cast->kind == codegen::lir::LirCastKind::UIToFP
                                                       ? "append scalar UIToFP cast"
                                                   : cast->kind == codegen::lir::LirCastKind::FPToSI
                                                       ? "append scalar FPToSI cast"
                                                   : cast->kind == codegen::lir::LirCastKind::FPToUI
                                                       ? "append scalar FPToUI cast"
                                                   : cast->kind == codegen::lir::LirCastKind::FPTrunc
                                                   ? "append scalar FPTrunc cast"
                                                   : cast->kind == codegen::lir::LirCastKind::SExt
                                                   ? "append scalar SExt cast"
                                                   : "append intrinsic trunc cast",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if (appended.value().results.size() != 1 ||
                    !source_values.emplace(cast->result.value_id()->value,
                                           appended.value().results[0]).second) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedOrdinaryInstruction,
                                           name, block.label,
                                           "intrinsic trunc cast result registration failed"};
                  return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
                }
                if (cast->kind == codegen::lir::LirCastKind::SExt)
                  scalar_sext_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::FPTrunc)
                  scalar_fptrunc_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::FPExt)
                  scalar_fpext_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::SIToFP)
                  scalar_sitofp_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::UIToFP)
                  scalar_uitofp_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::FPToSI)
                  scalar_fptosi_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::FPToUI)
                  scalar_fptoui_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::Trunc &&
                    cast->operand.value_id() &&
                    wide_ffs_select_results.count(cast->operand.value_id()->value) == 1)
                  wide_ffs_trunc_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::Trunc &&
                    cast->operand.value_id() &&
                    builtin_ctz_results.count(cast->operand.value_id()->value) == 1)
                  builtin_ctz_trunc_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::Trunc &&
                    cast->operand.value_id() &&
                    builtin_clz_results.count(cast->operand.value_id()->value) == 1)
                  builtin_clz_trunc_results.insert(cast->result.value_id()->value);
                if (cast->kind == codegen::lir::LirCastKind::Trunc &&
                    cast->operand.value_id() &&
                    builtin_ctpop_results.count(cast->operand.value_id()->value) == 1)
                  builtin_ctpop_trunc_results.insert(cast->result.value_id()->value);
                continue;
              }
              const auto& inline_asm = std::get<LirInlineAsmOp>(instruction);
              InlineAsmSpec spec;
              spec.asm_text = inline_asm.original_asm_text;
              spec.constraint_text = inline_asm.original_constraint_text;
              spec.side_effects = inline_asm.side_effects;
              spec.clobbers = inline_asm.clobbers;
              spec.inputs.reserve(inline_asm.ordinary_inputs.size());
              for (const auto& input : inline_asm.ordinary_inputs) {
                const auto found = ordinary_values.find(input.value.str());
                if (found == ordinary_values.end()) {
                  edit_error = ImportError{ImportErrorCode::UnsupportedInlineAsmShape,
                                           name, block.label,
                                           "structured input disappeared from the importer value map"};
                  return Result<void, BuildError>::failure(BuildError::InvalidValue);
                }
                spec.inputs.push_back(found->second.first);
              }
              spec.result_types.reserve(inline_asm.ordinary_results.size());
              for (const auto& result : inline_asm.ordinary_results) {
                spec.result_types.push_back(
                    *lower_lir_type(module, result.type));
              }
              const auto native_output_type = lower_lir_type(
                  module, inline_asm.ordinary_results.empty()
                              ? codegen::lir::LirTypeRef{}
                              : inline_asm.ordinary_results.front().type);
              const bool native_output = inline_asm.ordinary_inputs.empty() &&
                  inline_asm.ordinary_results.size() == 1 &&
                  inline_asm.ordinary_results.front().value.kind() ==
                      codegen::lir::LirOperandKind::SsaValue &&
                  inline_asm.ordinary_results.front().value.value_id() &&
                  inline_asm.ordinary_results.front().value.value_id()->valid() &&
                  inline_asm.ordinary_results.front().role == LirInlineAsmValueRole::Output &&
                  inline_asm.ordinary_results.front().constraint_index == 0 &&
                  native_output_type &&
                  is_native_inline_asm_output_type(*native_output_type);
              if (native_output)
                spec.source_result_id =
                    inline_asm.ordinary_results.front().value.value_id()->value;
              const auto source_result_id = spec.source_result_id;
              auto appended = function_builder.append(blocks.at(block.id.value),
                                                      std::move(spec));
              if (!appended) {
                edit_error = builder_failure(name, block.label,
                                             "append inline asm",
                                             appended.error());
                return Result<void, BuildError>::failure(appended.error());
              }
              if (appended.value().results.size() !=
                  inline_asm.ordinary_results.size()) {
                edit_error = ImportError{
                    ImportErrorCode::BuilderFailure, name, block.label,
                    "inline asm builder returned an inconsistent result count"};
                return Result<void, BuildError>::failure(
                    BuildError::StorageExhausted);
              }
              if (source_result_id &&
                  !source_values.emplace(*source_result_id,
                                         appended.value().results[0]).second) {
                edit_error = ImportError{ImportErrorCode::UnsupportedInlineAsmShape,
                                         name, block.label,
                                         "inline-asm source result registration collided"};
                return Result<void, BuildError>::failure(BuildError::DuplicateSourceValue);
              }
              for (std::size_t index = 0; index < inline_asm.ordinary_results.size(); ++index) {
                if (source_result_id && index == 0) continue;
                ordinary_values.emplace(
                    inline_asm.ordinary_results[index].value.str(),
                    std::pair{appended.value().results[index],
                              *lower_lir_type(module, inline_asm.ordinary_results[index].type)});
              }
            }
            Result<Terminator, ImportError> terminator =
                !block.insts.empty() &&
                        std::get_if<LirIndirectBrOp>(&block.insts.back())
                    ? [&]() -> Result<Terminator, ImportError> {
                        const auto& indirect_br =
                            std::get<LirIndirectBrOp>(block.insts.back());
                        const auto address = source_values.find(
                            indirect_br.addr_value->value);
                        if (address == source_values.end())
                          return fail<Terminator>(
                              ImportErrorCode::UnsupportedOrdinaryInstruction,
                              name, block.label,
                              "validated LirIndirectBrOp.addr_value disappeared from the current-function registry");
                        std::vector<BlockId> targets;
                        targets.reserve(indirect_br.successors.size());
                        for (const auto successor : indirect_br.successors) {
                          const auto target = blocks.find(successor.value);
                          if (target == blocks.end())
                            return fail<Terminator>(
                                ImportErrorCode::MissingBranchTarget, name,
                                block.label,
                                "validated LirIndirectBrOp successor disappeared from the current-function registry");
                          targets.push_back(target->second);
                        }
                        return Result<Terminator, ImportError>::success(
                            IndirectJumpTerm{address->second, std::move(targets)});
                      }()
                    : lower_terminator(module, imported_return_type,
                                       block.terminator, source_values,
                                       function_builder, blocks, name,
                                       block.label);
            if (!terminator) {
              edit_error = std::move(terminator.error());
              return Result<void, BuildError>::failure(
                  BuildError::UnsupportedOpcode);
            }
            auto set = function_builder.set_terminator(blocks.at(block.id.value),
                                                       std::move(terminator).value());
            if (!set) {
              edit_error = builder_failure(name, block.label, "set terminator",
                                           set.error());
              return Result<void, BuildError>::failure(set.error());
            }
          }
          return Result<void, BuildError>::success();
        });
    if (!edited) {
      if (edit_error)
        return Result<RawBir, ImportError>::failure(std::move(*edit_error));
      return Result<RawBir, ImportError>::failure(
          builder_failure(name, {}, "edit function", edited.error()));
    }
  }

  auto published = std::move(builder).publish();
  if (!published) {
    ImportError error{ImportErrorCode::PublicationFailure};
    error.detail = "foundation verification rejected imported RawBir";
    error.publish_error = published.error().reason;
    error.verification_errors = std::move(published.error().verification.errors);
    return Result<RawBir, ImportError>::failure(std::move(error));
  }
  return Result<RawBir, ImportError>::success(std::move(published).value());
}

Result<CanonicalBir, ImportError> lower_lir_to_canonical_bir(
    const LirModule& module, ImportOptions options) {
  auto raw = lower_lir_to_raw_bir(module, options);
  if (!raw)
    return Result<CanonicalBir, ImportError>::failure(std::move(raw.error()));
  auto canonical = canonicalize(std::move(raw).value());
  if (!canonical) {
    ImportError error{ImportErrorCode::PublicationFailure};
    error.detail = "canonical verification rejected imported RawBir";
    error.verification_errors = std::move(canonical.error().errors);
    return Result<CanonicalBir, ImportError>::failure(std::move(error));
  }
  return Result<CanonicalBir, ImportError>::success(
      std::move(canonical).value());
}

}  // namespace c4c::backend::bir
