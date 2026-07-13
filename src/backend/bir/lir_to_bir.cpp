#include "lir_to_bir.hpp"

#include "../../codegen/lir/ir.hpp"

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

bool is_void_return(const LirFunction& function) noexcept {
  if (function.signature_return_type_ref)
    return function.signature_return_type_ref->kind() ==
           codegen::lir::LirTypeKind::Void;
  const auto& type = function.return_type;
  return type.base == TB_VOID && type.ptr_level == 0 && !type.is_lvalue_ref &&
         !type.is_rvalue_ref && type.array_rank == 0 && !type.is_fn_ptr;
}

bool has_intrinsic_requirements(const LirModule& module) noexcept {
  return module.need_va_start || module.need_va_end || module.need_va_copy ||
         module.need_memcpy || module.need_memset || module.need_stacksave ||
         module.need_stackrestore || module.need_abs || module.need_ptrmask ||
         module.prefer_semantic_va_ops;
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
  if (!module.type_decls.empty() || !module.struct_decls.empty() ||
      !module.struct_decl_index.empty() ||
      !module.structured_layout_observations.empty())
    return fail<void>(ImportErrorCode::UnsupportedTypeDeclarations, {}, {},
                      "module type declarations require explicit lowering");
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
                      "only zero-parameter functions are in this slice");
  if (function.signature_is_variadic)
    return fail<void>(ImportErrorCode::UnsupportedVariadicFunction, name, {},
                      "variadic functions require explicit signature lowering");
  if (!is_void_return(function))
    return fail<void>(ImportErrorCode::UnsupportedReturnType, name, {},
                      "only structured void returns are in this slice");

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
      if (inline_asm->insn_r)
        return fail<void>(ImportErrorCode::UnsupportedInlineAsmMetadata, name,
                          block.label,
                          "parsed insn.r metadata is not Raw/Canonical BIR authority");
      if (!inline_asm->result.empty() ||
          inline_asm->ret_type.kind() != codegen::lir::LirTypeKind::Void ||
          inline_asm->ret_type.str() != "void" || !inline_asm->args_str.empty())
        return fail<void>(
            ImportErrorCode::UnsupportedInlineAsmShape, name, block.label,
            "result-bearing or argument-bearing inline asm lacks structured "
            "LIR value identities and is rejected without parsing args_str");
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
  for (const auto& function : module.functions) {
    const std::string name = function_link_name(module, function);
    FunctionSignature signature;
    signature.return_type = Type{TypeKind::Void};
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
              spec.asm_text = inline_asm.asm_text;
              spec.constraint_text = inline_asm.constraints;
              spec.side_effects = inline_asm.side_effects;
              spec.clobbers = inline_asm.clobbers;
              auto appended = function_builder.append(blocks.at(block.label),
                                                      std::move(spec));
              if (!appended) {
                edit_error = builder_failure(name, block.label,
                                             "append inline asm",
                                             appended.error());
                return Result<void, BuildError>::failure(appended.error());
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
