Status: Active
Source Idea Path: ideas/open/prealloc-responsibility-map-and-layout-plan.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Audit Review

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: final audit review for the prealloc
responsibility map and layout planning runbook.

The active source idea acceptance criteria are satisfied:

- `src/backend/prealloc` has a documented responsibility map and stable package
  model.
- The audit produced a coherent internal layout model without relying on an
  external reference model.
- Focused follow-up ideas exist for the highest-value cleanup slices.
- No broad code movement, implementation behavior change, test expectation
  change, or semantic migration was performed during this audit runbook.

### Final Responsibility Map Summary

The Step 1 inventory covered all recursive `.cpp` and `.hpp` files under
`src/backend/prealloc`: 109 files, 31,635 lines, 60 `.cpp`, and 49 `.hpp`.

The final package model keeps `prealloc` as the target-parameterized
preparation layer between semantic BIR and target MIR/codegen. It may compute
stable facts, plans, identities, storage decisions, ABI plans, and target
profile inputs consumed by later stages. It should not own final target
instruction selection, target register spelling, or assembly emission.

Durable families recorded by the audit:

| Proposed family | Durable owner | Contract decision |
| --- | --- | --- |
| Pipeline and prepared-module aggregate | top-level phase coordinator and aggregate prepared-module surface | Keep `module.hpp` as the aggregate prepared-module contract until smaller package APIs are stable. |
| Control-flow preparation | neutral CFG normalization, label identity, out-of-SSA, join/branch movement facts | Keep `control_flow.hpp` broad for route, branch, join, move, and invariant facts; defer diagnostics/route-history contraction. |
| Liveness and allocation planning | liveness, interval planning, value homes, placement identities, spill/reload, call-boundary move records | Keep `regalloc.hpp` as the public allocation-plan contract; split helper implementation phases first. |
| Stack and frame planning | frame plan, stack object collection, slot assignment, coalescing, inline-asm stack facts, address planning | Keep `frame.hpp` and `frame_plan.hpp` public; move stack-layout declarations only in a focused follow-up. |
| Call and return ABI plans | call argument/result homes, preservation, clobbers, indirect callees, memory returns, formals, boundary effects | Keep `calls.hpp` aggregate; split `call_plans.cpp` phases before any header split. |
| Publication and prepared accessors | codegen-facing publication plans, lookups, decoded homes, storage/value-location views, address models | Treat as a first-class bridge package, not leftover helpers. |
| Runtime and special carriers | i128/f128/atomics/intrinsics/inline-asm carrier discovery, helper calls, marshal plans, helper metadata | Keep runtime-helper contracts separate from special-carrier contracts. |
| Dynamic stack and variadic entry | dynamic-stack allocation facts and variadic entry resource/home plans | Keep separate from generic call plans because consumption happens at different phase points. |
| Target register profile facts | centralized target register pools, ABI register facts, profile queries | Preserve as the concrete target-register fact owner in prealloc. |
| Prepared-printer support | debug dump entry points, shared print helpers, data-family mirrors | Printers follow data-family movement; they should not drive package splits. |

Aggregate contracts intentionally retained by the audit:

- `module.hpp`
- `control_flow.hpp`
- `regalloc.hpp`
- `calls.hpp`
- `runtime_helpers.hpp`
- `special_carriers.hpp`

Future contraction candidates were captured as follow-up ideas or watch points
rather than being implemented in this audit.

### Generated Follow-Up Ideas

Recommended execution order:

1. `ideas/open/prealloc-publication-accessor-contracts.md`
   - Slice type: header/data-contract contraction plus prepared-printer
     alignment.
   - Durable owner: publication and prepared accessors.
   - Closure role: covers the P1 publication/accessor package around
     `publication_plans.*`, `prepared_lookups.*`,
     `decoded_home_storage.*`, `storage_plans.*`, and
     `value_locations.hpp`.
2. `ideas/open/prealloc-call-plan-phase-split.md`
   - Slice type: `.cpp` phase split with optional prepared-printer grouping
     alignment.
   - Durable owner: call and return ABI plans.
   - Closure role: covers the P1 `call_plans.cpp` durable-subphase split while
     preserving `calls.hpp` as the aggregate contract unless a smaller
     contract is proven.
3. `ideas/open/prealloc-stack-layout-contract-boundary.md`
   - Slice type: header/data-contract boundary cleanup plus focused `.cpp`
     phase split.
   - Durable owner: stack and frame planning.
   - Closure role: covers stack-layout-facing declarations currently exposed
     through `module.hpp` and coordinator internals along object collection,
     slot assignment, orchestration, and address-publication boundaries.
4. `ideas/open/prealloc-regalloc-coordinator-contraction.md`
   - Slice type: `.cpp` phase contraction and helper relocation or renaming.
   - Durable owner: liveness and allocation planning.
   - Closure role: covers `regalloc.cpp` contraction along existing
     `regalloc/` helper-family boundaries while keeping `regalloc.hpp`
     aggregate.
5. `ideas/open/prealloc-runtime-carrier-naming-alignment.md`
   - Slice type: helper/comment naming repair with prepared-printer alignment.
   - Durable owner: runtime and special carriers.
   - Closure role: covers runtime-helper and special-carrier naming alignment
     without merging the two aggregate concepts.

Each generated idea includes target files, slice type, durable owner, in-scope
and out-of-scope work, expected behavior-preservation proof, acceptance
criteria, and reviewer reject signals. The generated ideas are small enough for
focused follow-up runs and avoid creating one broad prealloc refactor.

### Unresolved Ownership Questions

No unresolved ownership question remains only in chat.

Captured follow-up work:

- Publication/accessor bridge contraction is captured by
  `prealloc-publication-accessor-contracts`.
- `call_plans.cpp` phase extraction is captured by
  `prealloc-call-plan-phase-split`.
- Stack-layout contract relocation and coordinator splitting are captured by
  `prealloc-stack-layout-contract-boundary`.
- `regalloc.cpp` coordinator contraction is captured by
  `prealloc-regalloc-coordinator-contraction`.
- Runtime-helper and special-carrier naming drift is captured by
  `prealloc-runtime-carrier-naming-alignment`.

Captured watch points that are not confirmed layout cleanup slices:

- AArch64/AAPCS64-specific variadic entry planning in
  `variadic_entry_plans.cpp`.
- Architecture branches in `regalloc/call_return_abi.cpp`.
- AArch64 feature validation in `intrinsics.cpp`.
- Fallback size/address paths in `stack_layout/coordinator.cpp`.

These watch points should become separate source ideas only if later
symbol-level review proves ownership drift. They are not blockers for closing
the active responsibility-map source idea.

### Closure Recommendation

Recommend asking the plan owner to close
`ideas/open/prealloc-responsibility-map-and-layout-plan.md`.

Rationale: the active idea was explicitly a planning and discovery idea. Its
required outputs now exist in canonical lifecycle state and generated
follow-up source ideas. The audit did not leave broad implementation movement
half-finished, did not hide semantic changes inside layout work, and did not
perform target-emission migration.

## Suggested Next

Ask the plan owner to close
`ideas/open/prealloc-responsibility-map-and-layout-plan.md` and clear the active
`plan.md` / `todo.md` lifecycle state if the plan owner accepts this closure
recommendation.

## Watchouts

- Do not treat the five generated follow-up ideas as already implemented; they
  are independent future cleanup slices.
- Preserve the aggregate contracts listed above until a focused follow-up proves
  a smaller consumer-facing boundary.
- Printer work should mirror data-family changes in the same slice and should
  not invent ownership movement on its own.
- Treat semantic watch points as separate initiatives only after symbol-level
  proof; none are confirmed migration candidates from this audit.

## Proof

Delegated proof: `git diff --check`.

Result: passed.

This was an audit-only final review. No build or test subset was required by
the delegated packet, and no implementation files, tests, generated follow-up
ideas, `plan.md`, or the active source idea were changed.
