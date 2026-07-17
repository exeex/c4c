# Idea 732 Step 8A: Non-Importer Cross-Phase Closure

Status: accepted documentation audit; Step 8B entry gate unmet
Scope: Markdown architecture only; no implementation or importer-final claim
Inventory-Basis: every `src/backend/bir/**/*.md` path present in this worktree

## Result

The non-importer A-to-F contract is internally closed. All 47 current BIR
Markdown paths are inventoried exactly once below, and all 31 ordered rows from
A1 through F3 are accounted for exactly once. Every non-importer seam has one
indexed owner, an exact predecessor/output relationship, explicit identity and
provenance treatment, exact revision/target/product-key requirements,
invalidation, a verifier or gate, failure-atomic behavior, truthful
implementation status, and a terminal authority boundary.

This is documentation acceptance, not capability acceptance. The importer-
dependent facts listed under “Reserved for Step 8B” remain provisional. No
Raw/A1/B1 conclusion in this audit overrides landed code evidence.

## Authority findings

| Concern | Unique normative authority | Audit conclusion |
|---|---|---|
| NodeKind/tag algebra, stage admission, identity gate | `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md` | Local documents cite it and own only phase-local dispositions; no second registry or catch-all inheritance is normative. |
| Storage, typed IDs, graph revision | `src/backend/bir/core/README.md` | Pointer, slot, order, spelling, and analysis indices are not identity; replacement/provenance follows the common identity gate. |
| Ordered phase occurrences | `src/backend/bir/README.md` and `pipeline/README.md` | The sole success order is A1 through F3; only the explicit private E3-to-E1 retry is additional. |
| Pass transaction shape | `passes/README.md` | Mutating owners stage private candidates and publish only after exact gates; local matrices close their own input sets. |
| Verification/public capabilities | `verify/README.md` plus the row owner | Gate ownership is unique; pass-local preconditions do not mint public capabilities. |
| Analysis/product keys and invalidation | `analysis/README.md` | Equal-looking or compatible products cannot replace exact revision/target/product identity. |
| C target/profile authority | external `src/target_profile/README.md`; BIR C owners consume it | BIR does not duplicate triple, OS, ABI, relocation, or float-ABI selection authority. |
| F machine and terminal semantics | external MIR, target assembler/encoder/linker, and MIR object owners | BIR-local F1-F3 own exact admission, disposition, gates, and handoff accounting only. |

## Complete BIR Markdown inventory

Each current path occurs in exactly one row. “Spine” means the exact adapted
17-section per-pass contract; “support” means the document intentionally owns
a shared schema, analysis, audit, or orchestration concern instead.

| # | Path | Class | Status and unique responsibility |
|---:|---|---|---|
| 1 | `src/backend/bir/LEGACY_COVERAGE.md` | support/audit | Reviewed legacy disposition ledger; no semantic implementation authority. |
| 2 | `src/backend/bir/README.md` | root | Sole A1-F3 ordered index and common authority routing; partial foundation is stated. |
| 3 | `src/backend/bir/REVIEW_TEMPLATE.md` | support/audit | Review questions only; mints no capability. |
| 4 | `src/backend/bir/allocated/README.md` | E4 spine | Absent E4 frame-action/final-product/Allocated publication contract. |
| 5 | `src/backend/bir/analysis/README.md` | support | Absent common exact-key analysis/product and invalidation framework. |
| 6 | `src/backend/bir/analysis/call_graph/README.md` | analysis | Absent revision-bound call-graph product. |
| 7 | `src/backend/bir/analysis/cfg/README.md` | analysis | Absent revision-bound CFG product. |
| 8 | `src/backend/bir/analysis/comparison/README.md` | analysis | Absent comparison/select product. |
| 9 | `src/backend/bir/analysis/dominance/README.md` | analysis | Absent dominance product. |
| 10 | `src/backend/bir/analysis/liveness/README.md` | E1 spine | Absent allocation liveness/interference owner. |
| 11 | `src/backend/bir/analysis/memory_effects/README.md` | analysis | Absent conservative memory-effect product. |
| 12 | `src/backend/bir/analysis/provenance/README.md` | analysis | Absent provenance product. |
| 13 | `src/backend/bir/analysis/publication/README.md` | analysis | Absent publication/value-flow product. |
| 14 | `src/backend/bir/compatibility/README.md` | support | Unimplemented compatibility quarantine, never a semantic fallback. |
| 15 | `src/backend/bir/core/README.md` | support/schema | Partial checked-in storage/ID/query surface and Raw receiving schema. |
| 16 | `src/backend/bir/diagnostics/README.md` | support | Unimplemented read-only diagnostics/rendering authority. |
| 17 | `src/backend/bir/emission/README.md` | F3 spine | Partial-external/absent-uniform terminal admission, routing, and output accounting. |
| 18 | `src/backend/bir/lir_to_bir/README.md` | A1 boundary | Partial checked implementation ledger; final importer facts reserved for 8B. |
| 19 | `src/backend/bir/lir_to_bir/memory/README.md` | A1 sub-boundary | Absent build-excluded migration placeholder; cannot publish independently. |
| 20 | `src/backend/bir/machine_construction/README.md` | F1 spine | Absent exact-view, apply-only, one-record machine handoff. |
| 21 | `src/backend/bir/machine_verification/README.md` | F2 spine | Absent shared exact-candidate, non-mutating machine gate. |
| 22 | `src/backend/bir/passes/README.md` | support | Partial common B-pass transaction/matrix framework. |
| 23 | `src/backend/bir/passes/aggregate/README.md` | B6 spine | Absent aggregate normalization. |
| 24 | `src/backend/bir/passes/call_lowering/README.md` | D2 spine | Absent shared ABI-aware pseudo call lowering. |
| 25 | `src/backend/bir/passes/cfg/README.md` | B3 spine | Absent CFG normalization. |
| 26 | `src/backend/bir/passes/intrinsics/README.md` | B7 spine | Absent intrinsic normalization. |
| 27 | `src/backend/bir/passes/legalize/README.md` | B1 spine | Absent Raw admission/type-op legalization; exact input remains 8B-sensitive. |
| 28 | `src/backend/bir/passes/memory/README.md` | B5 spine | Absent memory/address/effect normalization. |
| 29 | `src/backend/bir/passes/out_of_ssa/README.md` | D5 spine | Absent phi lowering and private post-E3 copy-resolution closure. |
| 30 | `src/backend/bir/passes/pseudo_lowering/README.md` | D1 spine | Absent generic pseudo lowering. |
| 31 | `src/backend/bir/passes/scalar/README.md` | B2 spine | Absent scalar/comparison/select normalization. |
| 32 | `src/backend/bir/passes/ssa/README.md` | B4 spine | Absent dynamic SSA construction/proof. |
| 33 | `src/backend/bir/passes/target/README.md` | D4 spine | Absent mandatory target-realizability legalization gate. |
| 34 | `src/backend/bir/pipeline/README.md` | support | Partial-foundation B1-B8 occurrence/checkpoint/publication orchestration. |
| 35 | `src/backend/bir/preparation/README.md` | support | Absent common immutable C-product sequencing/publication framework. |
| 36 | `src/backend/bir/preparation/abi/README.md` | C3 spine | Absent immutable ABI plan. |
| 37 | `src/backend/bir/preparation/address/README.md` | C6 spine | Absent immutable address/relocation-requirement plan. |
| 38 | `src/backend/bir/preparation/calls/README.md` | C4 spine | Absent immutable call plan. |
| 39 | `src/backend/bir/preparation/inline_asm/README.md` | C7 spine | Absent target-context vocabulary; never parses asm text. |
| 40 | `src/backend/bir/preparation/runtime_helpers/README.md` | C8 spine | Absent cumulative helper plan/publication. |
| 41 | `src/backend/bir/preparation/variadic/README.md` | C5 spine | Absent immutable variadic plan. |
| 42 | `src/backend/bir/pseudo/README.md` | support/schema | Documentation-only D-E pseudo schema; no production NodeKind claim. |
| 43 | `src/backend/bir/regalloc/README.md` | E2 spine | Absent finite abstract-home allocator. |
| 44 | `src/backend/bir/regalloc/constraints/README.md` | C9 spine | Absent constraint binding and sole later-revision projection authority. |
| 45 | `src/backend/bir/regalloc/spill_reload/README.md` | E3 spine | Absent explicit spill/reload and bounded private retry owner. |
| 46 | `src/backend/bir/target_layout/README.md` | C2 spine | Absent profile-keyed finite pseudo-layout product. |
| 47 | `src/backend/bir/verify/README.md` | support/gate | Partial Raw foundation; later cumulative profiles remain prospective. |

Inventory result: 47 unique paths, zero duplicates, zero omissions.

## Complete ordered-row adjacency audit

“Owner” is the indexed row owner; shared frameworks remain authorities for
their one concern but do not create a second row owner. Every output is the
exact next-row input unless the row explicitly produces immutable cumulative
facts or the private E3 retry candidate.

| # | Row | Exact predecessor | Exact output / next adjacency | Unique owner and gate result |
|---:|---|---|---|---|
| 1 | A1 | typed LIR | complete private Raw candidate -> A2 | `lir_to_bir/README.md`; final receiving facts deferred to 8B |
| 2 | A2 | A1 private candidate | exact verified `RawBir` -> B1 | `verify/README.md` Raw gate over core; atomic or nothing |
| 3 | B1 | A2 `RawBir` | stamped legal candidate -> B2 | `passes/legalize/README.md`; matrix finalization deferred only where Raw-dependent |
| 4 | B2 | exact B1 output | normalized scalar candidate -> B3 | `passes/scalar/README.md`; exact revision gate |
| 5 | B3 | exact B2 output | normalized CFG candidate -> B4 | `passes/cfg/README.md`; CFG product invalidated/rebuilt |
| 6 | B4 | exact B3 output | dynamic-SSA candidate -> B5 | `passes/ssa/README.md`; static tag never substitutes for proof |
| 7 | B5 | exact B4 output | normalized memory candidate -> B6 | `passes/memory/README.md`; exact effects/provenance keys |
| 8 | B6 | exact B5 output | normalized aggregate candidate -> B7 | `passes/aggregate/README.md`; closed dispositions |
| 9 | B7 | exact B6 output | stamped intrinsic-normal candidate -> B8 | `passes/intrinsics/README.md`; no helper/target backfill |
| 10 | B8 | exact stamped B7 output | verified immutable `CanonicalBir` -> C1 | pipeline/pass framework plus verifier Canonical gate; one publication owner |
| 11 | C1 | Canonical plus requested target tuple | validated exact target-context key -> C2 | external target-profile authority; no BIR mutation |
| 12 | C2 | exact C1 key | verified finite target-layout product -> C3 | `target_layout/README.md`; profile/revision keyed |
| 13 | C3 | Canonical plus C2 | immutable ABI plan -> C4 | `preparation/abi/README.md`; no graph mutation |
| 14 | C4 | exact cumulative C3 | immutable call plan -> C5 | `preparation/calls/README.md`; exact keys |
| 15 | C5 | exact cumulative C4 | immutable variadic plan -> C6 | `preparation/variadic/README.md`; exact keys |
| 16 | C6 | exact cumulative C5 | immutable address plan -> C7 | `preparation/address/README.md`; relocation requirements, not encoding |
| 17 | C7 | exact cumulative C6 plus opaque asm | immutable asm target context -> C8 | `preparation/inline_asm/README.md`; no parsing/binding |
| 18 | C8 | exact cumulative C7 | verified cumulative preparation bundle -> C9 | `preparation/runtime_helpers/README.md` plus preparation atomic gate |
| 19 | C9 | exact C8/layout/raw constraints | `BoundConstraintSet` and exact projection authority -> D1 | `regalloc/constraints/README.md`; sole interpretation/projection owner |
| 20 | D1 | Canonical plus exact C products | private pseudo candidate + projection -> D2 | `passes/pseudo_lowering/README.md`; fresh revision |
| 21 | D2 | exact D1 candidate/products | explicit ABI-aware call candidate + projection -> D3 | `passes/call_lowering/README.md`; abstract roles only |
| 22 | D3 | exact private D2 candidate/projection | verified Pseudo publication -> D4 | verifier Pseudo gate; no allocation requirement |
| 23 | D4 | exact verified D3 output/projection | directly one-record-realizable pseudo revision -> D5 | `passes/target/README.md`; full Pseudo reverification |
| 24 | D5 | exact D4 output then stable E3 candidate | initial parallel-copy revision -> E1; later resolved private candidate -> E4 | `passes/out_of_ssa/README.md`; no public post-E3 capability |
| 25 | E1 | exact initial D5 or E3-mutated revision | liveness/interference/pressure facts -> E2 | `analysis/liveness/README.md`; revision-bound |
| 26 | E2 | exact E1/projection/layout pools | abstract homes or bounded eviction request -> E3 | `regalloc/README.md`; finite allocation authority |
| 27 | E3 | exact E2 candidate/E1 facts | explicit spill/reload candidate -> fresh E1, or stable state -> D5 closure | `regalloc/spill_reload/README.md`; only deliberate private retry |
| 28 | E4 | exact private D5-resolved candidate | materialized frame actions, final products, owning Allocated plus borrowing MIR-ready view -> F1 | `allocated/README.md` plus Allocated verifier; atomic dual publication |
| 29 | F1 | exact E4 `MirReadyBirView` and products | sealed private distinct machine graph -> F2 | `machine_construction/README.md`; one record per node, apply-only |
| 30 | F2 | exact sealed F1 candidate/manifest | verified-machine capability -> F3 | `machine_verification/README.md`; read-only exact gate |
| 31 | F3 | exact F2 capability and request | assembly/image/object/relocation/link products -> named external consumers | `emission/README.md`; handoff/accounting only |

The phase counts are A=2, B=8, C=9, D=5, E=4, and F=3: exactly 31 rows.
Subordinate C9 reprojections and D5 post-E3 copy resolution retain their named
C9/D5 authority and are not additional A-F rows or duplicate owners.

## Cross-cutting proof

- Vocabulary: every spine uses the normative NodeKind/tag reference and a
  closed local matrix; unknown, illegal, omitted, stale, and premature values
  fail closed. There is no all-stage or inheritance catch-all.
- Identity: retained identity requires the common gate. Replacements and F1
  machine records get fresh IDs; BIR IDs become provenance only at F.
- Keys: analyses and C-E products bind exact revision, target, profile,
  registry, and predecessor fingerprints. Semantic equality never substitutes.
- Invalidation: graph/product/target mutation invalidates all observing
  products. Only E3 has a bounded mutation/recompute loop; validators do not
  repair.
- Gates: Raw, Canonical, Pseudo, private-assigned, Allocated, F1, F2, and F3
  admission are cumulative and failure-atomic. Partial-function publication is
  forbidden.
- Status: checked-in Raw/core/pipeline pieces remain partial; B1-F2 planned
  contracts are absent; F3 is partial only because external consumers exist,
  not because the uniform F2-gated path is implemented.
- Terminal authority: target profile, MIR semantics, assembler grammar,
  encoding, object/relocation semantics, and linker policy remain external.
  BIR owns only exact consumption and handoff boundaries.

## Link and metadata audit

All 47 files were read once by inventory tooling. All deliberate relative
Markdown file links resolve after excluding regex text that merely resembles a
link. Stale `lir_to_bir/README.md` sequencing and ownership wording was
corrected: 732 is active, only Step 8B awaits completed parallel-route
evidence, and historical 734 is not current lifecycle authority. No importer
fact changed.

All phase spines carry truthful contract and implementation metadata. The 26
current exact-spine owners have the adapted 17 headings in the same order and
a closed matrix. The remaining 21 documents are intentional root, framework,
analysis, schema, diagnostics, compatibility, or audit owners and therefore do
not pretend to be per-pass spines.

## Reserved for Step 8B

Only a named completed importer revision with inspectable code and matching
proof may settle these facts:

1. complete Raw `NodeKind` output set and B1 exact admission;
2. every payload alternative and operand/result/type role;
3. constants, globals, declarations, functions, blocks, terminators, CFG edges
   and ordering;
4. source-origin and stable-source-identity mapping;
5. metadata transport, ownership, and rejection;
6. private candidate/builder API and rollback behavior;
7. full Raw verifier rules and atomic publication capability;
8. every accepted and rejected LIR alternative with code/test evidence; and
9. proof that target/profile/layout/allocation/machine facts are absent.

No local non-importer document may infer these from design intent, the current
partial importer, a branch head, or an unfinished blocker.

## External route evidence and 8B handoff

The exact observed external progress marker is
`origin/new_bir@13123e7524c12a307bae372007be60e0a4e656a3`. At that revision the
external route retired the exhausted idea 734 runbook and activated blocker
idea 763 at Step 1. This is progress evidence only. It is not a final importer
revision and does not satisfy the Step 8B entry gate.

The runbook therefore remains active at Step 8B. Entry is currently unmet; the
next action is to wait for a named completed importer revision and then inspect
its code and proof. This is neither lifecycle closure nor a blocked-status
decision.
