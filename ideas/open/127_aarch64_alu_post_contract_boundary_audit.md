# 127 AArch64 ALU Post-Contract Boundary Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/alu.cpp` after the recent prepared,
publication, dispatch, comparison, calls, and memory contract work, then
produce focused follow-up ideas for any remaining ALU-side boundary gaps.

This idea is analysis-only. It should not directly edit implementation files.

## Why This Exists

`alu.cpp` remains the third-largest AArch64 codegen owner at roughly 4.3k
lines. Earlier scalar work repaired duplicate prepared authority in ALU and
related scalar owners:

- idea 51 repaired ALU prepared value-home, storage, memory-access,
  scalar-publication, return, and select-chain authority consumption;
- idea 55 repaired scalar cast plus ALU publication authority around
  pointer-derived values;
- idea 71 clarified scalar/control-flow prepared authority across ALU, casts,
  and comparison;
- idea 74 folded repeated local scalar register-view helpers;
- ideas 116, 117, 122, and 123 later added shared producer/publication and
  current-block prepared contract surfaces that ALU may now be able to consume
  more directly.

The remaining question is not "how do we shrink `alu.cpp` by line count?" It
is whether ALU still mixes target-neutral scalar producer/publication authority
with AArch64 scalar instruction emission. If shared authority remains, it
should move forward before mechanical owner splitting. If the boundary is
clean, follow-up work should be local clarity only or explicitly record
`no-new-idea`.

## In Scope

Audit the current ALU owner using these standards:

1. Prepared scalar operand and result homes
   - Verify ALU consumes prepared value-home/storage/regalloc facts for scalar
     operands and results rather than doing value-name or raw storage recovery.
   - Keep AArch64 register view, scratch choice, and operand spelling local.

2. Scalar producer and publication consumption
   - Check whether same-block scalar producer, load-local producer,
     current-block entry publication, stack publication, and select-chain
     dependency decisions consume shared prepared facts from closed routes.
   - Identify any ALU-local producer/materializability decision that should
     move to shared prepared/prealloc.

3. Immediate and constant materialization policy
   - Separate target-neutral immediate semantics from AArch64 immediate
     encodability, reduction, scratch materialization, and assembler spelling.
   - Decide whether any constant-materialization helper should be shared or is
     properly AArch64-local.

4. Control-source and fallback operand materialization
   - Audit fallback operand selection, control publication operands, return ABI
     retargeting, and materialized control source helpers.
   - Decide whether they are now pure consumers of prepared facts or still
     rebuild producer/control-flow authority.

5. Physical split readiness
   - If the above boundaries are clean, decide whether a local ALU owner
     surface should be split for clarity.
   - Treat physical splitting as mechanical clarity, not semantic progress.

## Relationship To Prior Work

Use these closed ideas as required context:

- idea 51: ALU prepared authority repair
- idea 55: scalar cast/ALU publication prepared authority repair
- idea 71: scalar/control-flow prepared authority cleanup
- idea 74: local scalar register helper fold-back
- idea 116: dispatch prepared producer contract surface
- idea 117: comparison fused-compare/current-block publication contract
- idea 122: call argument producer materializability contract
- idea 123: call result late-publication contract

Do not duplicate those routes. If one audit standard is already fully covered,
the closure note should explicitly say `no new idea` for that standard and
cite the closed evidence.

## Expected Outputs

The closure note should include:

- a classification table for the five audit standards;
- a helper/cluster map for current large `alu.cpp` regions;
- explicit `no new idea` entries for standards already covered by closed work;
- follow-up `ideas/open/` files only for concrete unresolved gaps.

Good follow-up idea shapes include:

- a shared prepared query/fact idea if ALU still owns target-neutral scalar
  producer/materializability/publication authority;
- a contract-visibility idea if cleanup is blocked by missing dump or route
  evidence;
- a local AArch64 ALU owner extraction idea if the boundary is clean and the
  split improves private owner clarity;
- a no-op closure note when current evidence proves `alu.cpp` is large but
  correctly target-local.

## Out Of Scope

- Direct implementation edits in `alu.cpp`, `alu.hpp`, tests, or build
  metadata.
- Reopening already closed scalar prepared-authority, cast/ALU publication,
  scalar control-flow, register-view fold-back, dispatch producer, comparison
  publication, or call publication-contract routes without new evidence.
- Moving AArch64 opcode choice, immediate encodability, scratch register
  policy, extension/mul/div/rem instruction spelling, or machine-record
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
  scalar ALU emission for each audit standard.
- No follow-up duplicates ideas 51, 55, 71, 74, 116, 117, 122, or 123.

## Reviewer Reject Signals

- The route proposes one monolithic `alu.cpp` shrink implementation.
- It treats a large helper cluster as wrong without identifying whether the
  responsibility is shared authority, target emission, or review visibility.
- It creates vague follow-up ideas without concrete files or proof routes.
- It reopens closed scalar prepared-authority or publication routes without
  new evidence.
- It proposes moving AArch64-specific opcode, immediate, scratch, extension, or
  machine-record emission policy into shared code.
- It claims progress from physical file movement while the same mixed authority
  remains hidden behind new helper names.
