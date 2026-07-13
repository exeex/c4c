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
schema, and runs the shared BIR liveness, allocation, and explicit
spill/reload flow. Only a verified `MirReadyBirView` crosses into MIR. MIR maps
pseudo homes to concrete target registers and lowers pseudo instructions to
machine instructions; it does not redo ordinary allocation or repair register
pressure.

This file is the normative top-level order. Subordinate documents may refine
their own input, output, verification, and invalidation contracts, but cannot
reorder adjacent stages. If a local document disagrees with this order, that
document is not yet converged.

## Normative stage and pass order

Every always-on row has exactly one predecessor and successor. `S21` is the
only optional extension point and is skipped unless an explicitly reviewed
target pseudo pass is enabled. `S25` has one deliberate retry edge to `S23`.

| ID | Stage / pass | Consumes | Publishes or guarantees | Owner |
|---|---|---|---|---|
| `S00` | LIR import and draft construction | typed LIR | one private, frozen `ModuleDraft`; importer errors publish no BIR | [LIR-to-BIR](lir_to_bir/README.md), with [memory import](lir_to_bir/memory/README.md) reserved as a build-excluded migration placeholder |
| `S01` | Draft/Raw verification and publication | `ModuleDraft` | verified, target-independent, unallocated `RawBir` | [BIR verifier](verify/README.md) over [Raw BIR core](core/README.md) |
| `S02` | `P01 legalize` | `RawBir` | legal target-independent type/opcode forms | [legalize pass](passes/legalize/README.md) |
| `S03` | `P02 scalar` | `P01` output | normalized scalar, comparison, and select forms | [scalar pass](passes/scalar/README.md) |
| `S04` | `P03 cfg` | `P02` output | normalized terminators, blocks, and edges | [CFG pass](passes/cfg/README.md) |
| `S05` | `P04 ssa` | `P03` output | canonical SSA definitions, uses, and phi form | [SSA pass](passes/ssa/README.md) |
| `S06` | `P05 memory` | `P04` output | canonical memory, address, atomic, and effect-bearing forms | [memory pass](passes/memory/README.md) |
| `S07` | `P06 aggregate` | `P05` output | normalized aggregate values, copies, and projections | [aggregate pass](passes/aggregate/README.md) |
| `S08` | `P07 intrinsics` | `P06` output | canonical target-independent intrinsic forms | [intrinsics pass](passes/intrinsics/README.md) |
| `S09` | Canonical verification and publication | stamped `P07` output | verified, target-independent, unallocated `CanonicalBir` | verifier `Canonical` profile; canonical orchestration is defined by the [pass framework](passes/README.md) and [pipeline contract](pipeline/README.md) |
| `S10` | `TargetProfile` selection and validation | `CanonicalBir` plus requested triple/arch/OS/ABI/relocation/float-ABI capabilities | one exact validated target-context key; no BIR mutation | external target-profile authority; BIR consumes the selected profile without adding target facts to Raw or Canonical storage |
| `S11` | BIR target-layout derivation | validated `TargetProfile` | verified profile-keyed pseudo categories, classes/groups, slots, aliases, reserved units, capacities, ABI eligibility, and concrete-mapping domain | [target layout](target_layout/README.md) |
| `S12` | Preparation 1: ABI | `CanonicalBir` plus verified target layout | typed parameter/result/byval/sret/classification requirements | [ABI plan](preparation/abi/README.md) |
| `S13` | Preparation 2: calls | `S12` facts | typed call input/output, preservation, clobber, and return requirements | [call plan](preparation/calls/README.md) |
| `S14` | Preparation 3: variadic | `S13` facts | typed variadic entry, save-area, promotion, and traversal requirements | [variadic plan](preparation/variadic/README.md) |
| `S15` | Preparation 4: address | `S14` facts | typed address-materialization and relocation requirements | [address plan](preparation/address/README.md) |
| `S16` | Preparation 5: inline-asm target context | `S15` facts plus opaque canonical `InlineAsm` payloads | target constraint vocabulary, eligibility/context tables, and clobber vocabulary only; no constraint parsing or operand binding | [inline-asm preparation](preparation/inline_asm/README.md) |
| `S17` | Preparation 6: runtime helpers | `S16` facts | complete verified cumulative preparation bundle, including helper interface requirements | [runtime-helper plan](preparation/runtime_helpers/README.md); preparation sequencing and atomic publication belong to [preparation](preparation/README.md) |
| `S18` | Register-constraint parsing, typing, and binding | cumulative preparation facts, target-layout tables, raw strings such as `r`, `=r`, `VR`, and `VRM2`, and ordinary instruction operands/results | typed class/group requirements, ties, early-clobber exclusions, and abstract clobber units | [register constraints](regalloc/constraints/README.md), the sole BIR owner of this interpretation |
| `S19` | Pseudo lowering | `CanonicalBir`, verified layout, cumulative preparation facts, and typed constraints | a new immutable revision containing only the admitted target-aware, machine-independent pseudo-node schema | [pseudo lowering](passes/pseudo_lowering/README.md) against the [pseudo instruction schema](pseudo/README.md) |
| `S20` | Pseudo verification and publication | private `S19` candidate | verified Pseudo BIR; no allocation completeness is required yet | verifier `Pseudo` profile |
| `S21` | Optional reviewed target pseudo pass | verified Pseudo BIR | a new pseudo revision followed by mandatory Pseudo reverification | [target pseudo-pass extension](passes/target/README.md); absent unless separately specified and reviewed |
| `S22` | Out of SSA | `S20` output, or reverified `S21` output | pseudo BIR with phi semantics lowered to explicit edge/parallel-copy operations | [out-of-SSA pass](passes/out_of_ssa/README.md) |
| `S23` | Allocation liveness and interference | exact post-out-of-SSA pseudo revision plus layout/constraint/call facts | revision-bound live ranges, interference, and pressure facts | shared allocation analysis consumed by the [BIR register allocator](regalloc/README.md) |
| `S24` | Shared pseudo-physical register allocation | `S23` facts and finite target-layout pools | legal abstract `(category, class/group, slot)` homes or explicit eviction requests | the same shared BIR allocator for RV64, AArch64, and x86 |
| `S25` | Explicit spill/reload insertion | allocation candidate, pressure/eviction decisions, and exact liveness | abstract spill-slot identities plus admitted pseudo `Spill`/`Reload` nodes | [spill/reload](regalloc/spill_reload/README.md); candidate mutation invalidates allocation facts and retries at `S23` until stable or rejected |
| `S26` | Allocated/MIR-ready verification and publication | stable allocation candidate with explicit spill state | `AllocatedBir`/`PreparedBir` capability and borrowed `MirReadyBirView` over the same immutable revision | [allocated BIR](allocated/README.md) plus verifier `Allocated` profile |
| `S27` | MIR construction | verified `MirReadyBirView` and the exact target mapping | target MIR: pseudo homes mapped to concrete registers and pseudo nodes lowered 1:1 or by bounded expansion | external [MIR architecture](../mir/README.md) |
| `S28` | Machine verification | private target-MIR candidate | verified machine instruction graph; no ordinary BIR allocation repair | external MIR/target verifier |
| `S29` | Assembly, object, and link emission | verified machine graph, opaque inline-asm text, concrete operand mappings, relocation/object facts | encoded instructions, relocations, object file, and linked output | target assembler and external [object boundary](../mir/object/README.md) |

The canonical pass IDs `P01` through `P07` are immutable. Adding a canonical
pass requires an architecture review and an explicit renumbering decision; a
local pass document cannot insert itself between two rows. Likewise,
preparation order is exactly `abi -> calls -> variadic -> address -> inline_asm
-> runtime_helpers`.

### Retry and failure behavior

All publications are transactional: a failed verifier or planner publishes no
capability and leaves its immutable input unchanged. `S25 -> S23` is not a new
semantic pipeline route. It is a private allocation-candidate loop because
inserting `Spill` or `Reload` changes CFG-local uses and therefore invalidates
liveness, interference, and assignments. The loop ends only when `S26` can
prove complete homes and spill coverage, or when allocation fails closed.

## Verifier profiles

| Profile | Boundary | Required contract |
|---|---|---|
| `Draft/Raw` | `S01` | The draft is fully typed and structurally valid before `RawBir` publication. Raw forms explicitly admitted for canonicalization are allowed. Register homes, target allocation facts, and spill/reload state are forbidden. |
| `Canonical` | `S09` | Raw rules plus every `P01`-`P07` normal form. It remains target-independent and unallocated; ABI placement, register homes, and spill/reload state are forbidden. |
| `Pseudo` | `S20`, and again after `S21` | Only the closed pseudo-node schema is admitted, operand/result/terminator shapes are valid, and target/layout keys agree. Allocation is not required yet, so unassigned allocatable values are valid at this boundary. Concrete registers, target opcodes, and frame offsets remain forbidden. |
| `Allocated/MIR-ready` | `S26` | Every allocatable use/result has a legal pseudo-physical home or an explicit, verified spill/reload transition. Class/group, alias, reserved-unit, tie, clobber, dominance, and capacity rules are complete. Unresolved pressure, implicit spills, concrete target registers, and non-admitted nodes are rejected. |

`PreparedInput` may remain an internal cumulative input-checking capability for
preparation, but it is not a replacement for any published BIR profile and is
not `PreparedBir`. The latter name belongs only to the verified allocated
revision.

## Analyses are dependencies, not extra stages

Analyses are immutable, on-demand facts keyed to one exact module/function
revision. They do not mutate or publish semantic BIR and therefore are not a
second linear pipeline. A pass requests an analysis at or after its earliest
normal consumer below; a mutation invalidates it unless the pass proves the
fact preserved. Recalculation after invalidation is normal.

| Analysis | Earliest normal point | Normal dependency / consumer |
|---|---|---|
| [CFG](analysis/cfg/README.md) | before `S04/P03` planning; recomputed from the resulting terminators afterward | CFG transforms, Raw/Canonical verification, dominance, SSA, and allocation |
| [Dominance](analysis/dominance/README.md) | after the applicable CFG is available, normally for `S05/P04` | SSA construction/verification, provenance, and spill/reload placement |
| [Memory effects](analysis/memory_effects/README.md) | from Raw BIR before memory/intrinsic normalization | `S06/P05`, `S08/P07`, calls, barriers, and later scheduling legality |
| [Provenance](analysis/provenance/README.md) | once CFG/dominance and typed values exist | memory/address normalization and target address preparation |
| [Call graph](analysis/call_graph/README.md) | from Raw or Canonical module call semantics | intrinsic/helper decisions, ABI/call preparation, recursion and visibility queries |
| [Comparison/select](analysis/comparison/README.md) | before `S03/P02` when scalar relationships are needed | scalar normalization and later select/condition consumers |
| [Publication/value flow](analysis/publication/README.md) | once typed def-use and call semantics exist | SSA, calls, returns, ABI preparation, and diagnostics |
| [Liveness](analysis/liveness/README.md) | semantic queries may begin after CFG/SSA; allocation liveness is freshly bound at `S23` | shared analysis machinery may expose revision-bound facts, while regalloc alone owns allocation policy and spill decisions |

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
unchanged. `S16` supplies only the target vocabulary, context, and eligibility
tables. `S18` alone parses/types/binds constraint descriptions such as `=r`,
`r`, `VR`, and `VRM2` against ordinary operands/results. Explicit clobbers are
compiler contracts and participate in allocation. Concrete names such as
`a0`, `x3`, or `rax` written directly in opaque text are not inspected or
reserved; unless the user also expresses them through supported constraints or
clobbers, collisions are the user's responsibility.

## Complete documentation review order

The stage table and analysis table above are the first part of the review
index. After reviewing them in numeric/dependency order, review the following
cross-cutting and audit contracts in this exact order. Together, the links in
this README name every current `src/backend/bir/**/*.md` file exactly once;
this README itself is the overview entry.

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
schema/lowering, out-of-SSA, target pseudo extensions, register allocation,
spill/reload, and allocated publication are currently scaffold or
build-excluded owners unless their own code and proof later establish a
stronger status.

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
8. MIR consumes only `MirReadyBirView`, realizes verified pseudo assignments,
   and cannot redo normal allocation or pressure spill/reload.
9. Optional target-specific optimization is explicit, reviewed, invalidation-
   declaring, and reverified; it is never a hidden allocation authority.
10. Every authoritative stage has one verifier/publication gate, and failure
    publishes nothing.
