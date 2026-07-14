#pragma once

// Result carrier shared by fallible BIR APIs.
#include "core/result.hpp"

namespace c4c::backend::bir {

template <class T, class E>
class Result;

}  // namespace c4c::backend::bir

// Stable identities used to refer to BIR entities without pointer identity.
#include "core/ids.hpp"

namespace c4c::backend::bir {

struct FunctionId;
struct BlockId;
struct InstId;
struct ValueId;

}  // namespace c4c::backend::bir

// Exact whole-stage pipeline revision and fingerprint identities.
#include "pipeline/identity.hpp"

namespace c4c::backend::bir {

struct ModuleRevision;
struct FunctionRevision;
struct Fingerprint128;
struct FunctionRevisionDigest;
struct PipelineStageStamp;

}  // namespace c4c::backend::bir

// Deterministic invocation cancellation and work-unit budget control.
#include "passes/execution_control.hpp"

namespace c4c::backend::bir {

struct ResourceBudget;
enum class ExecutionControlFailure : std::uint8_t;
class CancellationToken;
class CancellationSource;

}  // namespace c4c::backend::bir

// Semantic schema: types, values, control-flow terminators, and signatures.
#include "core/type.hpp"
#include "core/ir.hpp"

namespace c4c::backend::bir {

struct Type;
struct ValueDef;
struct InlineAsmNode;
struct JumpTerm;
struct CondJumpTerm;
struct IndirectJumpTerm;
struct ReturnTerm;
struct UnreachableTerm;
struct FunctionSignature;

}  // namespace c4c::backend::bir

// Read-only traversal and lookup surfaces for published BIR.
#include "core/view.hpp"

namespace c4c::backend::bir {

class ModuleView;
class FunctionView;
class BlockView;
class InstView;

}  // namespace c4c::backend::bir

// Scoped construction surfaces and the verified raw-BIR result.
#include "core/builder.hpp"

namespace c4c::backend::bir {

class RawBir;
class CanonicalBir;
class ModuleBuilder;
class FunctionBuilder;
struct BuildResult;
struct InlineAsmSpec;

}  // namespace c4c::backend::bir

// Verified LIR-to-RawBir import boundary and structured diagnostics.
#include "lir_to_bir.hpp"

namespace c4c::backend::bir {

struct ImportOptions;
enum class ImportErrorCode;
struct ImportError;

}  // namespace c4c::backend::bir

// Foundation verification diagnostics and entry point.
#include "verify/verifier.hpp"

namespace c4c::backend::bir {

struct VerificationError;
struct VerificationResult;
class FoundationVerifier;

}  // namespace c4c::backend::bir
