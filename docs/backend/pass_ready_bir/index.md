# Pass-Ready BIR Schema and Legacy Quarantine Research

Status: research package complete; implementation is not authorized by these
documents.

## Decision

c4c should replace the current immutable, hybrid identity publication format
with stable-ID mutable core BIR, terminator-derived CFG, contracted mutation,
verification, and revision-bound recomputable analyses. `RawBir` must pass a
canonical BIR-to-BIR pipeline and verifier before an unforgeable
`CanonicalBir` wrapper is produced. Target ABI, call moves, address placement,
frame layout, register allocation, out-of-SSA realization, and instruction
selection belong to typed prepared/MIR outputs, not canonical BIR.

Current display fallbacks, route snapshots, pointer/index identities, and other
legacy observations may survive temporarily only in a read-only
`LegacyBirCompatibilityCapsule`. Its writer and reader allowlist are closed,
its field/reader manifests decrease monotonically, and the terminal requirement
is zero fields and zero readers followed by deletion of the capsule.

## Answers

1. [01_current_lir_to_bir_schema.md](01_current_lir_to_bir_schema.md) — confirmed
   current Module/Function/Block/Inst/Terminator schema, construction,
   identities, consumers, freeze assumptions, and mutation hazards.
2. [02_field_classification_and_quarantine.md](02_field_classification_and_quarantine.md)
   — exhaustive C/A/L/P/X field classification, authority/lifetime rules,
   capsule schema, allowlist, and Q0–Q5 deletion gates.
3. [03_pass_ready_bir_contract.md](03_pass_ready_bir_contract.md) — stable
   FunctionId/BlockId/InstId/ValueId semantics, builders/editors, RAUW and CFG
   mutation, verifier, invalidation, and parallel processing invariants.
4. [04_reference_backend_comparison.md](04_reference_backend_comparison.md) —
   source-cited reference ownership, BlockId/Value, FlatAdj/CfgAnalysis, pass,
   and phi-removal comparison with explicit adopt/reject decisions.
5. [05_target_schema_and_api_blueprint.md](05_target_schema_and_api_blueprint.md)
   — implementable source tree, storage/types/APIs, typed RawBir → CanonicalBir
   → PreparedBir → MIR boundary, and stage authority ledger.
6. [06_staged_migration_and_followups.md](06_staged_migration_and_followups.md)
   — P0–P13 reversible migration, proof/rollback/compatibility checkpoints, and
   ordered follow-up proposals.

## Governing invariants

- Module owns symbols, resolved types, globals, and functions; each function
  owns blocks, instructions, values, and locals through generation-checked
  stable storage. Iteration order and allocation location are not identity.
- Persistent semantics use stable IDs. Names are debug/display data, raw
  pointers are mutation-scoped, and dense indices exist only inside one
  immutable analysis result tied to an exact revision.
- Terminators are the sole CFG successor authority. Phi/block-argument incoming
  keys use BlockId; split and redirect are transactional operations that update
  both terminators and SSA edge semantics.
- All operands participate in reciprocal def-use. Insertion, replacement,
  erasure, traversal, RAUW, block split, and edge redirect occur only through
  contracted editors and are verified before commit.
- Analyses are recomputable results keyed by FunctionId/FunctionRevision or
  module revision. Mutation summaries and declared preservation govern
  invalidation; stale results cannot be observed.
- Core passes cannot access compatibility or prepared/MIR policy. Per-function
  parallel work sees frozen read-only module state; cross-function mutation uses
  an exclusive module barrier.
- A `CanonicalBir` token means the canonical profile was successfully verified.
  Preparation is read-only over it and never writes ABI, frame, allocation, or
  instruction-selection facts back into canonical storage.

## Migration order

The accepted order begins with P0 no-semantic-change schema isolation and Q0
manifest capture, then stable module/function IDs, stable block/instruction/value
IDs, builder-only LIR import, mutation/def-use, verifier and stage wrappers, and
revision-bound analyses. Only then do semantic packets migrate legalization,
CFG/SSA, memory/address/atomic, and aggregate/intrinsic behavior. Prepared ABI,
call, and address plans follow verified canonical behavior; explicit out-of-SSA
then creates MIR; allocation, frame, and instruction selection cut over one
target at a time.

Every packet has an adapter, proof boundary, rollback point, and capsule
checkpoint. The rollback anchors advance from legacy publication to verified
CanonicalBir, then PreparedBir and verified MIR. No migration packet requires a
simultaneous all-family or all-target cutover.

## Provisional dependencies

Ideas 703–714 were not treated as accepted current behavior. Their unfinished
outcomes may change which current fields have active producers/consumers or how
soon a migration adapter can be deleted, but they do not change this package's
authority rules: semantic core, recomputable analysis, lowering-only input,
prepared/MIR output, and observational legacy remain distinct.

Before activating an implementation follow-up, re-audit any newly accepted
703–714 changes against:

- the Step 1 emitted-family and direct-consumer inventory;
- the Step 2 classification registry and initial Q0 manifests;
- the Step 5 dependency and authority ledger; and
- the Step 6 packet's adapter, proof, rollback, and deletion checkpoint.

If a predecessor adds a new current field/reader, it must be classified at Q0;
it cannot silently expand the capsule after Q0 is frozen. If it establishes a
new semantic initiative outside this design, lifecycle planning must split it
rather than broadening a packet.

## Unresolved human choices

These choices do not block the research decision, but implementation planning
must resolve them explicitly:

1. **Physical stable storage:** slot map, arena-plus-indirection, or equivalent,
   including bit widths for epoch/slot/generation and the no-wrap policy.
2. **SSA representation:** retain explicit phi instructions or use block
   arguments internally. Either must expose `(BlockId, ValueId)` edge semantics
   and the same verifier/mutator contract.
3. **Canonical alignment semantics:** whether a source-specified minimum
   alignment remains a core memory attribute while computed effective alignment
   belongs to preparation. The current blanket `align_bytes` classification is
   prepared output until that distinction is represented.
4. **Inline assembly boundary:** the exact target-independent opaque operation
   retained in canonical BIR versus constraint/register/encoding facts produced
   only during preparation/MIR.
5. **Initial canonical profile:** the exact raw forms legalized in the first P7
   pilot and which later families remain valid RawBir-only forms.
6. **Wrapper lifetime model:** unique ownership throughout, or shared immutable
   canonical lifetime held by PreparedBir; neither choice may permit mutation
   without consuming the state token and re-verifying.
7. **Parallelism rollout:** whether the first implementation merely enforces
   freeze/barrier capability or immediately executes function passes in parallel.
8. **First target cutover:** which target provides the smallest complete
   allocation/frame/instruction-selection proof for P13. Other targets must keep
   independent adapters and rollback points.

No unresolved choice permits core passes to read the capsule, analyses to become
semantic authority, or prepared policy to migrate back into canonical BIR.

## Acceptance and audit record

The package was audited as follows:

- Shape: exactly this `index.md` plus six numbered answer files; no extra package
  file is required.
- Coverage: Step 1 accounts for every current emitted owner/family and all eight
  required mutation hazards; Step 2 assigns every inventoried field/family once,
  including explicit field-level splits for mixed current structs.
- Source evidence: current c4c facts cite `src/backend/bir/bir.hpp`,
  `lir_to_bir/`, validators/views/routes/prealloc consumers; reference facts cite
  concrete `IrModule`, `IrFunction`, `BasicBlock`, `BlockId`, `Value`, `FlatAdj`,
  `CfgAnalysis`, pass execution, and phi-elimination symbols.
- Terminology: RawBir is pre-canonical storage; CanonicalBir is the verified
  semantic wrapper; A is recomputable analysis; PreparedBir/MIR owns target
  policy; LegacyBirCompatibilityCapsule is observational only.
- Authority consistency: terminators alone own CFG successors; IDs alone own
  semantic identity; analysis/prepared/capsule data cannot override core.
- Hazard closure: Step 3 maps insertion/removal/reorder, RAUW, block split, edge
  redirect, function movement, module growth, and stale pointer/index state to
  an API, invariant, verifier rule, barrier, or invalidation rule.
- Migration acceptance: Step 6 starts with a no-semantic-change packet, contains
  every required later capability family, and gives each packet compatibility,
  proof, rollback, and deletion checkpoints through zero fields/readers.
- Scope: documentation only; no implementation, tests, expectations, baselines,
  lifecycle source ideas, or claimed acceptance of provisional predecessor work.

The package is ready for lifecycle review and selection of the first follow-up;
it does not itself choose or activate one.
