# 124 AArch64 Memory Post-Contract Boundary Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/memory.cpp` after the recent prepared,
publication, calls, dispatch, and comparison contract work, then produce
focused follow-up ideas for any remaining memory-side boundary gaps.

This idea is analysis-only. It should not directly edit implementation files.

## Why This Exists

`memory.cpp` remains the second-largest AArch64 codegen owner at roughly 5.2k
lines. Earlier memory work deliberately folded target-local store-source
emission residue back into the memory owner after moving shared store-source
facts forward. Other closed routes also clarified prepared memory authority,
frame-slot address materialization, store retargeting, prepared dump
visibility, call argument/result publication, and current-block publication
contracts.

The remaining question is not "how do we shrink `memory.cpp` by line count?"
It is whether `memory.cpp` still mixes shared prepared/BIR authority with
AArch64 memory emission. If shared authority remains, it should move forward
before any mechanical owner split. If the boundary is clean, follow-up work
should focus on local owner clarity or explicitly record that no new idea is
needed.

## In Scope

Audit the current memory owner using these standards:

1. Prepared memory record and identity validation
   - Decide whether `memory.cpp` still owns target-neutral consistency or
     identity decisions that should be shared prepared queries.
   - Keep AArch64 operand legality, addressing mode selection, and diagnostic
     spelling target-local.

2. Store-source and store-global publication consumption
   - Check whether store-source/store-global paths are only consuming prepared
     publication facts or still maintaining target-neutral publication state.
   - Include pending store-global stack-value publication consumers such as
     `lower_pending_store_global_stack_value_publications`.

3. Frame-slot and pointer-base materialization residue
   - Verify that frame-slot, pointer-base-plus-offset, fixed-slot, and
     prepared-address paths consume closed-route prepared authority instead of
     value-name recovery or local fallback selection.

4. Variadic `va_list` field memory handling
   - Separate shared variadic payload/home facts from AArch64 ABI-sensitive
     load/store shape, field address, register, and memory operand emission.

5. Physical split readiness
   - If the above boundaries are clean, decide whether local memory owner
     surfaces should be split for clarity.
   - Treat physical splitting as mechanical clarity, not semantic progress.

## Relationship To Prior Work

Use these closed ideas as required context:

- idea 34: store-source publication planning
- idea 39 / 39a: memory store-source fold-back after semantic residue cleanup
- idea 50: memory prepared authority repair
- idea 70: memory prepared address authority cleanup
- idea 86: memory owner subresponsibility audit
- idea 88: memory frame-slot address materialization owner
- idea 89: memory store retargeting owner
- idea 111: store-source publication dump visibility
- idea 114: prepared outgoing stack argument area contract
- ideas 116 / 117: shared current-block and producer publication contracts
- ideas 122 / 123: shared call argument/result producer and late-publication
  contracts

Do not duplicate those routes. If one of the five audit standards is already
fully covered, the closure note should explicitly say "no new idea" for that
standard and cite the closed evidence.

## Expected Outputs

The closure note should include:

- a classification table for the five audit standards;
- a function/helper cluster map for the current large `memory.cpp` regions;
- explicit "no new idea" entries for standards already covered by closed work;
- follow-up `ideas/open/` files only for concrete unresolved gaps.

Good follow-up idea shapes include:

- a shared prepared query/fact idea if memory still owns target-neutral memory
  identity, publication, or materialization authority;
- a contract-visibility idea if cleanup is blocked by missing dump or route
  evidence;
- a local AArch64 memory owner extraction idea if the boundary is clean and the
  split improves private owner clarity;
- a no-op closure note when current evidence proves `memory.cpp` is large but
  correctly target-local.

## Out Of Scope

- Direct implementation edits in `memory.cpp`, `memory.hpp`, tests, or build
  metadata.
- Reopening already closed store-source, frame-slot, store-retargeting,
  prepared-address, or publication-contract routes without new evidence.
- Moving AArch64 addressing legality, scratch/base register policy, load/store
  opcode choice, ABI-specific `va_list` field layout, or machine-record
  emission into shared BIR/prealloc.
- Treating line-count reduction as acceptance.
- Starting x86 or RISC-V codegen implementation.

## Acceptance And Completion Criteria

- The audit creates no implementation, test, or build metadata changes.
- Each generated follow-up idea names:
  - owner boundary;
  - likely files;
  - proof route;
  - testcase-overfit reject signals.
- The closure note distinguishes shared prepared/BIR authority from AArch64
  memory emission for each of the five standards.
- No follow-up duplicates ideas 34, 39, 39a, 50, 70, 86, 88, 89, 111, 114,
  116, 117, 122, or 123.

## Reviewer Reject Signals

- The route proposes one monolithic `memory.cpp` shrink implementation.
- It treats a large helper cluster as wrong without identifying whether the
  responsibility is shared authority, target emission, or review visibility.
- It creates vague follow-up ideas without concrete files or proof routes.
- It reopens closed memory/store-source/frame-slot/retargeting work without
  new evidence.
- It proposes moving AArch64-specific addressing, opcode, scratch, ABI, or
  machine-record emission policy into shared code.
- It claims progress from physical file movement while the same mixed authority
  remains hidden behind new helper names.
