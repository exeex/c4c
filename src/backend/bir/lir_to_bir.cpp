#include "lir_to_bir.hpp"

#include "../../codegen/lir/ir.hpp"

#include <algorithm>
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
using codegen::lir::LirCondBr;
using codegen::lir::LirFunction;
using codegen::lir::LirIndirectBr;
using codegen::lir::LirInlineAsmOp;
using codegen::lir::LirInlineAsmValueBinding;
using codegen::lir::LirInlineAsmValueRole;
using codegen::lir::LirModule;
using codegen::lir::LirRet;
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

bool has_intrinsic_requirements(const LirModule& module) noexcept {
  return module.need_va_start || module.need_va_end || module.need_va_copy ||
         module.need_memcpy || module.need_memset || module.need_stacksave ||
         module.need_stackrestore || module.need_abs || module.need_ptrmask ||
         module.prefer_semantic_va_ops;
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

std::optional<Type> lower_signature_type(
    const LirModule& module, const TypeSpec& structured,
    const std::optional<codegen::lir::LirTypeRef>& mirror) {
  if (structured.base != TB_VOID) return std::nullopt;

  StructuredTypeSpecFacts facts;
  facts.pointer_level = structured.ptr_level;
  facts.is_lvalue_reference = structured.is_lvalue_ref;
  facts.is_rvalue_reference = structured.is_rvalue_ref;
  facts.array_rank = structured.array_rank;
  facts.is_pointer_to_array = structured.is_ptr_to_array;
  facts.inner_array_rank = structured.inner_rank;
  facts.is_function_pointer = structured.is_fn_ptr;

  Type result{TypeKind::Void, 0, "void"};
  result.structured_spec = facts;
  if (!is_well_formed(result)) return std::nullopt;
  if (mirror) {
    const auto mirrored = lower_lir_type(module, *mirror);
    if (!mirrored || mirrored->kind != TypeKind::Void) return std::nullopt;
  }
  return result;
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
  if (!module.globals.empty())
    return fail<void>(ImportErrorCode::UnsupportedGlobals, {}, {},
                      "module globals require a migrated semantic family");
  if (!module.string_pool.empty() || !module.str_pool_map.empty() ||
      module.str_pool_idx != 0)
    return fail<void>(ImportErrorCode::UnsupportedStringPool, {}, {},
                      "module string-pool state is not in the bounded slice");
  if (!module.extern_decls.empty() || !module.extern_decl_link_name_map.empty() ||
      !module.extern_decl_name_map.empty())
    return fail<void>(ImportErrorCode::UnsupportedExternDeclarations, {}, {},
                      "extern-only declaration state requires explicit lowering");
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
  if (has_intrinsic_requirements(module))
    return fail<void>(ImportErrorCode::UnsupportedIntrinsicRequirements, {}, {},
                      "module intrinsic requirements are not yet represented");
  if (!module.spec_entries.empty())
    return fail<void>(ImportErrorCode::UnsupportedSpecializations, {}, {},
                      "specialization metadata is not yet represented");
  return Result<void, ImportError>::success();
}

Result<void, ImportError> validate_function(const LirModule& module,
                                            const LirFunction& function) {
  const std::string name = function_link_name(module, function);
  if (name.empty())
    return fail<void>(ImportErrorCode::EmptyFunctionLinkName, function.name, {},
                      "function has no resolvable link-visible name");
  if (!function.params.empty() || !function.signature_params.empty() ||
      !function.signature_param_type_refs.empty())
    return fail<void>(ImportErrorCode::UnsupportedFunctionParameters, name, {},
                      "parameter/signature expansion belongs to Step 4A");
  if (function.signature_is_variadic)
    return fail<void>(ImportErrorCode::UnsupportedVariadicFunction, name, {},
                      "variadic functions require explicit signature lowering");
  if (!lower_signature_type(module, function.return_type,
                            function.signature_return_type_ref))
    return fail<void>(ImportErrorCode::UnsupportedReturnType, name, {},
                      "structured return TypeSpec is malformed, non-void, or "
                      "conflicts with its optional mirror");

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
      const auto* inline_asm = std::get_if<LirInlineAsmOp>(&instruction);
      if (!inline_asm)
        return fail<void>(ImportErrorCode::UnsupportedOrdinaryInstruction, name,
                          block.label,
                          "only LirInlineAsmOp is in the bounded carrier slice");
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
            if (terminator.value_str || terminator.type_str != "void")
              return fail<void>(ImportErrorCode::InvalidVoidReturn, name,
                                block.label,
                                "LirRet must carry type 'void' and no value");
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
    const codegen::lir::LirTerminator& terminator,
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
          if (lir_terminator.value_str || lir_terminator.type_str != "void")
            return fail<Terminator>(ImportErrorCode::InvalidVoidReturn,
                                    function, block,
                                    "LirRet must carry type 'void' and no value");
          return Result<Terminator, ImportError>::success(ReturnTerm{});
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
  for (std::size_t index = 0; index < module.link_names.size(); ++index) {
    const auto source_id = static_cast<c4c::LinkNameId>(index + 1);
    auto added = builder.add_link_name(
        source_id, std::string(module.link_names.spelling(source_id)));
    if (!added)
      return Result<RawBir, ImportError>::failure(builder_failure(
          {}, {}, "import link-name table", added.error()));
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
  for (const auto& function : module.functions) {
    const std::string name = function_link_name(module, function);
    FunctionSignature signature;
    signature.return_type =
        *lower_signature_type(module, function.return_type,
                              function.signature_return_type_ref);
    auto created =
        builder.create_function(std::move(signature), name, function.is_declaration);
    if (!created)
      return Result<RawBir, ImportError>::failure(builder_failure(
          name, {}, "create function", created.error()));
    if (function.is_declaration) continue;

    std::optional<ImportError> edit_error;
    auto edited = builder.with_function(
        created.value(), [&](FunctionBuilder& function_builder) {
          std::unordered_map<std::string, BlockId> blocks;
          std::unordered_map<std::string, std::pair<ValueId, Type>> ordinary_values;
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
            auto terminator =
                lower_terminator(block.terminator, blocks, name, block.label);
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
