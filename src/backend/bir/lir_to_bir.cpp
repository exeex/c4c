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
using codegen::lir::LirBr;
using codegen::lir::LirCallOp;
using codegen::lir::LirCondBr;
using codegen::lir::LirConstFloat;
using codegen::lir::LirConstInt;
using codegen::lir::LirExtAttr;
using codegen::lir::LirExternDecl;
using codegen::lir::LirFunction;
using codegen::lir::LirGepOp;
using codegen::lir::LirGlobal;
using codegen::lir::LirIndirectBr;
using codegen::lir::LirInlineAsmOp;
using codegen::lir::LirInlineAsmValueBinding;
using codegen::lir::LirInlineAsmValueRole;
using codegen::lir::LirModule;
using codegen::lir::LirLoadOp;
using codegen::lir::LirRet;
using codegen::lir::LirStoreOp;
using codegen::lir::LirSwitch;
using codegen::lir::LirUnreachable;

template <class T>
Result<T, ImportError> fail(ImportErrorCode code, std::string function = {},
                            std::string block = {}, std::string detail = {}) {
  return Result<T, ImportError>::failure(
      {code, std::move(function), std::move(block), std::move(detail)});
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
    if (!supported_plain_parameter_base(logical.base) ||
        !supported_plain_parameter_base(signature.type.base) ||
        !default_parameter_type_metadata(logical) ||
        !default_parameter_type_metadata(signature.type) || signature.is_byval ||
        !same_default_parameter_type(logical, signature.type) ||
        mirror.has_struct_name_id())
      return std::nullopt;
    const auto type = lower_signature_type(module, signature.type, mirror);
    if (!type) return std::nullopt;
    if ((type->kind == TypeKind::Integer &&
         (mirror.kind() != codegen::lir::LirTypeKind::Integer ||
          !mirror.integer_bit_width() ||
          *mirror.integer_bit_width() != type->bit_width)) ||
        (type->kind == TypeKind::Floating &&
         mirror.kind() != codegen::lir::LirTypeKind::Floating) ||
        mirror.str() != type->spelling)
      return std::nullopt;
    lowered.push_back(*type);
  }
  return lowered;
}

bool exact_direct_void_call(const LirModule& module, const LirCallOp& call) {
  if (!call.result.empty() || call.result.has_authority() ||
      call.return_type.kind() != codegen::lir::LirTypeKind::Void ||
      call.return_type.str() != "void" ||
      call.return_ext_attr != LirExtAttr::None ||
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      !call.structured_args.empty() || !call.arg_type_refs.empty() ||
      !call.callee_signature)
    return false;

  const auto return_type = lower_lir_type(module, call.return_type);
  const auto& signature = *call.callee_signature;
  if (!return_type || return_type->kind != TypeKind::Void ||
      !signature.return_type_ref ||
      signature.return_type_ref->kind() !=
          codegen::lir::LirTypeKind::Void ||
      signature.return_type_ref->str() != "void" ||
      *signature.return_type_ref != call.return_type ||
      signature.return_ext_attr != LirExtAttr::None ||
      !signature.fixed_param_types.empty() ||
      !signature.fixed_param_type_refs.empty() || signature.is_variadic ||
      signature.has_unspecified_params)
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
      call.direct_callee_link_name_id == c4c::kInvalidLinkName ||
      !call.callee_signature)
    return false;
  const auto return_type = lower_lir_type(module, call.return_type);
  const auto& signature = *call.callee_signature;
  if (!return_type || !is_integer_type(*return_type) ||
      !signature.return_type_ref || *signature.return_type_ref != call.return_type ||
      signature.return_ext_attr != LirExtAttr::None || signature.is_variadic ||
      signature.has_unspecified_params || signature.has_void_param_list ||
      signature.fixed_param_types.size() != signature.fixed_param_type_refs.size() ||
      signature.fixed_param_type_refs.size() != call.structured_args.size() ||
      call.arg_type_refs.size() != call.structured_args.size())
    return false;
  std::vector<Type> parameter_types;
  parameter_types.reserve(signature.fixed_param_type_refs.size());
  for (std::size_t index = 0; index < call.structured_args.size(); ++index) {
    const auto& parameter_ref = signature.fixed_param_type_refs[index];
    const auto& argument = call.structured_args[index];
    const auto parameter_type = lower_lir_type(module, parameter_ref);
    if (!parameter_type || !is_integer_type(*parameter_type) ||
        signature.fixed_param_types[index] != parameter_ref.str() ||
        call.arg_type_refs[index] != parameter_ref ||
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
    const auto target_params = lower_function_parameter_types(module, target);
    if (!target_return || *target_return != *return_type || !target_params ||
        *target_params != parameter_types || target.signature_is_variadic)
      return false;
    resolved = true;
  }
  return resolved;
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
      !call.callee_type_suffix.empty() || !call.callee_signature)
    return false;
  const auto type = lower_lir_type(module, call.return_type);
  const auto& signature = *call.callee_signature;
  const bool count_flag = call.intrinsic_kind == LirIntrinsicKind::Cttz ||
                          call.intrinsic_kind == LirIntrinsicKind::Ctlz;
  const std::size_t count = count_flag ? 2 : 1;
  if (!type || !is_integer_type(*type) || !signature.return_type_ref ||
      *signature.return_type_ref != call.return_type ||
      signature.return_ext_attr != LirExtAttr::None || signature.is_variadic ||
      signature.has_unspecified_params || signature.has_void_param_list ||
      signature.fixed_param_types.size() != count ||
      signature.fixed_param_type_refs.size() != count ||
      call.arg_type_refs.size() != count || call.structured_args.size() != count ||
      signature.fixed_param_types[0] != call.return_type.str() ||
      signature.fixed_param_type_refs[0] != call.return_type ||
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
    if (signature.fixed_param_types[1] != "i1" ||
        signature.fixed_param_type_refs[1] != codegen::lir::LirTypeRef::integer(1) ||
        call.arg_type_refs[1] != codegen::lir::LirTypeRef::integer(1) ||
        flag.type != "i1" || flag.type_ref != codegen::lir::LirTypeRef::integer(1) ||
        flag.ext_attr != LirExtAttr::None || !immediate || immediate->value != expected)
      return false;
  }
  return true;
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

Result<void, ImportError> validate_inline_asm_shape(
    const LirModule& module, const LirInlineAsmOp& inline_asm,
    const std::string& function,
    const std::string& block,
    std::unordered_map<std::string, Type>& ordinary_values) {
  if (inline_asm.insn_r)
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmMetadata, function,
                      block,
                      "parsed insn.r metadata is not Raw/Canonical BIR authority");

  const bool has_structured_values = !inline_asm.ordinary_inputs.empty() ||
                                     !inline_asm.ordinary_results.empty();
  if (!has_structured_values &&
      (!inline_asm.args_str.empty() || !inline_asm.result.empty() ||
       inline_asm.ret_type.kind() != codegen::lir::LirTypeKind::Void ||
       inline_asm.ret_type.str() != "void")) {
    return fail<void>(
        ImportErrorCode::UnsupportedInlineAsmShape, function, block,
        "textual LLVM operands/results have no structured LIR value identities");
  }
  if (!inline_asm.result.empty() && inline_asm.ordinary_results.empty()) {
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                      block,
                      "LLVM compatibility result lacks a structured result identity");
  }

  const std::size_t constraint_count =
      inline_asm_constraint_count(inline_asm.original_constraint_text);
  if (has_structured_values && constraint_count == 0) {
    return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                      block,
                      "structured values require original semantic constraints");
  }

  std::optional<std::size_t> previous_input_constraint;
  for (const auto& input : inline_asm.ordinary_inputs) {
    if (input.value.kind() != codegen::lir::LirOperandKind::SsaValue) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured input must be an ordinary SSA identity");
    }
    if (input.role != LirInlineAsmValueRole::Input &&
        input.role != LirInlineAsmValueRole::ReadWrite) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block, "structured input carries an output-only role");
    }
    if (input.constraint_index >= constraint_count ||
        (previous_input_constraint &&
         input.constraint_index <= *previous_input_constraint)) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured inputs do not follow original constraint order");
    }
    previous_input_constraint = input.constraint_index;
    const auto type = lower_lir_type(module, input.type);
    const auto found = ordinary_values.find(input.value.str());
    if (!type || found == ordinary_values.end()) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured input does not resolve in the function value map");
    }
    if (found->second != *type) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured input type disagrees with its defining value");
    }
  }

  std::optional<std::size_t> previous_result_constraint;
  std::unordered_set<std::string> pending_results;
  pending_results.reserve(inline_asm.ordinary_results.size());
  for (const auto& result : inline_asm.ordinary_results) {
    if (result.value.kind() != codegen::lir::LirOperandKind::SsaValue ||
        (result.role != LirInlineAsmValueRole::Output &&
         result.role != LirInlineAsmValueRole::ReadWrite)) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block, "structured result identity or role is invalid");
    }
    if (result.constraint_index >= constraint_count ||
        (previous_result_constraint &&
         result.constraint_index <= *previous_result_constraint)) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured results do not follow original constraint order");
    }
    previous_result_constraint = result.constraint_index;
    const auto type = lower_lir_type(module, result.type);
    if (!type || ordinary_values.find(result.value.str()) != ordinary_values.end() ||
        !pending_results.insert(result.value.str()).second) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "structured result is duplicate or has unsupported type");
    }

    const LirInlineAsmValueBinding* matching_input = nullptr;
    for (const auto& input : inline_asm.ordinary_inputs) {
      if (input.value == result.value) {
        return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                          block,
                          "structured result must be distinct from every input");
      }
      if (input.constraint_index == result.constraint_index)
        matching_input = &input;
    }
    if (result.role == LirInlineAsmValueRole::ReadWrite) {
      if (!matching_input ||
          matching_input->role != LirInlineAsmValueRole::ReadWrite ||
          lower_lir_type(module, matching_input->type) != type) {
        return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                          block,
                          "read/write result lacks a same-typed old-value input");
      }
    } else if (matching_input) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "only read/write values may share a constraint position");
    }
  }
  for (const auto& input : inline_asm.ordinary_inputs) {
    if (input.role != LirInlineAsmValueRole::ReadWrite) continue;
    const auto paired = std::find_if(
        inline_asm.ordinary_results.begin(), inline_asm.ordinary_results.end(),
        [&](const LirInlineAsmValueBinding& result) {
          return result.role == LirInlineAsmValueRole::ReadWrite &&
                 result.constraint_index == input.constraint_index;
        });
    if (paired == inline_asm.ordinary_results.end()) {
      return fail<void>(ImportErrorCode::UnsupportedInlineAsmShape, function,
                        block,
                        "read/write input lacks a distinct produced result");
    }
  }

  for (const auto& result : inline_asm.ordinary_results) {
    ordinary_values.emplace(result.value.str(),
                            *lower_lir_type(module, result.type));
  }
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
  if (function.signature_is_variadic)
    return fail<void>(ImportErrorCode::UnsupportedVariadicFunction, name, {},
                      "variadic functions require explicit signature lowering");
  if (!lower_function_parameter_types(module, function))
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
  if (!function.alloca_insts.empty())
    return fail<void>(ImportErrorCode::UnsupportedAllocaInstructions, name, {},
                      "hoisted allocas require the memory family");

  std::unordered_set<std::string> labels;
  std::unordered_set<std::uint32_t> block_ids;
  std::unordered_map<std::string, Type> ordinary_values;
  std::unordered_map<std::uint32_t, Type> source_values;
  labels.reserve(function.blocks.size());
  block_ids.reserve(function.blocks.size());
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
    for (const auto& instruction : block.insts) {
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
        if (!type || !is_integer_type(*type) ||
            store->type_str.kind() !=
                codegen::lir::LirTypeKind::Integer ||
            !store->type_str.integer_bit_width() ||
            store->val.kind() != codegen::lir::LirOperandKind::Immediate ||
            !immediate ||
            !integer_immediate_representable(
                immediate->value, *store->type_str.integer_bit_width()) ||
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
        continue;
      }
      if (const auto* load = std::get_if<LirLoadOp>(&instruction)) {
        const auto type = lower_lir_type(module, load->type_str);
        const auto* result = load->result.value_id();
        const auto* source = load->ptr.link_name_id();
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
        continue;
      }
      if (const auto* gep = std::get_if<LirGepOp>(&instruction)) {
        const auto* result = gep->result.value_id();
        const auto* base = gep->ptr.link_name_id();
        if (gep->element_type.kind() !=
                codegen::lir::LirTypeKind::Array ||
            gep->result.kind() != codegen::lir::LirOperandKind::SsaValue ||
            !result || !result->valid() ||
            gep->ptr.kind() != codegen::lir::LirOperandKind::Global ||
            !base || *base == c4c::kInvalidLinkName || gep->indices.empty()) {
          return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction,
                            name, block.label,
                            "getelementptr requires authoritative result/base identities and nonempty typed array indices");
        }
        const LirGlobal* selected = nullptr;
        for (const auto& global : module.globals) {
          if (global.link_name_id != *base) continue;
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
          continue;
        }
        if (exact_direct_void_call(module, *call)) continue;
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
      const auto* inline_asm = std::get_if<LirInlineAsmOp>(&instruction);
      if (!inline_asm)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "instruction family is outside the bounded constant/value slice");
      auto checked = validate_inline_asm_shape(
          module, *inline_asm, name, block.label, ordinary_values);
      if (!checked) return checked;
    }
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
            if (terminator.target_label.empty() ||
                labels.find(terminator.target_label) == labels.end())
              return fail<void>(ImportErrorCode::MissingBranchTarget, name,
                                block.label, terminator.target_label);
          } else if constexpr (std::is_same_v<Term, LirUnreachable>) {
            // Supported directly.
          } else if constexpr (std::is_same_v<Term, LirCondBr>) {
            return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                              block.label, "conditional branch");
          } else if constexpr (std::is_same_v<Term, LirSwitch>) {
            return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                              block.label, "switch");
          } else if constexpr (std::is_same_v<Term, LirIndirectBr>) {
            return fail<void>(ImportErrorCode::UnsupportedTerminator, name,
                              block.label, "indirect branch");
          }
          return Result<void, ImportError>::success();
        },
        block.terminator);
    if (!checked) return checked;
  }
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
    const std::unordered_map<std::string, BlockId>& blocks,
    const std::string& function, const std::string& block) {
  return std::visit(
      [&](const auto& lir_terminator) -> Result<Terminator, ImportError> {
        using Term = std::decay_t<decltype(lir_terminator)>;
        if constexpr (std::is_same_v<Term, LirBr>) {
          const auto target = blocks.find(lir_terminator.target_label);
          if (target == blocks.end())
            return fail<Terminator>(ImportErrorCode::MissingBranchTarget,
                                    function, block,
                                    lir_terminator.target_label);
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
          return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                  function, block, "conditional branch");
        } else if constexpr (std::is_same_v<Term, LirSwitch>) {
          return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                  function, block, "switch");
        } else {
          static_assert(std::is_same_v<Term, LirIndirectBr>);
          return fail<Terminator>(ImportErrorCode::UnsupportedTerminator,
                                  function, block, "indirect branch");
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
  for (const auto& declaration : module.struct_decls) {
    std::vector<StructField> fields;
    fields.reserve(declaration.fields.size());
    for (const auto& field : declaration.fields)
      fields.push_back(StructField{*lower_lir_type(module, field.type)});
    auto added = builder.add_struct_declaration(
        declaration.name_id, std::move(fields), declaration.is_packed,
        declaration.is_opaque);
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
    const Type imported_return_type =
        *lower_signature_type(module, function.return_type,
                              function.signature_return_type_ref);
    FunctionSignature signature;
    signature.return_type = imported_return_type;
    signature.parameter_types =
        *lower_function_parameter_types(module, function);
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

  for (std::size_t function_index = 0;
       function_index < module.functions.size(); ++function_index) {
    const auto& function = module.functions[function_index];
    const std::string name = function_link_name(module, function);
    const Type imported_return_type =
        *lower_signature_type(module, function.return_type,
                              function.signature_return_type_ref);
    if (function.is_declaration) continue;

    std::optional<ImportError> edit_error;
    auto edited = builder.with_function(
        function_ids[function_index], [&](FunctionBuilder& function_builder) {
          std::unordered_map<std::string, BlockId> blocks;
          std::unordered_map<std::string, std::pair<ValueId, Type>> ordinary_values;
          std::unordered_map<std::uint32_t, ValueId> source_values;
          blocks.reserve(function.blocks.size());
          for (const LirBlock& block : function.blocks) {
            auto created_block = function_builder.create_block(block.label);
            if (!created_block) {
              edit_error = builder_failure(name, block.label, "create block",
                                           created_block.error());
              return Result<void, BuildError>::failure(created_block.error());
            }
            blocks.emplace(block.label, created_block.value());
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
              if (const auto* store = std::get_if<LirStoreOp>(&instruction)) {
                const Type type = *lower_lir_type(module, store->type_str);
                auto reserved = function_builder.reserve_value(type);
                if (!reserved) {
                  edit_error = builder_failure(name, block.label,
                                               "reserve store immediate",
                                               reserved.error());
                  return Result<void, BuildError>::failure(reserved.error());
                }
                auto defined = function_builder.define_int_constant(
                    reserved.value(),
                    static_cast<std::int64_t>(
                        store->val.integer_immediate()->value));
                if (!defined) {
                  edit_error = builder_failure(name, block.label,
                                               "define store immediate",
                                               defined.error());
                  return defined;
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
                    blocks.at(block.label),
                    StoreSpec{destination->second, type, reserved.value()});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append store",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                continue;
              }
              if (const auto* load = std::get_if<LirLoadOp>(&instruction)) {
                const Type type = *lower_lir_type(module, load->type_str);
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
                    blocks.at(block.label),
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
              if (const auto* gep = std::get_if<LirGepOp>(&instruction)) {
                const auto base =
                    global_objects.find(*gep->ptr.link_name_id());
                if (base == global_objects.end()) {
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
                    blocks.at(block.label),
                    GetElementPtrSpec{
                        base->second, *lower_global_type(module, *selected),
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
                      blocks.at(block.label),
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
                auto appended = function_builder.append(
                    blocks.at(block.label),
                    CallSpec{callee->second, std::move(arguments),
                             integer_result
                                 ? std::optional<std::uint32_t>{call->result.value_id()->value}
                                 : std::nullopt});
                if (!appended) {
                  edit_error = builder_failure(name, block.label,
                                               "append call",
                                               appended.error());
                  return Result<void, BuildError>::failure(appended.error());
                }
                if ((integer_result && appended.value().results.size() != 1) ||
                    (!integer_result && !appended.value().results.empty())) {
                  edit_error = ImportError{
                      ImportErrorCode::BuilderFailure, name, block.label,
                      "call builder returned an inconsistent result count"};
                  return Result<void, BuildError>::failure(
                      BuildError::StorageExhausted);
                }
                if (integer_result) {
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
                }
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
                  edit_error = ImportError{
                      ImportErrorCode::UnsupportedInlineAsmShape, name,
                      block.label,
                      "structured input disappeared from the importer value map"};
                  return Result<void, BuildError>::failure(
                      BuildError::InvalidValue);
                }
                spec.inputs.push_back(found->second.first);
              }
              spec.result_types.reserve(inline_asm.ordinary_results.size());
              for (const auto& result : inline_asm.ordinary_results) {
                spec.result_types.push_back(
                    *lower_lir_type(module, result.type));
              }
              auto appended = function_builder.append(blocks.at(block.label),
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
              for (std::size_t index = 0;
                   index < inline_asm.ordinary_results.size(); ++index) {
                ordinary_values.emplace(
                    inline_asm.ordinary_results[index].value.str(),
                    std::pair{appended.value().results[index],
                              *lower_lir_type(module,
                                  inline_asm.ordinary_results[index].type)});
              }
            }
            auto terminator = lower_terminator(
                module, imported_return_type, block.terminator, source_values,
                function_builder, blocks, name, block.label);
            if (!terminator) {
              edit_error = std::move(terminator.error());
              return Result<void, BuildError>::failure(
                  BuildError::UnsupportedOpcode);
            }
            auto set = function_builder.set_terminator(blocks.at(block.label),
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
