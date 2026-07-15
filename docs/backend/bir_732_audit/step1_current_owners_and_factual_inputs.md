# Idea 732 Step 1: Current BIR Markdown Owners and Factual Inputs

Status: factual baseline at `d5cd3aa4e309b42c28be3b96151e992d025f7903`

This audit is descriptive evidence for idea 732. It does not supersede the
normative NodeKind contract, complete a pass contract, or claim that proposed
later-stage types and APIs exist. Repository facts below mean facts present in
the checked-out `bir_md` worktree at the revision above.

## Authority map

| Subject | Authoritative evidence | Landed fact consumed by 732 |
|---|---|---|
| Shared storage and identity | `src/backend/bir/core/ir.hpp`, `core/storage.hpp`, `core/view.hpp`, `core/builder.hpp`, and `src/backend/bir/core/README.md`; closed idea 746 is design history | Instructions are arena/slot-map records addressed by stable typed IDs. `InstId` is owned by `FunctionData::insts_` and is not repeated in `InstData`; pointer, spelling, vector position, and analysis-local indices are not semantic identity. The bootstrap record still has `opcode`, a closed payload variant, ordered input operands, and a compatibility result vector. |
| NodeKind and tag algebra | `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`, current `src/backend/bir/core/ir.hpp`, and focused landed tests; closed ideas 801 and 802 are acceptance history | One C++20 `consteval`-validated registry contains the current 16 kinds and derives schema, payload, arity, tag, stage-admission, and SSA-eligibility helpers. Static `SsaEligible` never proves graph SSA; B4 owns that proof. Unknown kinds fail closed. |
| Stage vocabulary and identity gate | `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md` | Raw, Canonical, Prepared, PseudoPreallocation, Allocated, and MirReadyMachine are explicit vocabularies without implicit carry-forward. C preparation products are immutable exact-revision facts rather than a second graph. Semantic/result/type/role/effect/owner changes require replacement identity and provenance. |
| Landed Raw producer | build-included `src/backend/bir/lir_to_bir.cpp`, public `src/backend/bir/lir_to_bir.hpp`, `src/backend/CMakeLists.txt`, core builder/verifier, and current tests | The top-level importer validates input, builds one private `ModuleBuilder`, and calls `std::move(builder).publish()`. Publication runs `FoundationVerifier` and returns move-only `RawBir` or one failure containing verification diagnostics. `lower_lir_to_canonical_bir` then invokes the landed `canonicalize` foundation. Nested `src/backend/bir/lir_to_bir/*.cpp` files are excluded from the build and are not runtime evidence. |
| C++ language level | root CMake authority and closed idea 802 | Native host code is C++20. Markdown examples may use C++20 syntax, but an example is not implementation evidence. |

The normative NodeKind document owns meanings; core code owns the currently
available query surface and storage reality. BIR phase/pass documents may state
their local retain/lower/reject decisions, but may not reproduce a second tag
table or infer validity from storage reuse.

## Complete current Markdown inventory

This table accounts for every path returned by
`rg --files src/backend/bir -g '*.md'` at the audited revision.

| # | Current Markdown path | Owner class | Present responsibility and implementation truth |
|---:|---|---|---|
| 1 | `src/backend/bir/LEGACY_COVERAGE.md` | audit | Legacy capability disposition ledger; evidence/quarantine aid, not B-F semantic authority. |
| 2 | `src/backend/bir/README.md` | root | Ordered A-F index and cross-phase routing. Architecture/scaffold; several later capabilities are prospective. |
| 3 | `src/backend/bir/REVIEW_TEMPLATE.md` | audit | Cross-boundary review checklist; no pass or implementation ownership. |
| 4 | `src/backend/bir/allocated/README.md` | phase | E4 private materialization and Allocated/MIR-ready publication design; unimplemented scaffold. |
| 5 | `src/backend/bir/analysis/README.md` | analysis | Common revision-bound analysis registry/key/invalidation contract; design only. |
| 6 | `src/backend/bir/analysis/call_graph/README.md` | analysis | Call graph and call-semantics product for B7/C4; absent. |
| 7 | `src/backend/bir/analysis/cfg/README.md` | analysis | CFG product for B3 onward; absent. |
| 8 | `src/backend/bir/analysis/comparison/README.md` | analysis | Comparison/select facts for B2; absent. |
| 9 | `src/backend/bir/analysis/dominance/README.md` | analysis | Dominance facts for B4; absent. |
| 10 | `src/backend/bir/analysis/liveness/README.md` | analysis | E1 liveness/interference design; unimplemented. |
| 11 | `src/backend/bir/analysis/memory_effects/README.md` | analysis | Memory/effect facts for B5 and read-only Raw queries; absent. |
| 12 | `src/backend/bir/analysis/provenance/README.md` | analysis | Provenance facts for B5/C6; absent. |
| 13 | `src/backend/bir/analysis/publication/README.md` | analysis | Publication/value-flow facts for B4/B8/C3; absent. |
| 14 | `src/backend/bir/compatibility/README.md` | support | Legacy compatibility quarantine; design only. |
| 15 | `src/backend/bir/core/README.md` | schema | Raw storage/identity/receiving and current NodeKind interface owner; partial implementation. |
| 16 | `src/backend/bir/diagnostics/README.md` | support | Stage-safe diagnostics/rendering contract; implementation not started. |
| 17 | `src/backend/bir/lir_to_bir/README.md` | boundary | A1 LIR-to-Raw import ledger and A2 handoff; partial, but its checked-in implementation summary is stale relative to landed code. |
| 18 | `src/backend/bir/lir_to_bir/memory/README.md` | boundary | A1 memory-family sub-boundary; marked absent and cannot publish independently. |
| 19 | `src/backend/bir/passes/README.md` | pass | Shared B1-B8 candidate/pass framework; partial foundation. |
| 20 | `src/backend/bir/passes/aggregate/README.md` | pass | B6/P06 aggregate canonicalization; absent. |
| 21 | `src/backend/bir/passes/call_lowering/README.md` | pass | D2 shared ABI-aware call lowering; unimplemented design. |
| 22 | `src/backend/bir/passes/cfg/README.md` | pass | B3/P03 CFG canonicalization; absent. |
| 23 | `src/backend/bir/passes/intrinsics/README.md` | pass | B7/P07 target-independent intrinsic canonicalization; absent. |
| 24 | `src/backend/bir/passes/legalize/README.md` | pass | B1/P01 Raw admission and type/op legalization; absent. |
| 25 | `src/backend/bir/passes/memory/README.md` | pass | B5/P05 memory canonicalization; absent. |
| 26 | `src/backend/bir/passes/out_of_ssa/README.md` | pass | D5 out-of-SSA/copy closure architecture; implementation deferred. |
| 27 | `src/backend/bir/passes/pseudo_lowering/README.md` | pass | D1 generic pseudo lowering; unimplemented design. |
| 28 | `src/backend/bir/passes/scalar/README.md` | pass | B2/P02 scalar normalization; absent. |
| 29 | `src/backend/bir/passes/ssa/README.md` | pass | B4/P04 dynamic SSA construction/proof; absent. |
| 30 | `src/backend/bir/passes/target/README.md` | pass | D4 target pseudo expansion/legalization; unimplemented deferred extension. |
| 31 | `src/backend/bir/pipeline/README.md` | phase | Ordered B1-B8 orchestration, checkpoint, B8 publication; partial foundation. |
| 32 | `src/backend/bir/preparation/README.md` | phase | Common immutable C-product order/key/publication contract; unimplemented. |
| 33 | `src/backend/bir/preparation/abi/README.md` | pass | C3 immutable ABI plan; absent. |
| 34 | `src/backend/bir/preparation/address/README.md` | pass | C6 immutable address plan; absent. |
| 35 | `src/backend/bir/preparation/calls/README.md` | pass | C4 immutable call plan; absent. |
| 36 | `src/backend/bir/preparation/inline_asm/README.md` | pass | C7 inline-assembly target context; unimplemented design. |
| 37 | `src/backend/bir/preparation/runtime_helpers/README.md` | pass | C8 runtime-helper plan; unimplemented design. |
| 38 | `src/backend/bir/preparation/variadic/README.md` | pass | C5 immutable variadic plan; absent. |
| 39 | `src/backend/bir/pseudo/README.md` | schema | Prospective D-E pseudo vocabulary/schema; unimplemented and not part of the current 16-kind production registry. |
| 40 | `src/backend/bir/regalloc/README.md` | pass | E2 allocation authority; unimplemented design. |
| 41 | `src/backend/bir/regalloc/constraints/README.md` | pass | C9 constraint binding and later projection authority; unimplemented design. |
| 42 | `src/backend/bir/regalloc/spill_reload/README.md` | pass | E3 spill/reload rewrite and retry; unimplemented design. |
| 43 | `src/backend/bir/target_layout/README.md` | pass | C2 verified target-layout product; absent. |
| 44 | `src/backend/bir/verify/README.md` | verifier | Raw implementation plus prospective Canonical/Prepared-input/Pseudo/Allocated gates; mixed implemented/design status. |

## Missing or shared pass owners

The current tree has semantic coverage for every ordered row, but not every row
has a standalone Markdown owner with the required 732 per-pass spine:

- C1 is intentionally external at `src/target_profile/README.md`; BIR root and
  verifier documents own only its binding seam.
- B8 is split across `passes/README.md`, `pipeline/README.md`, and
  `verify/README.md`; there is no single B8 contract owner.
- D3 is represented by `pseudo/README.md` and the verifier; there is no single
  D3 publication owner.
- E4 is concentrated in `allocated/README.md` with verifier support.
- F1-F3 are boundary references into MIR/emission documents outside
  `src/backend/bir`; there are no BIR-local standalone F1, F2, or F3 owners.

Step 2 must decide a single reference spine for shared publication rows. Later
phase steps may add narrowly named Markdown owners when the shared documents
cannot carry the required per-pass contract without duplicating authority.

## Drift and convergence findings

### Factual implementation-status drift

1. `lir_to_bir/README.md` says the build-included importer accepts only inline
   asm and a very small terminator subset. The current top-level importer is
   substantially broader: its landed history and source include typed scalar,
   memory, call, cast, compare, phi, authority, and all current terminator
   receipts. The status may remain `partial`, but its enumerated checked-in
   subset and `Last-Reconciled-Commit: none` are stale.
2. That document describes a `ModuleDraft -> verify_and_publish_raw` API. The
   landed API is `ModuleBuilder` followed by rvalue-qualified `publish()`, which
   runs `FoundationVerifier` and either mints `RawBir` or returns a structured
   publication failure. The contract intent is compatible; the named API is
   speculative/stale.
3. `core/README.md` still marks many containers and importer wires missing even
   though current `ir.hpp`, builders, importer history, and tests contain a
   larger bounded receiving surface. It correctly retains `partial` overall,
   but row-level dispositions require a fresh audit.
4. `verify/README.md` mixes a real Raw foundation gate with proposed later
   profiles in one status sentence. Each later gate must remain explicitly
   unimplemented until code evidence exists.
5. Most B-F documents correctly say `absent`, `unimplemented`, or
   `partial-foundation`. Their detailed type/API names are architectural
   contracts, not landed declarations.

### Duplicate or ambiguous authority

- The NodeKind taxonomy must have one authority: the normative document plus
  its production registry. `core/README.md` may explain the landed surface;
  phase and pseudo documents must not restate independently evolving tag or
  admission tables.
- Root, review-template, pipeline, and verifier documents repeat the A-F order
  and publication story. Step 2 must make root order, shared transaction rules,
  and verifier gates references rather than four subtly different authorities.
- `PreparedBir` is used in E4 documents as a readiness capability, whereas
  the normative `Prepared` vocabulary describes C-phase admission/reference
  facts. Step 2 must name this distinction explicitly so agents do not infer a
  C-phase graph mutation or stage inheritance.
- The prospective pseudo schema lists kinds not present in the current
  production `NodeKind` enum. It is a planning vocabulary only until a separate
  implementation lands; pass matrices must label it accordingly.

### Stale sequencing and speculative language

- `lir_to_bir/README.md` still says umbrella 732 proceeds “after idea 734
  closes.” Idea 732 is active now and explicitly permits Steps 1-7 in parallel;
  only final Raw/B1 reconciliation waits for landed importer evidence.
- The ordered root sequence itself is already A, B, C, D, E, F and must not be
  reordered. The stale item is the lifecycle dependency wording, not phase
  order.
- Proposed class, token, product, fingerprint, and verifier names throughout
  later-stage documents must stay marked as design/unimplemented. Markdown
  spelling alone cannot be cited as an available API.

## Locally landed Raw publication facts

At the audited revision:

- branch: `bir_md`;
- `HEAD`: `d5cd3aa4e309b42c28be3b96151e992d025f7903`;
- build inclusion: `src/backend/CMakeLists.txt` includes top-level
  `src/backend/bir/lir_to_bir.cpp` and excludes nested
  `src/backend/bir/lir_to_bir/*.cpp`;
- publication: `lower_lir_to_raw_bir` validates the module/functions, builds
  through one private `ModuleBuilder`, and consumes it with `publish()`;
- gate: `ModuleBuilder::publish()` rejects active edits/reuse, runs
  `FoundationVerifier`, establishes pipeline identity/stamp state, and returns
  a move-only `RawBir` only on success;
- failure: the importer returns no `RawBir` and carries `PublishError` plus
  verifier diagnostics;
- canonical convenience: `lower_lir_to_canonical_bir` consumes the Raw result
  through current `canonicalize`, but this foundation is not evidence that the
  planned B1-B8 pass contracts are implemented.

No uncommitted importer change is visible in this worktree. The locally known
`origin/new_bir` ref is an ancestor of this branch, so it supplies no newer
not-yet-landed importer evidence here. User-reported parallel importer work is
therefore intentionally not described. Its eventual commit and changed
behavior remain Step 8 inputs.

## Raw facts deferred to Step 8

Step 8 must re-run the audit against the actual landed importer revision and
must not copy expected behavior from this baseline. It must enumerate:

- the complete then-current `NodeKind` set actually emitted by A1;
- every accepted payload alternative and exact operand/result/type role;
- constants, globals, declarations, functions, blocks, terminators, CFG edges,
  source-origin/stable-ID mapping, metadata, and ordering behavior;
- the actual private construction type, full Raw verifier, atomic publication
  call, rollback behavior, and public capability;
- every accepted and rejected LIR alternative, with tests as evidence;
- target/profile/layout facts proven absent from Raw; and
- the exact factual B1 input vocabulary and any mismatch with planned B1.

Until then, current importer observations are a dated factual baseline, not a
promise about the parallel route. Any required code correction discovered by
Step 8 becomes a separate implementation idea.

## Step 2 handoff

Step 2 should establish one root/common reference spine that:

1. cites the normative NodeKind/tag document instead of duplicating it;
2. distinguishes static stage admission, B4 dynamic SSA, exact-revision
   products, E4 readiness capabilities, and graph publication;
3. makes root order, pass transactions, verifier gates, identity/provenance,
   invalidation, unknown-kind failure, and implementation-status vocabulary
   single-owner rules; and
4. corrects stale common claims without performing the Step 8 importer-final
   reconciliation early.
