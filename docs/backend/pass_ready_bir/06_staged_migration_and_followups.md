# Staged Migration and Follow-Ups

This sequence implements the Step 5 blueprint without a big-bang cutover. Each
packet is independently reviewable, preserves the preceding production route
behind an adapter or flag, and has an explicit proof and rollback boundary.
“Delete” means remove a manifested capsule field/reader key and record the
sorted-set difference required by Step 2.

## Global migration rules

- Land one coherent packet at a time. A packet cannot claim the next capsule
  checkpoint until its replacement path is proven and the old reader is gone.
- Production semantics remain unchanged unless the packet explicitly owns a
  semantic transformation and its before/after contract. Expectation downgrades
  and testcase-shaped exceptions are not migration proof.
- Every dual path has one authority. Initially legacy storage may remain the
  authority while new views shadow-compare; at the named cutover, the new typed
  product becomes authority and the legacy side becomes observational only.
- Rollback is a code/configuration switch to the last proven packet, not a data
  conversion backward from a later in-memory stage. Counts may return only to
  the preceding frozen capsule checkpoint, never above it.
- Proof commands are packet specifications; implementation plans must replace
  placeholders with matching repo-native build/CTest/full-scan commands and
  preserve canonical before/after logs where required.

## Ordered implementation packets

### P0 — No-semantic-change schema isolation

Create `bir/core/` facade headers and move or alias the current Module, Function,
Block, Inst, Terminator, Value, global, and local definitions behind them without
changing layout, dumps, validation, or lowering. Add dependency checks that core
cannot include LIR, prealloc, MIR, target, or compatibility definitions.

- Adapter: existing `bir.hpp` re-exports the isolated definitions; every current
  producer and consumer compiles unchanged.
- Proof: ABI/layout assertions where externally relevant, identical BIR dumps,
  validator results, full current BIR/prealloc test set, and matching regression
  baseline. This is explicitly a no-semantic-change packet.
- Rollback: revert facade/build moves; no serialized or runtime state migration.
- Compatibility checkpoint: capture Q0 exact field and reader manifests; delete
  nothing. Freeze the one raw writer and five-reader allowlist.

### P1 — Stable module and function identity/storage

Add `ModuleEpoch`, generation-checked `FunctionId`, module symbol/global IDs,
`SlotMap`, and explicit function order. Import current functions into stable
storage while retaining a vector-shaped read adapter.

- Adapter: `LegacyFunctionRange` resolves `FunctionId`s and presents the old
  iteration interface; name/link lookups shadow-compare old and new results.
- Proof: create/erase/reuse/move/grow property tests, stale/cross-module ID
  rejection, deterministic order/dumps, unchanged end-to-end corpus.
- Rollback: keep vector storage authoritative and disable stable-store publisher;
  IDs have not escaped the facade.
- Compatibility checkpoint: begin Q1; delete function name-fallback lookup keys
  only after all semantic consumers use symbol/FunctionId. Reader count unchanged.

### P2 — Stable block, instruction, value, and local IDs

Introduce `BlockId`, `InstId`, `ValueId`, `LocalId`, stable slot maps, separate
block/instruction order, and ID-backed operands/phi edges/terminators. Convert one
function at import time and shadow-render against the old structure.

- Adapter: `LegacyBlockInstView` projects ordered IDs as current vectors and
  display labels/names; route readers may still consume projections.
- Proof: generation/owner tests, stable identity under insertion/reorder/storage
  compaction, exact operand/result round trips, dump and validator parity on all
  current instruction families.
- Rollback: select legacy function body representation at the publication flag;
  dual construction is confined to import.
- Compatibility checkpoint: finish Q1 only after symbol/block/slot/value fallback
  consumers are deleted, remove `legacy.compat_validator`, and prove both field
  and reader counts are strictly below Q0.

### P3 — Builders, traversal, and import interface

Implement `ModuleBuilder`, `FunctionBuilder`, opcode operand/result descriptors,
complete operand/successor traversal, and `lower_lir_to_raw_bir`. Route all LIR
construction through builders; eliminate direct published-container writes.

- Adapter: old `try_lower_to_bir` calls `lower_lir_to_raw_bir` then projects a
  legacy view. Adapter-private parsing/name/layout maps remain in `lir_to_bir/`.
- Proof: builder negative tests for wrong owner/type/incomplete terminator,
  traversal coverage for every opcode role, identical raw dumps and supported
  behavior, dependency audit proving no L fields cross publication.
- Rollback: old lowering entry remains selectable; new builder output is not yet
  required by downstream consumers.
- Compatibility checkpoint: delete raw-name construction fields as each importer
  receives semantic IDs; no reader deletion beyond Q1.

### P4 — Mutation transactions and def-use

Add `FunctionEditor`, reciprocal use-lists, insertion/replacement/erasure,
operand editing, typed RAUW, `split_block`, `redirect_edge`, and `erase_block`.
Initially use them in mutation-focused tests and one no-op canonical pass.

- Adapter: legacy consumers receive const projections after a transaction;
  no current producer is allowed direct mutable access once opted in.
- Proof: property/fuzz sequences with verification after every commit; explicit
  tests for all Step 1 mutation hazards, atomic rollback on failed edits, RAUW
  coverage of phi/call/memory/atomic/terminator operands.
- Rollback: disable editor-using pass; raw builder output remains usable and the
  stable schema is retained.
- Compatibility checkpoint: delete pointer/index mutation helpers, but retain
  observational route snapshots until analysis replacements land.

### P5 — Layered verifier and typed raw/canonical wrappers

Implement raw/core/CFG/stage verifier rules, structured diagnostics, move-only
`RawBir` and verified `CanonicalBir` state tokens, and a canonical pipeline that
initially contains only normalization-equivalent/no-op passes.

- Adapter: old validation calls the Raw profile; old downstream entry unwraps a
  read-only canonical view only after successful verification.
- Proof: one negative test per verifier rule, malformed-ID/def-use/dominance/phi/
  stage fuzzing, current corpus passes Raw and Canonical gates, token construction
  cannot be forged by compile-time API tests.
- Rollback: keep Raw-only publication path behind a temporary flag; do not weaken
  verifier expectations to regain compatibility.
- Compatibility checkpoint: legacy validator rendering may remain, but verifier
  decisions cannot read capsule data; remove any such reader before proceeding.

### P6 — Revision-bound analysis manager and route replacement

Add function/module revisions, `MutationSummary`, `PreservedAnalyses`, dense CFG,
dominance, memory/effect/provenance, publication, comparison, call/return, and
call-graph results. Replace route 1–8 consumers incrementally with analysis APIs.

- Adapter: each route facade delegates to the new analysis and shadow-compares
  legacy snapshots until its reader is deleted.
- Proof: mutation/invalidation matrix tests, stale-result rejection, dense-index
  confinement checks, route-by-route equivalence over nearby feature families,
  and rebuild-after-CFG-mutation tests.
- Rollback: switch an individual facade back to its legacy computation; analyses
  are recomputable and carry no semantic state.
- Compatibility checkpoint: complete Q2 by deleting route snapshots, raw
  pointer/instruction-index records, and `legacy.route_fixture_comparator`.
  Counts must strictly decrease from Q1.

### P7 — Canonical legalization boundary

Move type/opcode/legal-form normalization into explicit canonical BIR passes.
Raw import may emit documented raw forms; `CanonicalBir` rejects them after the
pipeline. Begin with scalar/legal type normalization before complex families.

- Adapter: legacy import-time legalization can be selected per family and its
  output shadow-compared with the canonical pass.
- Proof: raw-to-canonical golden pairs, verifier rejection of residual raw forms,
  idempotence, semantic execution/codegen parity, neighboring type/op coverage.
- Rollback: switch the family to import-time legalization; stable schema and
  verifier remain.
- Compatibility checkpoint: delete each import-only raw-form field/writer after
  its pass owns the rule; no route reader may be reintroduced.

### P8 — CFG and SSA canonical passes

Implement CFG simplify, unreachable removal, block split/merge, edge redirect,
phi verification/repair, dominance, SSA construction/canonicalization, and
selected scalar transforms exclusively through `FunctionEditor`.

- Adapter: legacy CFG/phi lowering remains selectable per pass; conversion occurs
  at the stable-ID boundary, never through names or vector indices.
- Proof: differential CFG/SSA tests, dominance and phi-edge verifier coverage,
  randomized edit sequences, full same-feature cases, and canonical-pass
  idempotence/fixed-point checks.
- Rollback: disable individual passes in pipeline order; verified pre-pass
  CanonicalBir remains accepted.
- Compatibility checkpoint: delete remaining phi label/index and CFG publication
  observations once analysis/pass consumers use BlockId/ValueId only.

### P9 — Memory and address semantics

Convert local/global loads/stores, address expressions, atomics, local-array
semantic GEPs, and provenance/effect questions to canonical instructions plus
recomputable analyses. Remove semantic authority from detached tables.

- Adapter: memory facade projects canonical operations into current prealloc
  queries and shadow-compares legacy provenance records.
- Proof: alias/provenance/effect invalidation, local/global/dynamic address and
  atomic suites, mutation tests around loads/stores, target parity without
  expectation rewrites.
- Rollback: select legacy memory projection for downstream preparation; canonical
  instructions remain and can be ignored by the adapter.
- Compatibility checkpoint: delete local-array/global-static observational
  provenance fields as each new analysis proves coverage; field count decreases.

### P10 — Aggregate and intrinsic semantics

Move aggregate copies/values, semantic GEPs, intrinsic operation semantics, and
opaque inline-asm operation tokens into canonical BIR. Keep target constraints,
register classes, and instruction selection out of core.

- Adapter: aggregate/intrinsic facade translates canonical semantics to legacy
  prealloc/target expectations while prepared/MIR support grows.
- Proof: aggregate layout/value-flow, vector/scalar intrinsic, inline-asm semantic
  boundary, mutation and verifier suites across all target-independent forms.
- Rollback: family-specific legacy projection flags; no schema rollback.
- Compatibility checkpoint: delete aggregate text/layout fallback and target
  metadata from the capsule when no allowlisted reader consumes them.

### P11 — Prepared BIR, ABI, call moves, and address plans

Introduce `PreparedBir`, `AbiPlan`, `CallPlan`, and `AddressPlan`. Move computed
parameter/result classification, register/stack assignments, byval/sret/varargs,
call moves, and address materialization placement out of canonical Function,
Param, CallInst, Global, and memory fields.

- Adapter: `legacy.prealloc_adapter` translates typed plans to old prealloc
  structs; dual-plan comparison is diagnostic only.
- Proof: ABI matrix by target, direct/indirect/variadic/byval/sret/HFA calls,
  call-move cycles, address modes, revision-mismatch rejection, canonical BIR
  immutability assertions.
- Rollback: old prealloc computes from canonical projection while typed plans are
  disabled; core never regains prepared fields.
- Compatibility checkpoint: complete Q3 by deleting ABI/address/call-source
  compatibility fields and `legacy.prealloc_adapter`; counts strictly decrease.

### P12 — Out-of-SSA and MIR boundary

Implement explicit out-of-SSA after the last canonical SSA pass, with parallel
copy scheduling and critical-edge splitting through contracted mutators, then
produce target-independent `MirModule` identities/instructions.

- Adapter: MIR may initially translate to the existing prealloc/codegen input;
  canonical BIR remains available read-only for diagnostics only.
- Proof: cyclic copies, critical edges, loops, multi-phi blocks, no post-out-of-
  SSA SSA-pass execution, MIR verifier, semantic/codegen differential corpus.
- Rollback: use the existing late phi/prealloc path from verified CanonicalBir;
  disable MIR publication.
- Compatibility checkpoint: delete phi-removal and route-return debug snapshots
  after MIR consumers prove complete; retain only rendering fields if needed.

### P13 — Allocation, frame, and instruction selection cutover

Move register allocation/spills, frame object placement/offsets/prologue/epilogue,
target call realization, target opcode selection, inline-asm constraints, and
emission inputs to `AllocationPlan`, `FramePlan`, and MIR. Target emitters stop
reading canonical BIR or capsule data.

- Adapter: target-by-target MIR-to-existing-emitter bridge; cut over one target
  without blocking others.
- Proof: allocation pressure/spill suites, frame alignment/dynamic stack/unwind,
  ABI call/return corpus, instruction selection and object/runtime tests per
  target, full regression guard at each target cutover.
- Rollback: target-specific bridge selects previous prepared emitter input;
  unaffected targets remain on their prior packet.
- Compatibility checkpoint: Q4 deletes remaining display/fallback fields and
  `legacy.bir_printer`/`legacy.dump_renderer`; assert zero fields and zero readers.
  Q5 then deletes capsule type, manifests, writer, and access audit and asserts
  their symbols do not exist.

## Dependency and rollback shape

P0–P6 establish infrastructure without requiring a semantic pass rewrite.
P7–P10 migrate independent canonical capability families and can be landed one
family at a time behind adapters. P11 consumes verified canonical semantics but
does not require MIR cutover. P12 establishes the stage boundary; P13 cuts over
targets independently. No packet requires all backends or all instruction
families to change simultaneously.

At every point the last accepted typed boundary is a rollback anchor:
legacy-published BIR through P4, verified CanonicalBir from P5 onward, typed
PreparedBir from P11, and verified MirModule per target from P12/P13. Rollback
never makes legacy observations authoritative over a newer accepted boundary.

## Ordered follow-up idea proposals

These are proposals only; this research run does not create files under
`ideas/open/`.

1. **No-semantic-change BIR core isolation and dependency firewall** — P0 only,
   including Q0 manifests and matching before/after regression proof.
2. **Stable module/function identity and storage** — P1, with property tests and
   vector projection adapter.
3. **Stable block/instruction/value/local IDs** — P2 and Q1 completion.
4. **Builder-only LIR-to-raw-BIR construction** — P3, exhaustive opcode traversal.
5. **Transactional BIR mutation and def-use** — P4, including RAUW, split, and
   redirect hazard tests.
6. **Layered BIR verifier and RawBir/CanonicalBir tokens** — P5.
7. **Revisioned BIR analyses and route quarantine deletion** — P6 and Q2.
8. **Scalar legalization and canonical pass pilot** — first bounded P7 family.
9. **CFG/SSA canonical pass suite** — P8.
10. **Canonical memory/address/atomic semantics** — P9.
11. **Canonical aggregate/intrinsic semantics** — P10.
12. **Prepared BIR ABI/call/address planning** — P11 and Q3.
13. **Out-of-SSA and target-independent MIR** — P12.
14. **Per-target allocation/frame/instruction-selection migration** — one idea
    per target under P13, followed by Q4/Q5 capsule removal.

Each follow-up must link back to the corresponding packet, restate its exact
adapter/proof/rollback/checkpoint contract, and reject scope expansion into the
next packet until its own proof boundary is accepted.
