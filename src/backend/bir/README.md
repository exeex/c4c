# BIR Architecture and Ordered Review Index

Status: architecture contract and scaffold index. The Raw/Canonical carrier
has a partial bootstrap implementation; the order below is the target design
that every BIR document must converge on before the deferred target-aware and
allocation implementation starts. A listed owner or gate does not imply that
its implementation is complete. Several owners are intentionally
build-excluded placeholders.

BIR begins as target-independent, unallocated source semantics. `RawBir` and
`CanonicalBir` contain neither register homes nor spill decisions. After
Canonical publication, BIR validates a `c4c::TargetProfile`, derives a finite
pseudo-physical register layout, lowers the graph to the admitted pseudo-node
schema, lowers every call through one shared ABI-aware BIR owner, and runs the
shared BIR liveness, allocation, and explicit
spill/reload flow. Only a verified `MirReadyBirView` crosses into MIR. MIR maps
pseudo homes to concrete target registers and translates each allocated pseudo
instruction to exactly one machine instruction record; it does not expand
instructions, introduce allocatable temporaries, redo ordinary allocation, or
repair register pressure. Any target-specific one-to-many instruction
expansion belongs to the pre-allocation pseudo-BIR legalization chain so that
all introduced values participate in normal BIR analysis and allocation.

This file is the normative top-level order. Subordinate documents may refine
their own input, output, verification, and invalidation contracts, but cannot
reorder adjacent stages. If a local document disagrees with this order, that
document is not yet converged.

## Normative stage and pass order

The letter identifies the major ownership phase; the number gives the strict
execution order within that phase. The complete linear flow is therefore
`A1 -> A2 -> B1 -> ... -> F3`. The canonical pass identities `P01` through
`P07` remain visible inside phase B and are independent of this top-level
stage notation.

- Phase A — import and Raw publication.
- Phase B — target-independent canonicalization.
- Phase C — target facts and immutable preparation.
- Phase D — pseudo formation and pre-allocation legalization.
- Phase E — shared allocation and MIR-ready publication.
- Phase F — MIR construction and emission.

The normal success edge follows table order: `A1` has no predecessor, `F3` has
no successor, and `E3 -> E1` is the one deliberate retry edge. `D4` is an
always-on target-realizability gate whose legalization/expansion chain may be
empty only when every input pseudo node is already directly realizable as one
machine instruction. Separately reviewed target-specific optimizations may be
entries in that same pre-allocation chain.

| ID | Stage / pass | Consumes | Publishes or guarantees | Owner |
|---|---|---|---|---|
| `A1` | LIR import and draft construction | typed LIR | one private, frozen `ModuleDraft`; importer errors publish no BIR | [LIR-to-BIR](lir_to_bir/README.md), with [memory import](lir_to_bir/memory/README.md) reserved as a build-excluded migration placeholder |
| `A2` | Draft/Raw verification and publication | `ModuleDraft` | verified, target-independent, unallocated `RawBir` | [BIR verifier](verify/README.md) over [Raw BIR core](core/README.md) |
| `B1` | `P01 legalize` | `RawBir` | legal target-independent type/opcode forms | [legalize pass](passes/legalize/README.md) |
| `B2` | `P02 scalar` | `P01` output | normalized scalar, comparison, and select forms | [scalar pass](passes/scalar/README.md) |
| `B3` | `P03 cfg` | `P02` output | normalized terminators, blocks, and edges | [CFG pass](passes/cfg/README.md) |
| `B4` | `P04 ssa` | `P03` output | canonical SSA definitions, uses, and phi form | [SSA pass](passes/ssa/README.md) |
| `B5` | `P05 memory` | `P04` output | canonical memory, address, atomic, and effect-bearing forms | [memory pass](passes/memory/README.md) |
| `B6` | `P06 aggregate` | `P05` output | normalized aggregate values, copies, and projections | [aggregate pass](passes/aggregate/README.md) |
| `B7` | `P07 intrinsics` | `P06` output | canonical target-independent intrinsic forms | [intrinsics pass](passes/intrinsics/README.md) |
| `B8` | Canonical verification and publication | stamped `P07` output | verified, target-independent, unallocated `CanonicalBir` | verifier `Canonical` profile; canonical orchestration is defined by the [pass framework](passes/README.md) and [pipeline contract](pipeline/README.md) |
| `C1` | `TargetProfile` selection and validation | `CanonicalBir` plus requested triple/arch/OS/ABI/relocation/float-ABI capabilities | one exact validated target-context key; no BIR mutation | external target-profile authority; BIR consumes the selected profile without adding target facts to Raw or Canonical storage |
| `C2` | BIR target-layout derivation | validated `TargetProfile` | verified profile-keyed pseudo categories, classes/groups, slots, aliases, reserved units, capacities, ABI eligibility, and concrete-mapping domain | [target layout](target_layout/README.md) |
| `C3` | Preparation 1: ABI | `CanonicalBir` plus verified target layout | immutable typed parameter/result/byval/sret/classification requirements; no BIR mutation | [ABI plan](preparation/abi/README.md) |
| `C4` | Preparation 2: calls | `C3` facts | immutable typed call input/output, preservation, clobber, and return requirements; no BIR mutation | [call plan](preparation/calls/README.md) |
| `C5` | Preparation 3: variadic | `C4` facts | typed variadic entry, save-area, promotion, and traversal requirements | [variadic plan](preparation/variadic/README.md) |
| `C6` | Preparation 4: address | `C5` facts | typed address-materialization and relocation requirements | [address plan](preparation/address/README.md) |
| `C7` | Preparation 5: inline-asm target context | `C6` facts plus opaque canonical `InlineAsm` payloads | target constraint vocabulary, eligibility/context tables, and clobber vocabulary only; no constraint parsing or operand binding | [inline-asm preparation](preparation/inline_asm/README.md) |
| `C8` | Preparation 6: runtime helpers | `C7` facts | complete verified cumulative preparation bundle, including helper interface requirements | [runtime-helper plan](preparation/runtime_helpers/README.md); preparation sequencing and atomic publication belong to [preparation](preparation/README.md) |
| `C9` | Register-constraint parsing, typing, and binding | cumulative preparation facts, target-layout tables, raw strings such as `r`, `=r`, `VR`, and `VRM2`, and ordinary instruction operands/results | immutable Canonical-keyed `BoundConstraintSet`; its subordinate `ConstraintProjectionTransaction` is the sole shared authority for each later revision's `ProjectedConstraintSet` | [register constraints and projection](regalloc/constraints/README.md), the sole BIR owner of interpretation and later-revision projection |
| `D1` | Generic pseudo lowering | `CanonicalBir`, verified layout, cumulative preparation facts, and typed constraints | a new immutable revision containing only the admitted target-aware, machine-independent pseudo-node schema plus the first exact `ProjectedConstraintSet`; helper-eligible semantics are rewritten to generic pseudo calls so ordinary and runtime-helper calls share one downstream call-lowering owner | [pseudo lowering](passes/pseudo_lowering/README.md) against the [pseudo instruction schema](pseudo/README.md) |
| `D2` | Shared ABI-aware BIR call lowering | exact private D1 candidate and D1 `ProjectedConstraintSet` plus matching verified C3 `AbiPlan`, C4 `CallPlan`, cumulative preparation/product keys, and the ABI rule selected by `TargetProfile.backend_abi` | one new private revision with every `GenericCall` replaced by explicit pseudo argument moves, outgoing-call stores, call nodes, result moves, hidden sret/byval/variadic transport, fixed abstract ABI-slot requirements, caller-saved clobbers, callee-saved preservation requirements, and a freshly projected D2 constraint product | [shared call lowering](passes/call_lowering/README.md), the sole D2 owner for all supported targets; it uses only abstract pseudo slots and stack objects, cannot spell concrete registers/frame offsets/machine opcodes or perform general register assignment |
| `D3` | Pseudo verification and publication | private `D2` candidate plus its exact current `ProjectedConstraintSet` | verified Pseudo BIR; no allocation completeness is required yet | verifier `Pseudo` profile |
| `D4` | Target-specific pseudo legalization/expansion and full reverification | verified Pseudo BIR plus its exact current `ProjectedConstraintSet` | a directly realizable pseudo revision in which every semantic one-to-many target expansion, including required call-sequence legalization, has become explicit pseudo nodes, with one exact-revision projected constraint product followed by full Pseudo reverification | [target pseudo-pass extension](passes/target/README.md); it cannot redo ABI classification or general call lowering; separately reviewed optimization entries remain optional |
| `D5` | Out of SSA and owned copy-resolution closure | reverified `D4` output and exact projection; its subordinate closure later consumes the stable post-E3 allocation candidate and its projection | intermediate pseudo BIR with phi semantics lowered to explicit edge-local `ParallelCopy`/`EdgeCopy` plus preallocated scratch-reservation identities and an exact projection; after stable E3, the same owner transactionally resolves every bundle and projects the resolved revision to directly realizable single-move `EdgeCopy` nodes before E4 | [out-of-SSA pass](passes/out_of_ssa/README.md) |
| `E1` | Allocation liveness and interference | exact initial D5 or E3-retry revision plus layout/call facts and the exact current `ProjectedConstraintSet` | revision-bound live ranges, simultaneous-copy interference, and pressure facts covering all uses, definitions, fixed homes, clobbers, and non-spillable `CopyScratch` reservations | shared allocation analysis consumed by the [BIR register allocator](regalloc/README.md) |
| `E2` | Shared pseudo-physical register allocation | exact current `E1` facts, `ProjectedConstraintSet`, and finite target-layout pools | legal abstract `(category, class/group, slot)` homes for ordinary identities and every non-spillable scratch reservation, or an ordinary eviction request; shortage or forbidden alias fails closed | the same shared BIR allocator for RV64, AArch64, and x86 |
| `E3` | Explicit spill/reload insertion | allocation candidate, exact current projection, pressure/eviction decisions, and exact liveness | abstract spill-slot identities plus admitted, directly realizable pseudo `Spill`/`Reload` nodes while preserving `ParallelCopy` and scratch-reservation semantics, with one newly keyed projection per retry | [spill/reload](regalloc/spill_reload/README.md); it cannot spill scratch or resolve bundles, and candidate mutation invalidates allocation facts and retries at `E1` until stable or rejected |
| `E4` | Allocated/MIR-ready verification and publication | exact private D5-resolved candidate with explicit spill state, `CopyResolutionFingerprint`, exact resolved `ProjectedConstraintSet`, and exact revision-bound target/product keys | E4-owned `FrameRealizationTransaction` first publishes an immutable private exact-revision frame/mapping plan and the final target-realizability key incorporates it; then owning `AllocatedBir`, `PreparedBir`, and borrowing `MirReadyBirView` capabilities name that exact immutable revision and every complete assignment, spill transition, resolved copy, frame placement, and one-record mapping | [allocated BIR](allocated/README.md) plus private assigned-candidate and verifier `Allocated` gates |
| `F1` | Strict one-to-one machine-graph construction | verified `MirReadyBirView`, exact immutable `FrameRealizationPlan`, and exact target mapping | target machine graph with already fixed pseudo homes/frame placements applied and exactly one machine instruction record for each allocated pseudo instruction | external [machine architecture](../mir/README.md); the consumer cannot expand calls/instructions, synthesize argument/result moves, alter fixed ABI/frame/stack placements, introduce allocatable temporaries, allocate registers, or pressure-spill |
| `F2` | Machine verification | private target-MIR candidate | verified machine instruction graph; allocation repair is forbidden | external MIR/target verifier |
| `F3` | Assembly, object, and link emission | verified machine graph, opaque inline-asm text, concrete operand mappings, relocation/object facts | encoded instructions, relocations, object file, and linked output | target assembler and external [object boundary](../mir/object/README.md) |

Every use, definition, fixed-home requirement, and clobber introduced by `D2`
continues through target legalization, out-of-SSA, allocation liveness, the
shared allocator, and spill/reload. Call lowering cannot bypass those stages or
publish preassigned general-purpose homes.

Initial D5 publication deliberately retains simultaneous `ParallelCopy`
semantics and explicit `CopyScratch` reservation identities so E1/E2 allocate
the complete problem. After E3 reaches a stable candidate and the final E1/E2
facts are current, D5's subordinate `CopyResolutionTransaction` schedules
acyclic, overlapping, and cyclic groups through their already assigned legal
homes, publishes one new exact revision and fingerprint, and hands that private
candidate to an in-transaction exact-current closure: constraint projection,
E1 recomputation, E2 assignment validation without reallocation, E3
spill-state validation without mutation, the E4-owned non-mutating
`FrameRealizationTransaction`, and final target-realizability recomputation, in
that order. The frame product fixes exact object regions, bases, offsets,
displacements, adjustments, stack size/alignment, and registered mapping rules
for spill/reload, outgoing calls, static/dynamic frames, and every other
frame-dependent access. The final `TargetRealizabilityKey` incorporates its
exact `FrameRealizationKey`. Only their jointly keyed resolved-revision
products may reach E4. This is a continuation of D5 ownership, not an
additional A-F stage. It cannot create a temporary, change allocation, add a
spill/reload, or defer a bundle to MIR. Failure is atomic and produces no E4
capability; predecessor products stay immutable, and stable IDs or preservation
records cannot rekey them.

The canonical pass IDs `P01` through `P07` are immutable. Adding a canonical
pass requires an architecture review and an explicit renumbering decision; a
local pass document cannot insert itself between two rows. Likewise,
preparation order is exactly `abi -> calls -> variadic -> address -> inline_asm
-> runtime_helpers`.

### Retry and failure behavior

All publications are transactional: a failed verifier or planner publishes no
capability and leaves its immutable input unchanged. `E3 -> E1` is not a new
semantic pipeline route. It is a private allocation-candidate loop because
inserting `Spill` or `Reload` changes CFG-local uses and therefore invalidates
liveness, interference, and assignments. The loop ends only when one stable
candidate has complete homes, including all scratch reservations, and spill
coverage, or when allocation fails closed. The stable candidate and its exact
current E1/E2/E3 facts then enter D5 copy resolution; only that resolved output
with newly installed exact-current projected-constraint, E1, E2, E3,
frame-realization, and final target-realizability products may enter E4. E4
rejects every predecessor-keyed product.

## Verifier profiles

| Profile | Boundary | Required contract |
|---|---|---|
| `Draft/Raw` | `A2` | The draft is fully typed and structurally valid before `RawBir` publication. Raw forms explicitly admitted for canonicalization are allowed. Register homes, target allocation facts, and spill/reload state are forbidden. |
| `Canonical` | `B8` | Raw rules plus every `P01`-`P07` normal form. It remains target-independent and unallocated; ABI placement, register homes, and spill/reload state are forbidden. |
| `Pseudo` publication | `D3`, in full after the complete `D4` chain, and after initial `D5` | `PseudoPublicationGate` admits only the closed pseudo-node schema and rejects assignments and spill state. The post-`D4` interval proves every then-present node except `InlineAsm` directly one-record realizable; initial D5 may add only bounded `ParallelCopy`/`CopyScratch`. Unassigned values remain valid. |
| Private assigned candidate | `E3` retry and D5-resolved candidates | `AssignedAllocationCandidateGate` retains every cumulative graph/Pseudo schema, revision, and failure rule but admits/requires exact assignments and explicit spill state. It admits `ParallelCopy`/`CopyScratch` only before D5 resolution, publishes no second `PseudoBir` type, and is rerun by E4. |
| `Allocated/MIR-ready` | `E4` | The private candidate plus exact frame plan proves every stack, call, spill, reload, frame, scratch, and ordinary node has one registered one-record mapping. No unresolved copy intermediate, implicit spill/frame action, pressure deficit, mixed product, or unrepresented expansion is accepted. |

`PreparedInput` may remain an internal cumulative input-checking capability for
preparation, but it is not a replacement for any published BIR profile and is
not `PreparedBir`. E4 alone mints the latter as a readiness capability bound to
the exact immutable revision owned by `AllocatedBir`; `MirReadyBirView` only
borrows that same revision and its verified assignments and product keys.

## Analyses are dependencies, not extra stages

Analyses are immutable, on-demand facts keyed to one exact module/function
revision. They do not mutate or publish semantic BIR and therefore are not a
second linear pipeline. A pass requests an analysis at or after its earliest
normal consumer below; a mutation invalidates it unless the pass proves the
fact preserved. Recalculation after invalidation is normal.

| Analysis | Earliest normal point | Normal dependency / consumer |
|---|---|---|
| [CFG](analysis/cfg/README.md) | before `B3/P03` planning; recomputed from the resulting terminators afterward | CFG transforms, Raw/Canonical verification, dominance, SSA, and allocation |
| [Dominance](analysis/dominance/README.md) | after the applicable CFG is available, normally for `B4/P04` | SSA construction/verification, provenance, and spill/reload placement |
| [Memory effects](analysis/memory_effects/README.md) | from Raw BIR before memory/intrinsic normalization | `B5/P05`, `B7/P07`, calls, barriers, and later scheduling legality |
| [Provenance](analysis/provenance/README.md) | once CFG/dominance and typed values exist | memory/address normalization and target address preparation |
| [Call graph](analysis/call_graph/README.md) | from Raw or Canonical module call semantics | intrinsic/helper decisions, ABI/call preparation, recursion and visibility queries |
| [Comparison/select](analysis/comparison/README.md) | before `B2/P02` when scalar relationships are needed | scalar normalization and later select/condition consumers |
| [Publication/value flow](analysis/publication/README.md) | once typed def-use and call semantics exist | SSA, calls, returns, ABI preparation, and diagnostics |
| [Liveness](analysis/liveness/README.md) | semantic queries may begin after CFG/SSA; allocation liveness is freshly bound at `E1` | shared analysis machinery may expose revision-bound facts, while regalloc alone owns allocation policy and spill decisions |

The [analysis framework](analysis/README.md) owns revision keys, caching,
preservation declarations, and stale-result rejection. Dense indices inside an
analysis result are never semantic identity.

## Inline assembly contract

`InlineAsm` is an ordinary pseudo instruction with ordinary ordered operands
and results. Its inputs, outputs, read/write pairs, definitions, uses, phi
edges, CFG placement, liveness, interference, and register assignment follow
the same rules as every other node. A read/write output is represented by an
old input value and a distinct produced result; a constraint tie equates their
assignments without merging their SSA identities.

The assembly text remains opaque through BIR and MIR and reaches the assembler
unchanged. `C7` supplies only the target vocabulary, context, and eligibility
tables. `C9` alone parses/types/binds constraint descriptions such as `=r`,
`r`, `VR`, and `VRM2` against ordinary operands/results. Explicit clobbers are
compiler contracts and participate in allocation. Concrete names such as
`a0`, `x3`, or `rax` written directly in opaque text are not inspected or
reserved; unless the user also expresses them through supported constraints or
clobbers, collisions are the user's responsibility.

At `F1`, one allocated `InlineAsm` pseudo node becomes exactly one opaque MIR
inline-asm record with its concrete operand mappings. MIR does not parse or
expand its text and does not introduce hidden temporaries. The assembler first
parses that payload at `F3`; this opaque assembler boundary does not weaken
the strict one-node-to-one-record BIR-to-MIR allocation contract.

## Complete documentation review order

The stage table and analysis table above are the first part of the review
index. After reviewing them in phase/dependency order, review the following
cross-cutting and audit contracts in this exact order. Together, the links in
this README name all 43 current subordinate `src/backend/bir/**/*.md` files
exactly once; this README itself is the overview entry, for an exact inventory
of 44 Markdown files.

1. [Diagnostics and rendering](diagnostics/README.md) — read-only views of
   published stages and analysis facts; never semantic authority.
2. [Legacy compatibility quarantine](compatibility/README.md) — temporary,
   observational compatibility only; it cannot affect verification or
   lowering.
3. [Legacy capability coverage ledger](LEGACY_COVERAGE.md) — migration/audit
   coverage, not an implementation-completion claim.
4. [Architecture review record/template](REVIEW_TEMPLATE.md) — adjacency,
   authority, invalidation, failure, and proof questions for convergence.

Review completion means the root order, each adjacent owner contract, verifier
profiles, and cross-cutting rules agree. It does not mean the scaffold has
been implemented. In particular, `lir_to_bir/memory`, target layout, pseudo
schema/lowering, shared ABI-aware call lowering, out-of-SSA, target pseudo
legalization/expansion, register allocation, spill/reload, and allocated
publication are currently scaffold or build-excluded owners unless their own
code and proof later establish a stronger status.

## Non-negotiable authority rules

1. Stable IDs—not names, pointers, vector positions, or route numbers—are
   semantic identity.
2. Terminators are the sole persistent CFG-successor authority; predecessors
   and dense numbering are derived analysis facts.
3. Def-use and ordinary operands/results are the value authority for all
   instructions, including `InlineAsm`.
4. Raw and Canonical BIR are target-independent and unallocated.
5. `TargetProfile` selects target context; BIR derives and verifies the finite
   pseudo-register layout instead of receiving allocator state.
6. Supported targets share one BIR liveness/regalloc/spill implementation;
   target variation is data and legality rules.
7. Pseudo and Allocated stages are new immutable revisions/capabilities, not
   target facts written backward into Canonical storage.
8. Generic pseudo lowering routes ordinary and runtime-helper calls through the
   shared `D2` ABI-aware call-lowering owner. That pass selects verified ABI
   rules from `TargetProfile.backend_abi`, represents locations abstractly, and
   completes call transport before target legalization or allocation.
9. All target-specific one-to-many expansion occurs in the `D4`
   pseudo-legalization chain before out-of-SSA, liveness, allocation, and
   spill/reload, so every introduced use, definition, value, and constraint is
   ordinary BIR state covered by those stages. It may legalize call-sequence
   pseudos but cannot redo ABI classification or general call lowering.
10. MIR consumes only the read-only same-revision `MirReadyBirView`, rechecks
    its product fingerprints, and applies the exact E4-owned
    `FrameRealizationPlan` and target rule to map each allocated pseudo node to
    exactly
    one machine instruction record. It cannot change assignments, expand
    instructions or calls, synthesize argument/result moves, choose ABI
    locations, frame offsets, stack adjustments/displacements, introduce
    allocatable temporaries, redo normal allocation, or
    pressure spill/reload; mapping failure publishes no machine graph.
11. Optional target-specific optimization is an explicit, reviewed,
    invalidation-declaring, fully reverified entry in the same pre-allocation
    `D4` chain; it is never a hidden allocation authority.
12. C9's immutable `BoundConstraintSet` remains Canonical-keyed. The subordinate
    shared `ConstraintProjectionTransaction` alone emits one
    `ProjectedConstraintSet` for each exact D1/D2/D4/D5/E3/copy-resolution
    revision; it is invoked inside those transactions and is not a new stage.
13. Every authoritative stage has one verifier/publication gate, and failure
    publishes nothing.
