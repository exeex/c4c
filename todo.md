Status: Active
Source Idea Path: ideas/open/prealloc-responsibility-map-and-layout-plan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Propose The Stable Package Model

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: turned the Step 1/2 inventory and
responsibility map into a proposed durable package model for
`src/backend/prealloc`, separated aggregate contracts from future contraction
candidates, and grouped cleanup candidates by slice type. No implementation
files, tests, `plan.md`, or source idea files were changed.

### Proposed Stable Package Model

`prealloc` should remain the target-parameterized preparation layer between
semantic BIR and target MIR/codegen. Its internal layout should make five
boundaries visible:

1. stable prepared-data contracts consumed by codegen and printers
2. semantic planning phases that build or mutate prepared facts
3. publication/accessor packages that translate prepared data for later
   consumers
4. target-profile facts that are concrete but non-emitting
5. prepared-printer mirrors that follow data-family ownership

The package model below is proposed as the durable ownership map for follow-up
layout ideas. File paths are relative to `src/backend/prealloc/`.

| Proposed family | Durable owner | Files | Contract rule |
| --- | --- | --- | --- |
| Pipeline and prepared-module aggregate | top-level phase coordinator and aggregate prepared-module surface | `prealloc.cpp`, `prealloc.hpp`, `legalize.cpp`, `module.hpp`, `names.hpp` | Keep `module.hpp` as the aggregate read/write prepared-module contract until consumers no longer need one top-level view. Do not split it merely because it includes many families. |
| Control-flow preparation | neutral CFG normalization, label identity, out-of-SSA, join/branch movement facts | `control_flow.hpp`, `out_of_ssa.cpp`, `label_identity.cpp`, `label_identity.hpp` | Keep one control-flow contract for route, branch, join, parallel-copy, and invariant facts, but mark diagnostics/route-history comments as candidates for later contraction. |
| Liveness and allocation planning | liveness, value classification, interval planning, value homes, placement identities, spill/reload, stack slots, and call-boundary move records | `liveness.*`, `regalloc.cpp`, `regalloc.hpp`, `regalloc/*`, `regalloc_placement_identity.*` | Keep `regalloc.hpp` as the public allocation-plan contract for now. Future cleanup should split helper implementation phases before splitting the aggregate header. |
| Stack and frame planning | frame plan, stack object collection, slot assignment, coalescing, inline-asm stack facts, and address-materialization planning | `frame.hpp`, `frame_plan.*`, `stack_layout/*` | Keep `frame.hpp`/`frame_plan.hpp` as public contracts. Move stack-layout declarations now hosted by `module.hpp` only after a focused data-contract split proves consumer clarity. |
| Call and return ABI plans | call argument/result homes, call preservation, clobbers, indirect callee plans, memory returns, formals, and boundary effects | `calls.hpp`, `call_plans.*`, `formal_publications.*` | Keep `calls.hpp` as the aggregate call ABI contract. Split implementation phases from `call_plans.cpp` before attempting a header split. |
| Publication and prepared accessors | codegen-facing publication plans, lookups, decoded homes, storage/value-location views, and address models | `addressing.hpp`, `decoded_home_storage.*`, `prepared_lookups.*`, `publication_plans.*`, `storage.hpp`, `storage_plans.*`, `value_locations.hpp` | Treat this as a first-class package, not as leftover helpers. Publication/accessor contracts bridge families and should be reviewed as API boundaries, not line-count cleanup. |
| Runtime and special carriers | i128/f128/atomics/intrinsics/inline-asm carrier discovery, helper calls, marshal plans, and runtime helper metadata | `runtime_helpers.hpp`, `special_carriers.*`, `i128_runtime_helpers.*`, `f128_runtime_helpers.*`, `atomics.*`, `intrinsics.*`, `inline_asm.*` | Keep runtime-helper and special-carrier contracts separate. Runtime helpers describe helper call resources; special carriers describe value-carrier requirements across special operations. |
| Dynamic stack and variadic entry | dynamic-stack allocation facts and variadic entry resource/home plans | `dynamic_stack.*`, `variadic.hpp`, `variadic_entry_plans.*` | Keep separate from generic call plans because entry ABI facts and dynamic-stack facts are consumed at different phase points. Target-specific ABI details must stay parameterized facts, not codegen emission. |
| Target register profile facts | centralized target register pools, ABI register facts, and profile queries | `target_register_profile.*` | Preserve as the only concrete target-register fact owner in prealloc. New architecture tables elsewhere should be rejected unless they are local decode/print views over this contract. |
| Prepared-printer support | debug dump entry points, shared print helpers, and mirrors for prepared data families | `prepared_printer.*`, `prepared_printer/*` | Printers follow data-family movement. They should not drive package splits, but every durable data-family move should update the matching printer mirror in the same follow-up idea. |

### Aggregate Contracts Versus Split Candidates

| Header or contract | Step 3 decision | Reasoning | Future movement trigger |
| --- | --- | --- | --- |
| `module.hpp` | Keep as aggregate contract. | It is the prepared-module surface and shared lookup host across phases and printers. Splitting it first would make consumers include many small headers before stable package APIs exist. | Split only after publication/accessor and stack-layout ownership have stable smaller contracts. |
| `control_flow.hpp` | Keep broad, consider contraction later. | Route names, branch/join facts, parallel copies, label metadata, and invariants are one control-flow data model. | Split diagnostics/route-history helpers if they become independent of the authoritative CFG facts. |
| `regalloc.hpp` | Keep as aggregate allocation-plan contract. | It publishes allocation state and ABI-aware move plans consumed across prealloc and codegen. | Split only after helper implementation packages stop needing private aggregate state. |
| `calls.hpp` | Keep as aggregate call ABI contract. | Argument/result storage, clobbers, preservation, indirect callee, memory returns, and boundary effects must be reasoned about together. | Split after `call_plans.cpp` phase extraction clarifies which subcontracts are independently consumed. |
| `runtime_helpers.hpp` | Keep as runtime-helper aggregate. | Helper call resources, marshal direction, ownership, and value homes form one helper-call contract. | Consider contraction only if i128/f128 helper-call APIs diverge from atomics/intrinsics helper APIs. |
| `special_carriers.hpp` | Keep separate from runtime helpers. | Carrier facts for i128/f128/atomics/intrinsics/inline asm are related but not identical to helper-call resources. | Split carrier subfamilies only when callers consume one carrier kind without the rest. |
| `value_locations.hpp` | Candidate for future bridge contraction. | It bridges regalloc homes and move bundles with compatibility comments that likely reflect consumer migration. | Contract after consumers converge on publication/accessor views or regalloc-home views. |
| `publication_plans.hpp` / `prepared_lookups.hpp` / `decoded_home_storage.hpp` | Keep as a named publication/accessor package. | These cross family boundaries deliberately and are the main codegen-facing prepared-data adapters. | Split by consumer API only if a follow-up proves independent call sites and no duplicated decode logic. |
| `frame.hpp` / `frame_plan.hpp` / `stack_layout/stack_layout.hpp` | Keep public frame/stack planning contracts; inspect module-hosted declarations later. | Stack layout is a durable phase family, but some declarations live outside its directory. | Move declarations under stack-layout ownership if consumers do not need the full module aggregate. |

### Prioritized Cleanup Candidates

#### Header and Data-Contract Splits

| Priority | Candidate | Slice type | Durable owner | Reviewer reject signals |
| --- | --- | --- | --- | --- |
| P1 | Name and document the publication/accessor package around `publication_plans.*`, `prepared_lookups.*`, and `decoded_home_storage.*`; optionally contract compatibility comments in `value_locations.hpp` if consumers already use the new views. | header/data-contract split or contraction | Publication and prepared accessors | Reject if it duplicates decode/home logic, forces broad codegen include churn, or changes publication semantics under a layout label. |
| P1 | Extract stack-layout-facing declarations currently exposed through `module.hpp` into a stack-layout-owned public contract only if call sites can include that contract directly. | header/data-contract split | Stack and frame planning | Reject if `module.hpp` remains the real dependency and the new header is just a forwarding shell. |
| P2 | Contract `control_flow.hpp` by separating durable CFG/move facts from route-history or invariant diagnostic helpers. | header/data-contract contraction | Control-flow preparation | Reject if branch/join/source-of-truth facts are split into multiple inconsistent contracts. |
| P2 | Split `calls.hpp` only after implementation extraction proves stable subcontracts for boundary effects, preservation, and indirect callee/memory-return plans. | header/data-contract split | Call and return ABI plans | Reject if targets must include many tiny call headers to build one call plan. |
| P3 | Review `runtime_helpers.hpp` and `special_carriers.hpp` for naming consistency while preserving two aggregate contracts. | header/data-contract contraction | Runtime and special carriers | Reject if helper-call resources and carrier facts are merged into one less precise mega-contract. |

#### `.cpp` Phase Splits Or Merges

| Priority | Candidate | Slice type | Durable owner | Reviewer reject signals |
| --- | --- | --- | --- | --- |
| P1 | Split `call_plans.cpp` by durable subphase: direct/indirect call argument plans, preservation/clobber derivation, boundary effects, and memory-return handling. | `.cpp` phase split | Call and return ABI plans | Reject if extraction is line-count-only, hides behavior changes, or moves ABI policy away from `TargetProfile`/call contracts. |
| P1 | Split `stack_layout/coordinator.cpp` into orchestration, object collection, slot assignment orchestration, and address-materialization publication helpers. | `.cpp` phase split | Stack and frame planning | Reject if fallback address facts become target-emission logic or duplicate stack object authority. |
| P1 | Split `regalloc.cpp` coordinator internals only along existing helper-family boundaries already present under `regalloc/`. | `.cpp` phase split | Liveness and allocation planning | Reject if extracted helpers require broad access to private mutable coordinator state without a clearer API. |
| P2 | Review `i128_runtime_helpers.cpp` and `f128_runtime_helpers.cpp` for shared helper-plan extraction versus deliberate family separation. | `.cpp` phase merge/split assessment | Runtime and special carriers | Reject if shared code blurs i128/f128 ABI differences or creates generic helper dispatch with target-shaped shortcuts. |
| P2 | Review `out_of_ssa.cpp` for extraction of parallel-copy or join-transfer helpers only if they map directly to `control_flow.hpp` data families. | `.cpp` phase split | Control-flow preparation | Reject if extraction creates another control-flow source of truth. |
| P3 | Keep `liveness.cpp` intact unless future symbol review finds reusable interval helpers not already covered by `regalloc/intervals.*`. | no-op or small extraction | Liveness and allocation planning | Reject speculative extraction without repeated consumers. |

#### Helper Relocation Or Renaming

| Priority | Candidate | Slice type | Durable owner | Reviewer reject signals |
| --- | --- | --- | --- | --- |
| P1 | Move or rename stack-layout helper declarations now exposed from `module.hpp` so their names advertise stack-layout ownership. | helper relocation/renaming | Stack and frame planning | Reject if relocation requires broad unrelated include rewrites or changes prepared-module ownership. |
| P1 | Rename bridge/compatibility comments in `value_locations.hpp` and `prepared_printer/calls.cpp` into explicit package-transition notes, or remove them if no longer true. | helper/comment naming repair | Publication/accessor and printer mirrors | Reject if comments are deleted without proving the compatibility boundary is gone. |
| P2 | Audit architecture-named helper functions in `regalloc/call_return_abi.cpp` and `variadic_entry_plans.cpp` for naming that states "ABI fact planning" rather than target emission. | helper renaming | Allocation planning and variadic entry | Reject if renaming masks concrete target-specific policy that belongs in `target_register_profile.*`. |
| P2 | Align runtime helper and special carrier helper names so i128/f128/atomics/intrinsics/inline-asm ownership is visible from file and function names. | helper renaming | Runtime and special carriers | Reject if naming collapses distinct helper-call and carrier concepts. |

#### Prepared-Printer Alignment

| Priority | Candidate | Slice type | Durable owner | Reviewer reject signals |
| --- | --- | --- | --- | --- |
| P1 | For any publication/accessor package split, update `prepared_printer/value_locations.cpp`, `prepared_printer/storage.cpp`, and related shared helpers to mirror the new data-family names. | prepared-printer alignment | Prepared-printer support | Reject printer-only movement that precedes or invents a data-contract split. |
| P1 | If call-plan phases are split, align `prepared_printer/calls.cpp` labels and helper grouping with the new call subcontracts. | prepared-printer alignment | Prepared-printer support | Reject if printed output changes meaning or drops fields under a layout cleanup label. |
| P2 | If runtime helper/carrier naming is repaired, align `prepared_printer/runtime_helpers.cpp` and `prepared_printer/special_carriers.cpp` in the same slice. | prepared-printer alignment | Prepared-printer support | Reject if printer files become a second source of ownership taxonomy not present in data files. |
| P3 | Keep `prepared_printer/private.hpp` importing aggregate module state until the data contracts move. | no-op guardrail | Prepared-printer support | Reject isolated private-printer include churn without data-family movement. |

#### Semantic Migration Candidates

No confirmed semantic migration candidate should be mixed into layout cleanup
from the Step 2 map. The following are watch points only and should become
separate source ideas if later symbol-level review proves ownership drift:

| Watch point | Why it is not a layout cleanup slice yet | Reviewer reject signals |
| --- | --- | --- |
| AArch64/AAPCS64-specific variadic entry planning in `variadic_entry_plans.cpp` | It appears to be target-parameterized ABI planning, which is valid prealloc ownership. | Reject any follow-up that moves final instruction selection, register spelling, or codegen emission into prealloc. |
| Architecture branches in `regalloc/call_return_abi.cpp` | They describe ABI return/register facts and may belong near target register profile data, but Step 2 did not prove duplication or wrong ownership. | Reject if the slice creates testcase-shaped target branches or bypasses `target_register_profile.*`. |
| AArch64 feature validation in `intrinsics.cpp` | Feature checks may be semantic validation for intrinsic carrier planning, not layout drift. | Reject if a layout idea changes supported/unsupported intrinsic behavior. |
| Fallback size/address paths in `stack_layout/coordinator.cpp` | These may be conservative stack-layout facts, not target emission. | Reject if cleanup weakens stack object authority or hides runtime address semantics. |

### Recommended Step 4 Handoff

Create focused follow-up ideas in this order:

1. `prealloc-publication-accessor-contracts`: define the publication/accessor
   package around `publication_plans.*`, `prepared_lookups.*`,
   `decoded_home_storage.*`, `storage_plans.*`, and `value_locations.hpp`;
   include matching prepared-printer alignment only where data-family names
   change.
2. `prealloc-call-plan-phase-split`: split `call_plans.cpp` into durable
   call-plan subphases without changing call ABI behavior; keep `calls.hpp`
   aggregate unless extraction proves a stable contract split.
3. `prealloc-stack-layout-contract-boundary`: move or document
   stack-layout-facing declarations currently exposed through `module.hpp` and
   split `stack_layout/coordinator.cpp` only along object collection,
   orchestration, slot assignment, and address-publication boundaries.
4. `prealloc-regalloc-coordinator-contraction`: contract `regalloc.cpp` and
   related helper names along existing `regalloc/` package boundaries, leaving
   `regalloc.hpp` aggregate until consumers prove smaller contracts.
5. `prealloc-runtime-carrier-naming-alignment`: repair naming/comment drift
   between runtime helper contracts, special carrier contracts, and their
   prepared-printer mirrors without merging the two aggregate concepts.

Each Step 4 idea should include target files, slice type, durable owner,
behavior-preservation proof, out-of-scope semantic changes, and the reject
signals from the table above. Do not create one giant prealloc refactor idea.

## Suggested Next

Execute Step 4 from `plan.md`: create separate focused follow-up ideas under
`ideas/open/` for the highest-value candidates above. Start with the P1
publication/accessor, call-plan phase split, and stack-layout contract boundary
ideas. Keep each idea small enough for one focused run and include concrete
reviewer reject signals.

## Watchouts

- Step 4 should generate source ideas, not perform implementation movement.
- Preserve `module.hpp`, `control_flow.hpp`, `regalloc.hpp`, `calls.hpp`,
  `runtime_helpers.hpp`, and `special_carriers.hpp` as aggregate contracts
  unless a focused follow-up proves a consumer-facing split.
- Printer work must follow data-family changes and should not lead them.
- Treat semantic watch points as separate initiatives only after symbol-level
  review proves ownership drift; none are confirmed migration candidates from
  this audit packet.

## Proof

Delegated proof: `git diff --check`. This audit-only packet has no build or
test subset. Result: passed.
