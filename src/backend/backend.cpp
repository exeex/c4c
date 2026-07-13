#include "backend.hpp"

#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c4c::backend {
namespace {

const char* import_error_name(bir::ImportErrorCode code) {
  switch (code) {
#define C4C_IMPORT_ERROR_NAME(value) \
  case bir::ImportErrorCode::value:   \
    return #value
    C4C_IMPORT_ERROR_NAME(UnsupportedGlobals);
    C4C_IMPORT_ERROR_NAME(UnsupportedStringPool);
    C4C_IMPORT_ERROR_NAME(UnsupportedExternDeclarations);
    C4C_IMPORT_ERROR_NAME(UnsupportedTypeDeclarations);
    C4C_IMPORT_ERROR_NAME(UnsupportedIntrinsicRequirements);
    C4C_IMPORT_ERROR_NAME(UnsupportedSpecializations);
    C4C_IMPORT_ERROR_NAME(EmptyFunctionLinkName);
    C4C_IMPORT_ERROR_NAME(UnsupportedFunctionParameters);
    C4C_IMPORT_ERROR_NAME(UnsupportedVariadicFunction);
    C4C_IMPORT_ERROR_NAME(UnsupportedReturnType);
    C4C_IMPORT_ERROR_NAME(DeclarationHasBody);
    C4C_IMPORT_ERROR_NAME(DefinitionHasNoBlocks);
    C4C_IMPORT_ERROR_NAME(UnsupportedStackObjects);
    C4C_IMPORT_ERROR_NAME(UnsupportedAllocaInstructions);
    C4C_IMPORT_ERROR_NAME(EmptyBlockLabel);
    C4C_IMPORT_ERROR_NAME(DuplicateBlockLabel);
    C4C_IMPORT_ERROR_NAME(DuplicateBlockId);
    C4C_IMPORT_ERROR_NAME(InvalidEntryBlock);
    C4C_IMPORT_ERROR_NAME(UnsupportedEntryBlock);
    C4C_IMPORT_ERROR_NAME(UnsupportedOrdinaryInstruction);
    C4C_IMPORT_ERROR_NAME(InvalidVoidReturn);
    C4C_IMPORT_ERROR_NAME(MissingBranchTarget);
    C4C_IMPORT_ERROR_NAME(UnsupportedTerminator);
    C4C_IMPORT_ERROR_NAME(BuilderFailure);
    C4C_IMPORT_ERROR_NAME(PublicationFailure);
#undef C4C_IMPORT_ERROR_NAME
  }
  return "UnknownImportError";
}

std::string format_import_error(const bir::ImportError& error) {
  std::ostringstream out;
  out << "LIR-to-BIR import failed: " << import_error_name(error.code);
  if (!error.function.empty())
    out << " function='" << error.function << "'";
  if (!error.block.empty())
    out << " block='" << error.block << "'";
  if (!error.detail.empty())
    out << ": " << error.detail;
  if (error.build_error)
    out << " [build_error=" << static_cast<int>(*error.build_error) << "]";
  if (error.publish_error)
    out << " [publish_error=" << static_cast<int>(*error.publish_error) << "]";
  if (!error.verification_errors.empty())
    out << " [verification_errors=" << error.verification_errors.size() << "]";
  return out.str();
}

std::string block_name(bir::BlockId id) {
  return "bb" + std::to_string(id.slot);
}

std::string render_terminator(const bir::Terminator& terminator) {
  return std::visit(
      [](const auto& term) -> std::string {
        using Term = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<Term, bir::JumpTerm>) {
          return "jump " + block_name(term.target);
        } else if constexpr (std::is_same_v<Term, bir::CondJumpTerm>) {
          return "cond_jump value" + std::to_string(term.condition.slot) + " " +
                 block_name(term.true_target) + " " +
                 block_name(term.false_target);
        } else if constexpr (std::is_same_v<Term, bir::ReturnTerm>) {
          return term.value ? "return value" + std::to_string(term.value->slot)
                            : "return void";
        } else {
          return "unreachable";
        }
      },
      terminator);
}

std::string render_semantic_bir(const bir::RawBir& raw_bir) {
  std::ostringstream out;
  const auto module = raw_bir.view();
  out << "raw_bir\n";
  for (const auto function_id : module.functions()) {
    const auto resolved_function = module.function(function_id);
    if (!resolved_function)
      throw std::logic_error("verified RawBir contains an unresolvable function");
    const auto function = resolved_function.value();
    out << "function fn" << function_id.slot << " '" << function.link_name()
        << "' " << (function.is_declaration() ? "declaration" : "definition")
        << "\n";
    for (const auto block_id : function.blocks()) {
      const auto resolved_block = function.block(block_id);
      if (!resolved_block)
        throw std::logic_error("verified RawBir contains an unresolvable block");
      out << "  block " << block_name(block_id);
      const auto debug_name = resolved_block.value().debug_name();
      if (!debug_name.empty())
        out << " '" << debug_name << "'";
      out << "\n";
      const auto instructions = function.instructions(block_id);
      if (!instructions)
        throw std::logic_error(
            "verified RawBir block has unresolvable instruction order");
      for (const auto instruction_id : instructions.value()) {
        const auto instruction = function.instruction(instruction_id);
        if (!instruction)
          throw std::logic_error(
              "verified RawBir contains an unresolvable instruction");
        if (const auto* call = instruction.value().call())
          out << "    call fn" << call->callee.slot << "\n";
      }
      const auto terminator = function.terminator(block_id);
      if (!terminator)
        throw std::logic_error("verified RawBir block has no terminator");
      out << "    " << render_terminator(terminator.value()) << "\n";
    }
  }
  return out.str();
}

const char* dump_stage_name(BackendDumpStage stage) {
  switch (stage) {
    case BackendDumpStage::SemanticBir:
      return "SemanticBir";
    case BackendDumpStage::PreparedBir:
      return "PreparedBir";
    case BackendDumpStage::MirSummary:
      return "MirSummary";
    case BackendDumpStage::MirTrace:
      return "MirTrace";
  }
  return "unknown";
}

std::string bir_to_mir_unsupported() {
  return "BIR-to-MIR lowering is not implemented for the new verified BIR";
}

}  // namespace

BackendObjectResult emit_module_object(const BackendModuleInput& input,
                                       const BackendOptions&) {
  auto imported = bir::lower_lir_to_raw_bir(input.lir_module());
  if (!imported)
    return BackendObjectResult{{}, format_import_error(imported.error())};
  return BackendObjectResult{{}, bir_to_mir_unsupported() +
                                     "; object emission is unavailable"};
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  auto imported = bir::lower_lir_to_raw_bir(input.lir_module());
  if (!imported)
    throw std::runtime_error(format_import_error(imported.error()));
  if (options.emit_semantic_bir)
    return render_semantic_bir(imported.value());
  throw std::runtime_error(bir_to_mir_unsupported());
}

std::string dump_module(const BackendModuleInput& input,
                        const BackendOptions&,
                        BackendDumpStage stage) {
  auto imported = bir::lower_lir_to_raw_bir(input.lir_module());
  if (!imported)
    throw std::runtime_error(format_import_error(imported.error()));
  if (stage == BackendDumpStage::SemanticBir)
    return render_semantic_bir(imported.value());
  throw std::runtime_error(std::string("backend dump stage ") +
                           dump_stage_name(stage) +
                           " is unavailable: " + bir_to_mir_unsupported());
}

}  // namespace c4c::backend
