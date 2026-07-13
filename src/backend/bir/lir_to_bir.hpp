#pragma once

#include "core/builder.hpp"

#include <optional>
#include <string>
#include <vector>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend::bir {

struct ImportOptions {};

enum class ImportErrorCode {
  UnsupportedGlobals,
  UnsupportedStringPool,
  UnsupportedExternDeclarations,
  UnsupportedTypeDeclarations,
  UnsupportedIntrinsicRequirements,
  UnsupportedSpecializations,
  EmptyFunctionLinkName,
  UnsupportedFunctionMetadata,
  UnsupportedFunctionParameters,
  UnsupportedVariadicFunction,
  UnsupportedReturnType,
  DeclarationHasBody,
  DefinitionHasNoBlocks,
  UnsupportedStackObjects,
  UnsupportedAllocaInstructions,
  EmptyBlockLabel,
  DuplicateBlockLabel,
  DuplicateBlockId,
  InvalidEntryBlock,
  UnsupportedEntryBlock,
  UnsupportedOrdinaryInstruction,
  UnsupportedInlineAsmShape,
  UnsupportedInlineAsmMetadata,
  InvalidVoidReturn,
  MissingBranchTarget,
  UnsupportedTerminator,
  BuilderFailure,
  PublicationFailure,
};

struct ImportError {
  ImportErrorCode code = ImportErrorCode::BuilderFailure;
  std::string function;
  std::string block;
  std::string detail;
  std::optional<BuildError> build_error;
  std::optional<PublishError> publish_error;
  std::vector<VerificationError> verification_errors;
};

Result<RawBir, ImportError> lower_lir_to_raw_bir(
    const codegen::lir::LirModule& module, ImportOptions options = {});
Result<CanonicalBir, ImportError> lower_lir_to_canonical_bir(
    const codegen::lir::LirModule& module, ImportOptions options = {});

}  // namespace c4c::backend::bir
