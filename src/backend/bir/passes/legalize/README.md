# BIR Legalize Pass Contract

Status: first-pass design under review. This document specifies the intended
contract; the checked-in bootstrap BIR implementation does not yet implement
this pass or the complete core schema assumed below.

`legalize` is the first mutating pass after verified `RawBir`. It converts the
closed set of *well-formed, typed, lossless* Raw forms into the portable forms
accepted by scalar canonicalization. It is not an error-recovery pass and is
not a target instruction legalizer.

```text
verified RawBir
  -> legalize (this contract)
  -> Legalized checkpoint
  -> scalar
  -> cfg -> ssa -> memory -> aggregate -> intrinsics
  -> Canonical verification and CanonicalBir publication
```

The word “legal” in this document means legal for the target-independent BIR
pipeline. It never means “has a native machine instruction” or “has been
assigned an ABI carrier.”

## 1. Authority boundary

### 1.1 Preconditions inherited from Raw publication

`legalize` receives a `RawBir` that already passed the complete Raw verifier.
It may therefore rely on all of the following without defensive repair:

- module/function ownership, epochs, generations, stable IDs, order vectors,
  reservations, and forward references are valid;
- every type, constant, symbol, global, local, block, instruction, value, and
  terminator reference resolves to the correct owner and kind;
- every instruction has the payload, operands, results, roles, and types
  required by its closed opcode descriptor;
- function signatures, calls, call effects, argument attributes, operand
  bundles, and zero-or-one semantic result shape are complete;
- inline assembly has structured operands, types, constraints, clobbers, and
  asm-goto block IDs; it is never text-only authority;
- blocks have one typed terminator, CFG targets are valid, phi entries use
  exact `EdgeKey`s, and def-use is reciprocal and complete;
- memory accesses carry their semantic access type, alignment, volatility,
  address space, and atomic semantics;
- exact integer/floating bits, aggregate topology, relocation targets, and
  object layout are present rather than reconstructed from text;
- no ABI placement, physical register, frame, helper selection, target opcode,
  encoding, route table, or prepared side-table authority exists in BIR.

Consequently this pass **must not** repair malformed IDs, unresolved names,
missing operands, missing result types, incomplete calls, missing call effects,
missing inline-asm operands, absent atomic ordering, text-only initializers, or
any producer gap listed by the importer. Those are publication/import errors.
Accepting one here would weaken the Raw trust boundary.

### 1.2 This pass owns

- canonical encoding of exact scalar constants without changing their value;
- normalization of representable raw scalar opcode aliases into one semantic
  opcode/predicate/cast vocabulary;
- explicit normalization of non-`I1` truth values at boolean-use boundaries;
- normalization of arbitrary-width *SSA scalar integer operations* into the
  width-explicit `WideIntegerOp` family without choosing a machine carrier;
- classification of wide integer, extended-float, complex, and checked-
  overflow semantic operations into a typed portable semantic form consumed by
  `scalar`; it does not choose their implementation;
- target-independent validation and normalization of structured inline-asm
  constraint grammar (roles, ties, alternatives, modifiers, and symbolic
  references), while retaining target tokens losslessly;
- rejection of a typed Raw form that has no semantics-preserving portable BIR
  representation and no registered next-pass owner.

### 1.3 This pass does not own

- constant folding, algebraic simplification, compare/select folding, or
  canonical expression trees (`scalar`);
- switch lowering, critical-edge splitting, reachability, block order, or
  compare/branch fusion (`cfg` and later target selection);
- phi insertion, phi placement/order, memory-to-register promotion, or
  out-of-SSA (`ssa` and later MIR construction);
- GEP flattening, provenance, alias ranges, load/store widening, bitfield
  extraction/insertion, or memory intrinsic expansion (`memory`);
- aggregate decomposition, by-value copy expansion, hidden result lowering, or
  physical complex lanes (`aggregate`, preparation, and MIR);
- intrinsic selection or expansion (`intrinsics`), except checking that the
  typed registry entry has a named downstream owner;
- ABI classification, argument/result registers, stack arguments, sret
  placement, HFA/HVA decisions, variadic save areas, or call moves
  (`preparation/abi`, `preparation/calls`, and MIR);
- runtime-helper symbol/signature selection (`preparation/runtime_helpers`);
- target constraint satisfaction, fixed-register availability, register
  allocation, frame layout, instruction selection, relocation encoding, or
  emission.

## 2. Exact Raw-only inventory and first owner

The accepted Raw documents enumerate the following forms. Every row has one
first owner; “preserve” means this pass traverses it but must not opportunistically
rewrite it.

| Raw-only form | First owner | `legalize` disposition | Required handoff |
|---|---|---|---|
| arbitrary integer widths | `legalize` | preserve exact typed constants and boundaries; map non-canonical scalar operations to width-explicit `WideIntegerOp` as specified in section 5 | no arbitrary-width ordinary scalar alias reaches `scalar`; the explicit wide semantic family does |
| exact FP literals | `legalize` | canonicalize payload byte/bit encoding only; never round or change format | exact `FloatFormat` and bits reach `scalar` |
| old/new duplicate LIR shapes | importer | impossible in Raw; reject any compatibility alternative | one BIR opcode only |
| typed non-canonical operator | `legalize`, then `scalar` | map aliases to closed semantic `UnaryOp`/`BinaryOp`/raw semantic op | `scalar` owns algebraic form |
| typed non-canonical comparison | `legalize`, then `scalar` | map predicate spelling/domain to closed `CompareOp`; retain operand order unless the mapping requires a documented swap | `scalar` owns preferred predicate/order |
| typed non-canonical cast | `legalize`, then `scalar` | map source conversion kind to one explicit `CastOp`; never infer signedness from type spelling | `scalar` owns redundant-cast folding |
| phi forward references | importer | preserve IDs, values, and `EdgeKey`s | `ssa` validates/canonicalizes phi placement |
| raw switch/indirect target set | `cfg` | preserve selector and every ordered successor slot | `cfg` owns normalization |
| multi-index GEP/address-space-rich access | `memory` | preserve full typed path and address space | `memory` is the single first normalizer |
| aggregate insert/extract, byval/sret markers | `aggregate` | preserve aggregate identity, paths, and semantic attributes | `aggregate` owns decomposition; ABI owns placement |
| semantic/feature intrinsic | `intrinsics` | validate registered downstream disposition only; preserve ID/types/immediates/effects | `intrinsics` owns canonical registry form/expansion |
| structured raw inline-asm constraints | `legalize` | normalize target-independent grammar; preserve target token spelling and semantic requirements | preparation validates target satisfaction |
| `WideIntegerOp` | `legalize` | classify operation and normalize width operands; keep a semantic wide operation | `scalar` decides target-independent expansion shape |
| `ExtendedFloatOp` | `legalize` | resolve exact format and semantic operation; retain exact format | `scalar`, then runtime-helper preparation if needed |
| `ComplexOp` | `legalize` | normalize component format and operation/result contract; retain one semantic aggregate result | `scalar`/`aggregate`; never physical return lanes |
| `CheckedOverflowOp` | `legalize` | normalize arithmetic kind and signedness; retain explicit value+overflow results | `scalar` owns canonical expansion/folding |

Raw is already typed and text-free. No untyped opcode, opaque “unsupported”
node, raw call spelling, raw label spelling, `args_str`, or `init_text` is an
additional Raw-only form.

## 3. Proposed C++ API

The pass uses, and does not redeclare, the framework API in
[`../README.md`](../README.md). In particular, `PreservedAnalyses`,
`PassResult`, `PassFailure`, `ModulePassSession`, `MutationSummary`, revision
accounting, commit/rollback, diagnostics, and statistics are framework-owned.
The pass reports intent; the executor derives the authoritative mutation and
committed result. Legalize never mints a stage token or a private checkpoint.

```cpp
namespace c4c::backend::bir::passes {

enum class LegalizeClass : std::uint8_t {
  AlreadyLegal,
  NormalizeInBir,
  PreserveForScalar,
  PreserveForCfg,
  PreserveForSsa,
  PreserveForMemory,
  PreserveForAggregate,
  PreserveForIntrinsics,
  DeferToPreparation,
  MissingDownstreamDisposition,
  UnsupportedPortableSemantics,
};

enum class SemanticDisposition : std::uint8_t {
  CanonicalSemanticOp,
  ExpandInScalar,
  PreserveForAggregate,
  PreserveForIntrinsics,
  PreserveForPreparedInput,
  MissingDownstreamDisposition,
  UnsupportedPortableSemantics,
};

class LegalizeSemanticRegistry {
public:
  SemanticDisposition disposition(RuntimeSemanticOp, TypeId) const;
  bool has_downstream_owner(IntrinsicId,
                            std::span<const TypeId>) const;
};

class LegalizePass final : public ModulePass {
public:
  explicit LegalizePass(const LegalizeSemanticRegistry& registry);
  PassId id() const noexcept override { return PassId::Legalize; }
  Result<PassResult, PassFailure>
  run(ModulePassSession& session) const override;

private:
  const LegalizeSemanticRegistry& registry_; // immutable, frozen before use
};

} // namespace c4c::backend::bir::passes
```

The registered descriptor is authoritative and has this shape:

```cpp
PassDescriptor{
  .id = PassId::Legalize,
  .kind = PassKind::Module,
  .requires = {PassProperty::RawVerified},
  .establishes = {PassProperty::TypesLegal},
  .repeat = RepeatContract::Idempotent,
  .accepts_declarations = true,
  // Exact mutation effects and preservable analyses are closed registry sets.
};
```

`TypesLegal` is granted by the executor only after the pass-contract checker
and configured verifier gate succeed. It is the immediate successor
precondition; it is not a public stage object. The current pass framework
explicitly leaves a module transaction that edits both module entities and
function bodies as a core API gap. Legalize requires that one atomic
`ModuleEditor` capability before implementation; it must not simulate it by
committing per-function edits or by inventing `ModulePassTransaction`.

Declarations and unreachable blocks are inventoried. Declarations normally
produce no body edits, but their exact signatures and attributes participate
in call/type-disposition checks. Unreachable definitions receive the same
normalization as reachable definitions: reachability cannot make a malformed
or unowned Raw form disappear. Permitted mutation effects are limited to type/
constant interning, instruction payload replacement, instruction insertion or
erasure, operand replacement, def-use updates, and debug-origin composition.
Symbol identity, declarations, signatures, globals' object layouts, CFG edges,
block order, locals, call semantic shape, memory semantics, and stage-excluded
authority are not permitted effects.

The registry contains only target-independent semantic alias mappings and the
identity of the required later owner. It cannot answer whether a target has a
native operation, register class, ABI placement, runtime helper, instruction,
or inline-asm constraint implementation. Target support and helper selection
are validated at `PreparedInput` and later target gates.

Boolean normalization, exact constant canonicalization, opcode/predicate/cast
normalization, and structured inline-asm grammar validation are mandatory
parts of `PassId::Legalize`; no correctness option can disable them. Diagnostic
and work limits come exclusively from `session.context().budget()`.
Observational counters go only through
`session.context().statistics()` using the framework's closed
`PassStatisticId` registry. Core-derived instruction/operand mutation counts
remain authoritative; this pass charges classification and traversal through
`PassStatisticId::WorkUnits` and defines no independent stats object.

## 4. Type and constant normal form

### 4.1 Type roles

Legalization distinguishes *semantic storage types* from *SSA computation
types*. It must not repeat the legacy mistake of changing an object's type and
then rewriting its byte size/alignment to match a convenient register carrier.

| Type occurrence | Legalize action |
|---|---|
| function parameter/result semantic type | preserve; language/ABI-visible type is not widened here |
| global/local/record/vector/array element type and `ObjectLayout` | preserve exactly |
| load/store `MemoryAccess::access_type` | preserve exactly; memory pass owns access normalization |
| GEP source/index types and address space | preserve exactly |
| call callee type, argument/result semantic type, attributes | preserve exactly |
| SSA-only integer computation/result | preserve exact type; select the ordinary or explicit wide semantic opcode family as section 5 specifies |
| boolean condition role | require `I1`; insert explicit compare-to-zero when Raw contains a permitted non-`I1` truth value |
| exact float type | preserve `FloatFormat`; never alias x87 extended-80 to binary64 or binary128 |
| aggregate/vector/function/pointer identity | preserve `TypeId`; no flattening or pointer-to-integer substitution |

The ordinary scalar-op descriptor widths are initially `{1,8,16,32,64,128}`.
Other widths remain first-class semantic `TypeId`s and use the width-explicit
`WideIntegerOp` family. This is an opcode-schema partition, not a list of
native target registers and not permission to widen objects, signatures, or
SSA values. Changing the partition requires corresponding core, verifier,
scalar, constant, memory, and target coverage.

`I1` remains the semantic boolean type. The legacy `NoTargetFacingI1` invariant
is deliberately **not** a legalize postcondition. Turning every `I1` into `I32`
changes function signatures, memory widths, phi types, call contracts, and
object layouts. Target register widening is a preparation/MIR decision.

### 4.2 Integer constants

For `IntegerBits{little_endian_bits}` of declared width `N`:

1. verify (do not repair) that the Raw verifier accepted the declared width and
   payload cardinality;
2. clear unused high bits in the final payload byte only if the core constant
   contract defines them as non-semantic padding; otherwise reject a mismatch;
3. intern the canonical exact-bit payload for the same semantic type;
4. replace constant references transactionally; preserve `Undef` and `Poison`
   as distinct alternatives and never convert either to zero;
5. preserve the exact integer type at every use and definition; conversion to
   another width exists only when the source operation itself is an explicit
   typed cast.

`Zero` may be canonicalized to the core's one typed zero representation. `Null`
remains pointer-only and is not interchangeable with integer zero. Symbol,
function, TLS, and block addresses remain typed address constants; they are not
folded to integers even when pointer width is known.

### 4.3 Floating constants

`FloatBits` is canonicalized by `(FloatFormat, exact semantic bits)`. The pass:

- preserves NaN sign, payload, quiet/signaling state, infinities, signed zero,
  subnormals, and exact finite values;
- separates semantic bits from storage padding for `X87Extended80` using the
  module's `FloatLayout` padding policy;
- preserves all 128 bits of `IeeeBinary128`;
- never converts through host `float`, `double`, or `long double`;
- never treats x86 long double as binary64 and never treats every legacy
  `F128` spelling as IEEE binary128;
- reports `UnsupportedPortableSemantics` only when BIR cannot preserve the
  operation's exact semantics, or `MissingDownstreamDisposition` when no
  target-independent later owner is registered. Native target support is not
  queried here. Constant payload support alone does not prove operation
  coverage, but target support is a `PreparedInput` concern.

Aggregate constants recurse in declared element order. Their topology and
layout remain aggregate authority; this pass only canonicalizes scalar leaves.
Relocation fragments, label differences, raw bytes, and offsets are preserved.

## 5. Scalar width and boolean transformations

### 5.1 Arbitrary-width SSA integers

Let `N` be a well-formed integer width outside the ordinary scalar-op
descriptor set. Legalize preserves `iN` operands, results, constants,
parameters, memory accesses, aggregate members, calls, phis, and returns
exactly. It maps each accepted raw operation to a registered `WideIntegerOp`
whose payload carries the operation, `N`, and every semantic discriminator
needed by that operation:

- signedness for ordered compare, division, remainder, conversion, and
  arithmetic shift;
- modulo-`2^N`, checked-overflow, or trapping/undefined behavior as applicable;
- shift-count domain and poison/undef propagation;
- exact result roles for checked operations.

No carrier type is chosen here. In particular, legalize does not round `i9` to
`i16`, split `i128` into lanes, choose a limb width, or insert extension/mask/
truncation sequences. Such a sequence is not target-independent unless its
full equivalence and downstream ownership have already been designed, and its
casts would themselves keep the arbitrary-width boundary alive. `scalar` may
expand a registered wide semantic operation into exact BIR when there is a
portable, width-parametric rule; otherwise the semantic node survives to a
registered later lowering/runtime owner.

If an operation on `iN` has neither an exact `WideIntegerOp` descriptor nor a
named later disposition, legalization fails without mutation. Storage,
signature, object layout, access width, alignment, address range, ABI
placement, and physical return lanes are never changed as a workaround.

### 5.2 Boolean normal form

Canonical boolean values are `i1` with bits `{0,1}`. The following table is
exhaustive:

| Raw source/use | Normal form |
|---|---|
| `Compare` result | already `i1`; preserve result identity when only predicate metadata changes |
| `LogicalNot x` | `Compare Eq x, zero(type(x))`, yielding `i1`; `scalar` may later simplify |
| integer truth use | `Compare Ne x, zero(type(x))` |
| pointer truth use | `Compare Ne x, Null(pointer-type)` |
| floating truth use | language-defined ordered/unordered nonzero comparison encoded by the closed predicate; NaN behavior must match C truth semantics |
| constant used as condition | typed comparison or canonical `i1` constant; no target-facing `i32` boolean convention |
| select/conditional branch condition | exactly `i1` after an inserted compare when required |
| stored/passed/returned `_Bool` | semantic `i1` boundary retained; later memory/ABI stages choose carrier width |

An existing `i1` constant is canonicalized to exactly zero or one bit. A Raw
integer constant of another type is not silently retagged as `i1`; conversion
is explicit. `Undef`/`Poison` boolean semantics are preserved and are not
normalized by observing a concrete value.

### 5.3 Stable IDs during width/boolean rewriting

- Metadata-only normalization of an instruction preserves its `InstId`, result
  `ValueId`s, block membership, order, debug location, and origin.
- If opcode/result arity/types are descriptor-compatible, edit the instruction
  in place through the transaction.
- If result type or arity must change for another legalize-owned normalization,
  create replacement instructions at the
  same semantic position. Old result IDs are not retagged. Use typed atomic
  RAUW for every mapped result, including phi and terminator users, then erase
  the old instruction only after its use lists are empty.
- Inserted boolean-conversion instructions receive new stable IDs in
  deterministic operand/result-role order and
  `OriginKind::SynthesizedByPass` pointing to the source origin.
- Function parameters, global constants, and signature types are never mutated
  merely to preserve an old result ID.

## 6. Opcode, comparison, and cast normalization

### 6.1 Unary and binary operators

| Raw meaning | Legalized representation | Notes |
|---|---|---|
| integer negate | `Unary{Neg}` | width restoration as required |
| floating negate | `Unary{FNeg}` | exact format retained |
| bitwise not | `Unary{BitNot}` | integer/vector only |
| logical not | typed compare-to-zero | result is `i1` |
| integer add/sub/mul | `Binary{Add/Sub/Mul}` | signedness-independent modulo width unless checked form |
| signed/unsigned div/rem | `SDiv/UDiv/SRem/URem` | signedness explicit in opcode |
| shifts | `Shl/LShr/AShr` | shift-count policy remains explicit; no target masking assumption |
| integer bit operations | `And/Or/Xor` | vector legality remains descriptor-driven |
| FP arithmetic | `FAdd/FSub/FMul/FDiv/FRem` | format and exceptional semantics retained |
| wide integer semantic op | `WideIntegerOp` with normalized operation/width | helper or limb expansion is later |
| extended FP semantic op | `ExtendedFloatOp` with exact format | helper/native choice is later |
| complex arithmetic | `ComplexOp` with component format | no hidden physical lanes |
| checked arithmetic | `CheckedOverflowOp` with signedness and explicit results | no loss of overflow result |

Aliases such as textual `add`, source enum duplicates, or a binary opcode used
as compare authority do not survive. The importer should normally remove them;
if the Raw schema deliberately includes a typed migration alias, this table
must enumerate it before it is accepted.

### 6.2 Comparisons

The legalized predicate set is exactly:

```text
Eq Ne
Ult Ule Ugt Uge
Slt Sle Sgt Sge
FOeq FOne FOlt FOle FOgt FOge FOrd FUno
```

Integer ordered predicates retain explicit signedness. Floating predicates
retain ordered/unordered NaN behavior. Pointer comparisons permit only the
relations defined by the core/language contract; they are never normalized by
casting pointers to integers. A target's flag condition codes are not BIR
predicates.

This pass may swap operands only as part of a declared alias mapping, for
example source `greater-than(a,b)` to canonical `less-than(b,a)`. It records the
swap in `MutationSummary`, updates use lists atomically, and does not fuse the
comparison with a branch. `scalar` later chooses preferred orientations and
folds constants.

### 6.3 Casts

Every cast becomes one of:

```text
Trunc ZExt SExt
FPTrunc FPExt
FPToUI FPToSI UIToFP SIToFP
PtrToInt IntToPtr Bitcast
```

Transformation rules:

| Source relation | Required `CastOp` |
|---|---|
| wider integer -> narrower integer | `Trunc` |
| narrower integer -> wider, unsigned semantics | `ZExt` |
| narrower integer -> wider, signed semantics | `SExt` |
| wider FP format -> narrower FP format | `FPTrunc` |
| narrower FP format -> wider FP format | `FPExt` |
| FP -> unsigned/signed integer | `FPToUI` / `FPToSI` |
| unsigned/signed integer -> FP | `UIToFP` / `SIToFP` |
| pointer -> integer of explicit width | `PtrToInt` |
| integer -> pointer in explicit address space | `IntToPtr` |
| representation-preserving same-size conversion | `Bitcast` only when the core descriptor permits the pair |

Same-type casts may remain until `scalar` removes them; legalize may remove one
only through typed RAUW when poison/undef and debug policies make removal
observably safe. Pointer width comes from `SemanticDataLayout`, but pointer
casts do not authorize pointer arithmetic, provenance inference, or ABI
normalization. Address spaces are never dropped or changed by a generic
bitcast.

Extended-float conversions retain exact source/destination formats. The
reference compiler's approximation of x86 `F128` as `F64` is explicitly
rejected: new BIR distinguishes `X87Extended80` and `IeeeBinary128`, so a
conversion cannot be classified as a no-op merely because legacy code used one
spelling for both.

## 7. Non-scalar family disposition

### 7.1 Calls and function boundaries

Legalize verifies its inherited assumptions and otherwise preserves:

- direct `SymbolId` or indirect callee operand and exact function type;
- source calling convention, fixed/variadic boundary, ordered arguments,
  argument/return attributes, tail request, call effects, and bundles;
- zero results for void and one semantic result for every non-void call,
  including aggregates and complex values.

It does not add `CallArgAbiInfo`, `CallResultAbiInfo`, result lanes, hidden
storage, sret selection, register/stack classes, or call moves. A semantic
extension attribute may remain on the call/signature; realization is ABI
preparation. Runtime-capable scalar calls are not replaced with helper calls
here.

### 7.2 Memory and address operations

Legalize preserves every `MemoryAccess` field:

| Field | Owner/disposition |
|---|---|
| `access_type` | semantic authority; preserve |
| `align_bytes` | Raw verifier already checked; preserve |
| `address_space` | preserve; memory/preparation consume it |
| `is_volatile` | preserve exactly |
| atomic operation/order/result mode | preserve; never weaken to non-atomic |
| address operand | preserve typed value and provenance seed |
| transfer size/count | preserve dynamic or constant operand |
| GEP source element/index path | preserve for `memory` |

Carrier-width legalization occurs around the value edge, not by changing load,
store, object, or address size. This rejects the legacy behavior that changed
`align_bytes`, `MemoryAddress::size_bytes`, or
`MemoryAddress::align_bytes` when promoting `I1`.

### 7.3 Aggregates and vectors

`ExtractValue`, `InsertValue`, `BuildAggregate`, vector element operations,
shuffle masks, aggregate constants, record fields, `by_value`, and semantic
structure-return attributes retain their typed topology. Legalize may
canonicalize scalar leaves but cannot flatten fields, create byte slots, split
complex results, infer HFA lanes, or convert an aggregate to a pointer.

### 7.4 Intrinsics and runtime-capable operations

For each `IntrinsicPayload`, legalize requires a registry entry whose signature,
immediates, effects, feature class, and target-independent downstream owner
agree with the already verified instruction. It preserves the semantic ID,
types, immediates, and effects unchanged. It does not write a disposition into
BIR and does not ask whether the selected target has a native instruction or a
runtime fallback. Memory, stack, variadic, bit, FP, SIMD/vector, overflow,
classification, CRC/crypto, barrier/cache, and hint families proceed when the
`intrinsics` pass (or another explicit target-independent owner) is registered.
Missing ownership is `MissingDownstreamDisposition`; target support and helper
availability are checked at `PreparedInput`.

No helper spelling, selected target intrinsic, machine opcode, or call ABI is
written into BIR.

### 7.5 Inline assembly

Legalize may normalize only target-independent constraint grammar:

- ordered input/output/read-write/tied/immediate/memory/address/label roles;
- alternative boundaries, symbolic names, tie indices, early-clobber and
  commutative modifiers;
- canonical representation of `memory` and condition-code clobbers;
- exact association between asm-goto label operands and terminator target
  slots;
- lossless retention of raw target constraint/register/modifier spelling as a
  target token attached to the structured node.

It does not decide whether an architecture accepts a token, assign a register,
rewrite the template, parse an instruction encoding, or consume `insn_r` as
opcode authority. Missing operands/types/ties are importer failures, not
legalization opportunities.

## 8. Required mutation order

The implementation performs one deterministic module transaction:

1. capture input epoch/revision and run `verify(Raw)` in debug/CI policy;
2. inventory all Raw-only forms in module semantic order without mutation;
3. classify every item and accumulate independent diagnostics; if any item is
   unsupported or lacks a named owner, abort before mutation;
4. intern required canonical constants in stable
   `(kind,width/format,bits)` order, never hash iteration order;
5. normalize module scalar constant leaves and build an old-to-new constant map;
6. process functions in declared order, blocks in semantic order, instructions
   in order, and operand/result roles in descriptor order;
7. apply metadata-only opcode/predicate/cast/asm grammar edits;
8. insert boolean conversions, perform typed RAUW where an instruction is
   replaced, and erase replaced nodes after all uses move;
9. update eager def-use and function/module revisions through editors;
10. run the pass-contract postcondition checker below and the configured Raw
    verifier gate against the candidate revision;
11. return `PassResult`; the framework executor compares intent with the
    derived mutation, validates preservation, and either commits once or rolls
    back once. On success it establishes `PassProperty::TypesLegal`; the pass
    itself mints no checkpoint.

Module-level type/constant changes and function rewrites commit together. A
failure in function N cannot leave earlier functions legalized.

## 9. Postconditions and handoff to `scalar`

The pre-pass gate is the complete `VerifyProfile::Raw`, not a legalize-specific
subset. In particular, it must be green for the following rule families before
classification starts:

| Raw verifier family | Rules relied upon directly |
|---|---|
| identity/storage/order | `ModuleEpochInvalid` through `ActiveEditAtFreeze` |
| types/constants/layout | `DataLayoutInvalid` through `UndefPoisonRoleInvalid` |
| function/CFG/terminators | `FunctionShapeInvalid` through `UnreachableBlock` (an allowed unreachable warning is not an error) |
| descriptors/types/def-use/SSA identity | `OpcodeInvalid` through `CrossComponentUse` |
| calls/variadics | `CallCalleeInvalid` through `VarArgInvalid` |
| memory/atomics | `MemoryAccessInvalid` through `FenceInvalid` |
| aggregates/intrinsics/asm | `AggregatePathInvalid` through `InlineAsmEffectInvalid` |
| stage boundary | `ForbiddenStageFact`, `ForbiddenCompatibilityPayload` |

These are pass-contract diagnostics in the `LegalizeRule` registry from
section 11. They are deliberately **not** `VerifyRule` values:

```cpp
enum class LegalizePostcondition : std::uint16_t {
  RawFormRemaining,
  WideIntegerDispositionInvalid,
  BooleanFormInvalid,
  ScalarOpcodeInvalid,
  FloatEncodingInvalid,
  RuntimeDispositionAbsent,
  IntrinsicOwnerAbsent,
  AsmGrammarInvalid,
  SemanticDrift,
  ForbiddenAuthority,
};
```

The sole `VerifyRule` registry owner is
[`../../verify/README.md`](../../verify/README.md), whose current complete
schema revision ends at `ForbiddenCompatibilityPayload = 0x0904`. The pass
framework explicitly forbids a parallel verifier-rule registry. If a future
canonical verifier revision promotes any invariant below into `VerifyRule`, it
must allocate the ID and semantics there first; this document then references
that authoritative ID instead of reserving `0x0Axx` locally.

The exact post-legalize pass-contract checks are:

1. no importer compatibility opcode/payload exists;
2. every arbitrary-width scalar computation uses a registered typed
   `WideIntegerOp`; exact SSA, storage, and signature types remain unchanged;
3. every boolean-use operand is `i1`; every concrete `i1` constant is 0 or 1;
4. every scalar operator, comparison, and cast uses the closed canonical enum;
5. every floating type/constant names an exact `FloatFormat` and exact bits;
6. every wide/extended/complex/checked operation has a registered
   target-independent downstream owner; no target/helper availability fact is
   required or recorded;
7. every intrinsic has a registered target-independent `intrinsics` owner;
8. every inline-asm constraint is structurally normalized and retains every
   target token losslessly;
9. CFG successor slots, block order, phi `EdgeKey`s, call semantic shape,
   memory access semantics, aggregate topology, and symbol identity are
   unchanged except for explicit value bridges;
10. no ABI, allocation, frame, target opcode, helper name, or prepared fact was
    added;
11. full IDs, descriptors, types, def-use, dominance, and stage-boundary rules
    remain valid at the committed revision.

`scalar` may rely on these postconditions and therefore need not understand
raw aliases, non-`i1` conditions, host-form FP literals, or unclassified
wide/extended semantic nodes. It must still preserve width, poison/undef,
overflow, FP exception/NaN, pointer, aggregate, and call semantics.

## 10. Analyses, revisions, and preservation

The pass requires only core descriptor traversal, eager def-use, type/constant
interning, and the frozen semantic-owner registry. It must not request CFG,
dominance, liveness, provenance, ABI, or target-register analysis to decide its
transformations.

Preservation rules:

| Analysis/fact | Result |
|---|---|
| module symbol/link-name index | preserved if constant/type interning does not alter symbols |
| CFG successor slots and block order | preserved exactly |
| reachability/dominance | logically unchanged by non-CFG edits, but revision-keyed caches are invalidated unless the analysis manager supports mutation-summary validation |
| def-use | eagerly updated core authority, then independently verified |
| use-derived comparison/select/call analyses | invalidated for each changed function |
| memory effects/provenance/liveness | invalidated for each changed function |
| debug/source attachments | preserved on original nodes; composed on synthesized nodes according to explicit policy |
| ABI/frame/regalloc analyses | cannot exist at this stage |

Every changed function receives a new revision; module-level constant/type
interning increments the module revision. Unchanged functions may retain their
revision only if the core revision model supports independent function
revisions and no referenced module entity changed meaning. A stale cache is
never declared preserved merely because the CFG did not change.

## 11. Transaction, failure, and diagnostics

### 11.1 Failure atomicity

Classification is separated from mutation so common unsupported cases fail
without edits. All later failures—including allocation failure, editor error,
RAUW type error, verifier error, revision race, or diagnostic limit—roll back
types, constants, instructions, uses, order, origins, and revisions as one
transaction. There is no best-effort per-function output.

### 11.2 Diagnostic model

```cpp
enum class LegalizeRule : std::uint16_t {
  RawPreconditionFailed,
  UnownedRawForm,
  IntegerWidthNotRepresentable,
  IntegerWidthBridgeInvalid,
  BooleanBoundaryInvalid,
  FloatFormatUnrepresentable,
  FloatPayloadNotCanonical,
  OperatorAliasUnknown,
  CompareAliasUnknown,
  CastRelationUnsupported,
  RuntimeSemanticDispositionMissing,
  IntrinsicDispositionMissing,
  InlineAsmGrammarInvalid,
  UnsupportedPortableSemantics,
  MissingDownstreamDisposition,
  ForbiddenAbiAuthority,
  ForbiddenTargetAuthority,
  RewriteDefUseMismatch,
  PostconditionFailed,
  RevisionChanged,
};
```

Each diagnostic names rule, module/function/block/instruction/value ID as
applicable, operand/result role, source origin, semantic type/width/format,
requested disposition, and deterministic remediation owner. Diagnostics are
ordered by pass phase and semantic module order. They do not use pointer
addresses, hash order, rendered instruction parsing, or legacy names as
identity.

`UnsupportedPortableSemantics` and `MissingDownstreamDisposition` are hard pass
failures with the original semantic node intact. They are never converted to
`Unreachable`, zero, an ordinary call, a generic intrinsic, or a no-op. Lack of
native target support is not a legalize failure and is not queried here.

## 12. Idempotence, fixed point, and determinism

One successful run reaches a fixed point. Re-running on the resulting revision
with the same frozen semantic registry version must:

- insert no types, constants, instructions, or origins;
- perform no RAUW or order changes;
- produce an empty `MutationSummary` and unchanged semantic dump;
- emit the same non-mutating notes (or no notes by policy);
- leave function/module revisions unchanged.

The registry is frozen before modules are accepted. A different semantic
registry version requires a new pipeline fingerprint and restart from an
appropriate Raw snapshot rather than reclassifying already-mutated BIR.

Deterministic choices use semantic order and stable IDs. Constant interning
keys use exact type/format/bits. No host FP behavior, unordered-map iteration,
target register availability, target support query, or helper-name ordering
influences output.

## 13. Legacy `prealloc/legalize.cpp` field-by-field disposition

This is the required quarantine map. “Reject” means the behavior must not be
ported into canonical BIR, not that the compiler feature is discarded.

| Legacy anchor/field behavior | Classification | New owner/reason |
|---|---|---|
| `should_promote_i1(TargetProfile::arch)` | reject legacy authority | `I1` remains semantic bool; target carrier widening is preparation/MIR |
| `legalize_type(I1 -> I32)` | reject | global retagging changes semantics and layout |
| `legalize_value` retags every value | reject | use explicit typed bridges only |
| immediate bool normalization to 0/1 | adopt narrowly | canonicalize only values already semantically boolean or explicit truth conversion |
| `type_size_bytes` scalar table | already core/layout | `TypeData` + `SemanticDataLayout`; no pass-local size authority |
| `legalize_sized_type` fills/changes size and alignment | reject repair | Raw verifier requires coherent layout; legalize cannot invent or widen object layout |
| `legalize_call_arg_abi` / `legalize_call_result_abi` | defer | preparation/ABI owns classification and carrier normalization |
| `scalar_i16_call_arg_abi_needs_repair` | reject | ABI agreement is not canonical BIR state |
| `direct_bir_call_arg_abi_repair` | defer | preparation/ABI, using full target ABI and aggregate rules |
| `direct_bir_function_return_abi_repair` | defer | preparation/ABI classifies a function's semantic return without mutating its signature |
| `direct_bir_call_result_abi_repair` | defer | preparation/ABI classifies the call result from the full calling convention |
| F32/F64/F128 `Sse` vs `Integer` selection | defer | target ABI, not semantic legalize |
| x86-64 F128 passed in memory | defer | target ABI; exact float format remains BIR |
| I128 passed on stack | defer | target ABI; semantic I128 remains intact |
| `passed_in_register`, `passed_on_stack` | reject in BIR | prepared call plan/MIR only |
| `byval_copy`, `sret_pointer` placement flags | split | semantic attributes remain core; copy/storage placement is ABI/calls/aggregate preparation |
| `returned_in_memory` | reject in BIR | prepared return plan only |
| `legalize_memory_access_metadata` rewrites alignment/size | reject | preserve semantic access; memory pass and target lowering decide legal implementation |
| `find_compare_for_condition` name scan | replace | core def-use/comparison analysis; no spelling lookup |
| `intern_prepared_block_label` spelling fallback | reject | typed `BlockId`; names are debug only |
| `names.function_names.intern(function.name)` | reject as semantic lookup | `FunctionId`/`SymbolId` are authority; a later printer may build display-name tables |
| `make_branch_condition` | defer | CFG/comparison analysis plus target instruction selection |
| `FusedCompare` / `can_fuse_with_branch` | reject in BIR | target selection/peephole decision |
| global type/size/alignment mutation | reject | verified type/layout authority is immutable |
| global initializer and element value retagging | replace narrowly | exact scalar constant canonicalization; aggregate/object layout preserved |
| function return type/size/alignment mutation | reject | preserve semantic signature |
| synthesize missing function return ABI | defer | preparation/ABI |
| reset a void/non-memory `return_abi` sentinel | reject repair | Raw has no partial ABI record; preparation emits either a complete typed plan or a diagnostic |
| parameter type/size/alignment mutation | reject | preserve semantic signature/object facts |
| synthesize parameter ABI and apply byval/sret | defer | semantic attrs already core; ABI plan later |
| local-slot type/size/alignment mutation | reject | `LocalId` is semantic object, not frame slot |
| `BinaryInst` result/operand/value retagging | replace | preserve exact types; normalize opcode or map arbitrary-width operations to `WideIntegerOp` |
| `SelectInst::compare_type` and values retagging | replace | boolean boundary normalization; scalar owns select canonicalization |
| `CastInst` result/operand retagging | replace | explicit `CastOp` mapping; never change type without a cast |
| `PhiInst` result/incoming retagging | reject | phi types remain exact; bridges are inserted on edges/uses if required, SSA owns phi form |
| call return/result/arg/callee retagging | reject | preserve exact call type; bridge values explicitly where semantics demand |
| `return_type_name = render_type(...)` | reject | rendered text is never semantic authority |
| `result_lanes` legalization | reject legacy form | core calls have one semantic result; aggregate/MIR owns physical lanes |
| grow `arg_abi` to match args | defer | preparation/ABI must produce a complete typed plan atomically |
| repair special I16 ABI | defer | target ABI; no named narrow-case patch in BIR |
| synthesize `result_abi` for named result | reject name condition | ABI depends on semantic call/result, never value spelling/kind |
| load/store result/value retagging | reject | preserve access width; explicit bridge around memory operation |
| address base value retagging | reject | pointer type/address space preserved |
| terminator return/condition retagging | replace narrowly | return type preserved; non-I1 truth explicitly compares to zero |
| construct `PreparedControlFlowBlock` | defer | CFG analysis/preparation product, not legalize mutation |
| copy branch target labels into prepared tables | reject | CFG uses typed successor slots |
| publish branch conditions/fused compares | defer | analysis/target selection |
| nullable `PreparedControlFlow*` and publish-only-when-nonempty branch | reject API shape | analyses return typed revision-bound results; canonical pass behavior cannot depend on an optional output side table |
| `completed_phases.push_back("legalize")` | replace | unforgeable pipeline checkpoint at exact revision |
| clear/repopulate `prepared_.control_flow` | reject | legalize cannot own prepared side tables |
| publish `NoTargetFacingI1` | reject | false canonical invariant; only target carrier plans may require it |
| append free-form `PrepareNote` | replace | structured diagnostics/stats, never proof of invariants |
| public `infer_call_arg_abi` from legalize TU | move | preparation/ABI API |

The legacy function also omits many instruction alternatives from its visitor.
The new pass uses the core descriptor visitor and an exhaustive opcode
classification table; adding an opcode fails inventory tests until a legalize
disposition exists.

## 14. Full backend semantic-family coverage

This matrix prevents narrow testcase legalization from being mistaken for
backend coverage.

| Family | Legalize guarantee | Later owner |
|---|---|---|
| integers 1/8/16/32/64/128 and arbitrary widths | exact bits, explicit signed operations, ordinary scalar form or width-explicit semantic node; no carrier chosen | scalar, memory, target lowering |
| F16/F32/F64/x87-80/binary128 | exact format/bits and explicit conversions/ops | scalar; runtime helper preparation/MIR |
| pointers/address spaces | no integer aliasing or address-space loss | memory/address preparation |
| constants/undef/poison/null | distinct canonical typed forms | scalar and later consumers |
| unary/binary/compare/cast/select | closed enums and boolean boundaries | scalar |
| checked overflow/multi-result atomics | result arity and semantics preserved | scalar/intrinsics/MIR |
| vectors/SIMD/shuffle | scalar leaves canonical; lanes/masks intact | scalar/intrinsics/target |
| aggregates/complex | topology and one semantic value intact | aggregate/ABI/MIR |
| globals/initializers/relocations | exact scalar leaves; object ranges unchanged | emission after preparation |
| locals/dynamic stack/lifetimes | semantic object/type intact | memory/frame/MIR |
| loads/stores/transfers/atomics/fences | width, alignment, volatility, address space, order intact | memory/intrinsics/MIR |
| direct/indirect/variadic/tail/effectful calls | full semantic signature/effects/bundles intact | ABI/calls/variadic preparation |
| intrinsic/runtime-capable ops | registered target-independent owner, no target disposition or helper chosen | intrinsics/runtime helpers |
| branches/switch/indirect/asm-goto/unreachable | successor authority unchanged; conditions i1 | cfg/ssa |
| inline asm | target-independent grammar normalized, target tokens retained | inline-asm preparation/MIR |
| TLS, aliases, constructors, directives, top-level asm | untouched semantic IDs/metadata | preparation/emission |
| debug/origin | preserved/composed, never semantics | debug emission |
| exceptions/unwind | only accepted if core/importer first define typed forms and this table gains a disposition | future explicit owner; otherwise importer failure |

Coverage is not “complete” merely because this pass accepts a family. Each row
requires an executable later owner and end-to-end proofs before implementation
can claim the old backend feature is replaced.

## 15. Proof and review plan

### 15.1 Contract/inventory proofs

- pin every Raw opcode, payload alternative, operand/result role, terminator,
  constant alternative, type alternative, and intrinsic registry category to
  one row in this document;
- fail schema review when a new Raw form lacks a `LegalizeClass` and named owner;
- compare legacy BIR/prealloc consumers and the reference IR/backend families
  against section 14; absence must be a source gap, explicit unsupported
  feature, or later owner—not silence.

### 15.2 Positive semantic matrices

- every integer width around boundaries: 1, 2, 7, 8, 9, 15, 16, 17, 31, 32,
  33, 63, 64, 65, 127, 128, and >128 registered/unregistered cases;
- signed/unsigned division, remainder, compare, shifts, overflow, casts, and
  constant extremes for each width;
- boolean values from integer, pointer, float (including NaN and signed zero),
  compare, phi, select, call, load/store, parameter, and return boundaries;
- F16/F32/F64/x87 extended-80/binary128 zeros, infinities, subnormals, NaNs,
  payloads, conversions, arithmetic, comparison, and exact constants;
- direct/indirect/variadic calls with effects, attributes, bundles, aggregate
  result, i128/f128 result, and no-return/returns-twice behavior;
- volatile/address-space memory, dynamic sizes, all atomic orderings, cmpxchg
  result forms, aggregates, vectors, intrinsics, and asm-goto.

### 15.3 Negative and non-overfit proofs

- malformed IDs, missing call fields, text-only inline asm, absent atomic
  ordering, and producer gaps fail before legalize; tests assert the pass never
  repairs them;
- unrepresentable portable semantics and missing downstream-owner registrations
  fail transactionally with unchanged semantic dumps and revisions; lack of a
  target instruction/helper is tested only at `PreparedInput`;
- attempts to insert ABI classes, call registers, frame offsets, helper names,
  target opcodes, or prepared records are rejected by stage-boundary checks;
- arbitrary-width tests include neighboring widths and every operation family,
  not one named regression;
- ABI matrices cover x86-64, i686, AArch64, and RISC-V only in later preparation
  tests; legalize output must be identical when semantic layouts/formats are
  identical and must never vary with register availability;
- compare/branch tests prove this pass does not publish fusion decisions;
- load/store tests prove object size/alignment are unchanged by operation
  legalization;
- idempotence runs the pass twice and requires byte-identical semantic dumps,
  empty second mutation summary, and unchanged second-run revisions;
- failure injection at every editor operation proves whole-module rollback.

### 15.4 Equivalence obligation

Every rewrite needs a local, reviewable equivalence statement over exact typed
semantics. For integer operation-family rewrites this includes modulo width,
signedness, shift-domain behavior, divide traps/undefined behavior,
poison/undef, and overflow results. For FP it includes format, rounding
contract, NaN behavior, exceptions where modeled, and signed zero. For boolean
conversion it includes NaN truth behavior and pointer null semantics. A green
target testcase without these obligations is insufficient.

## 16. Source gaps and open design choices

The following prevent this contract from being considered final:

1. The checked-in `core/type.hpp` and `core/ir.hpp` are bootstrap-only and do
   not yet implement the full type/opcode/payload schema documented by core.
2. The exact payload and descriptor registry for arbitrary-width
   `WideIntegerOp` must be settled before implementing section 5; every
   operation must preserve its exact `iN` type and semantics.
3. The ordinary-scalar versus `WideIntegerOp` width partition needs a
   versioned descriptor table; it is not a target-native-width table.
4. FP environment semantics (rounding mode, exception observability,
   contraction) need explicit core fields before aggressive scalar work; this
   pass must conservatively preserve operations meanwhile.
5. `X87Extended80` padding canonicalization must define whether padding is
   zeroed, unspecified, or preserved separately from semantic bits.
6. Pointer ordered-comparison legality and provenance semantics require one
   language/core ruling; legalize must not use pointer-to-integer as a shortcut.
7. Non-`I1` floating truth conversion must pin ordered/unordered NaN predicate
   semantics in the descriptor registry.
8. Inline-asm constraint grammar needs a versioned target-independent token
   schema and a clear boundary between grammar normalization here and target
   token validation in preparation.
9. Target capability and per-function feature validation belongs to
   `PreparedInput`; legalize's semantic-owner registry remains module-independent
   and frozen for the pipeline fingerprint.
10. Exception/unwind operations have no accepted typed producer/core family;
    they remain importer failures rather than opaque legalize nodes.
11. The framework's whole-module editor transaction capable of atomically
    editing module entities and function bodies remains an explicit API gap.
    Legalize cannot be implemented as specified until core supplies it.

None of these gaps authorizes fallback to legacy names, strings, host floating
point, target ABI tables, or testcase-shaped special cases.

## 17. Evidence anchors inspected

### C4C accepted design and current bootstrap

- `src/backend/bir/core/README.md`: type/constant schema, opcode families,
  descriptor traversal, stable IDs, def-use, editors, revisions, and stage
  authority.
- `src/backend/bir/lir_to_bir/README.md`: exhaustive import boundary, Raw-only
  forms, source gaps, and backend semantic coverage ledger.
- `src/backend/bir/verify/README.md`: cumulative profiles, Raw trust boundary,
  type/call/memory/aggregate/intrinsic/asm rules, forbidden prepared facts, and
  transactional publication.
- `src/backend/bir/core/{type.hpp,ir.hpp,builder.cpp}` and
  `src/backend/bir/verify/verifier.cpp`: current partial implementation. AST
  inventory confirms these are bootstrap coverage, not the full documented
  schema.
- `src/backend/bir/lir_to_bir/{types,scalar,calling,call_abi}.cpp` and memory
  importer files: current importer seams and behavior that must not leak ABI or
  memory normalization into this pass.

### Legacy C4C quarantine evidence

- `src/backend/legacy/prealloc/legalize.cpp`: AST inventory of
  `should_promote_i1`, `legalize_type`, `legalize_value`, `type_size_bytes`,
  `legalize_sized_type`, ABI repair helpers, memory metadata rewrite,
  comparison/branch publication, `legalize_module`, `infer_call_arg_abi`, and
  `BirPreAlloc::run_legalize`.
- `src/backend/legacy/bir.hpp`, `bir_validate.cpp`, call/comparison/control-flow/
  memory views, and Route1–Route8 files: feature and consumer evidence only;
  names/routes are not new authority.
- `src/backend/legacy/prealloc/{comparison,call_plans,atomics,intrinsics,
  inline_asm,out_of_ssa,frame_plan,variadic_entry_plans,runtime_helpers}.cpp`,
  `regalloc/`, and `stack_layout/`: downstream features used to verify the
  ownership map, not code to transplant into canonical BIR.

### Reference compiler feature oracle

- `ref/claudes-c-compiler/src/ir/instruction.rs`, `ir/ops.rs`,
  `ir/intrinsics.rs`, and `common/types.rs`: scalar, cast, comparison, i128,
  f128/long-double, complex, atomic, vector, intrinsic, call, and asm families.
- `ref/claudes-c-compiler/src/backend/cast.rs`: useful shared cast
  classification shape; reject its pointer-as-host-integer and x86
  F128-as-F64 approximations as canonical BIR semantics.
- architecture `codegen/{cast*,alu,comparison,float_ops,i128_ops,f128,
  calls,memory,intrinsics,inline_asm}.rs`: evidence that exact operation/type
  coverage is needed, while all instruction/helper/register choices remain
  outside this pass.
- `ref/claudes-c-compiler/src/backend/{call_abi,liveness,regalloc}.rs` and
  `stack_layout/`: evidence for later ownership; none is input authority for
  legalize.

The reference backend is a coverage oracle, not a correctness oracle. In
particular, host-width reductions, architecture-specific no-op casts, hidden
complex return lanes, and direct helper selection must not be copied into
canonical BIR.
