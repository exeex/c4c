# 120 AArch64 Calls After-Call Result Value Local Owner

## Goal

Extract a bounded AArch64-local owner for after-call result and value lowering
that consumes prepared result plans, after-call ABI bindings, destination
prepared homes, and f128 carrier facts.

## Why This Exists

Idea 118 classified after-call result/value lowering as `ready-local-owner`.
The safe boundary is target-local result-register/view conversion plus
call-boundary move or frame-slot store record construction. Shared planning
already owns destination homes and f128 carriers, so this follow-up should
contract local emission without reopening those facts.

## In Scope

- AArch64-local after-call lowering around `lower_after_call_move`.
- Conversion from prepared result lanes and after-call ABI bindings into
  AArch64 GP, FP, VREG, and f128 register views.
- Construction of call-boundary move records or frame-slot store records for
  result values.
- Consumption of prepared destination value homes and prepared f128 carrier
  facts.

## Out Of Scope

- Recomputing result destination homes or stack frame slots already covered by
  idea 93.
- Reopening f128 carrier selection covered by idea 94.
- Moving ABI result-register spelling, scalar/FPR/VREG view selection, or
  frame-slot memory operand spelling into shared BIR/prealloc.
- Changing call result planning semantics or shared prepared result
  classification beyond narrow consumer needs.
- Broad cleanup of unrelated call instruction or ABI record construction.

## Likely Files

- `src/backend/mir/aarch64/codegen/calls.cpp`
- A new or existing AArch64 calls result helper file if the owner is split from
  `calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp` or nearby local declarations, if
  needed
- Focused backend/AArch64 tests covering GP, FP, VREG/f128, and stack result
  paths

## Owner Boundary

Shared prepared code owns result classification, result lane bindings,
destination homes, and f128 carrier facts. The AArch64-local owner owns
target-register parsing, register-view conversion, memory operand spelling,
and selected call-boundary machine record construction after the call.

## Proof Route

- Characterize the current `lower_after_call_move` helper cluster and its
  prepared inputs.
- Run a focused backend/AArch64 subset that covers scalar integer results,
  scalar FP results, f128 or vector-carrier result movement, and stack/frame
  result storage.
- Add or tighten route-visible coverage only if the prepared result boundary is
  not already exposed by existing tests or dumps.
- Escalate to broader backend proof if shared prepared result structures are
  modified.

## Acceptance And Completion Criteria

- After-call result/value lowering has a bounded AArch64-local owner with a
  clear prepared-input interface.
- The owner consumes prepared result plans, ABI bindings, destination homes,
  and f128 carrier facts without duplicating closed stack or f128 ownership.
- AArch64 result register spelling, scalar/FPR/VREG view choice, q/f128
  rendering, and frame-slot memory spelling remain target-local.
- Proof covers adjacent after-call result shapes rather than a single named
  fixture.

## Reviewer Reject Signals

- The route rederives destination homes, stack frame slots, or memory-return
  storage instead of consuming idea 93 ownership.
- The route adds new f128 carrier completion or q-register lookup authority
  instead of consuming idea 94 ownership.
- Target-specific ABI result register spelling or register-view policy is
  moved into shared prepared code.
- Tests are weakened, supported paths are downgraded, or expected output is
  rewritten to avoid after-call result movement.
- The proof only covers one named result testcase while nearby GP, FP, f128, or
  frame-slot result paths remain unexamined.
- The old result/value lowering behavior remains in `calls.cpp` behind renamed
  helpers while the slice claims owner extraction progress.

## Closure Note

Closed after Step 5 acceptance review.

The accepted route extracts the after-call result movement body into the
AArch64-local `AfterCallMoveLocalOwner` surface. The wrapper still gathers the
prepared destination home, call-boundary classification, prepared f128 carrier
facts, and selected destination f128 carrier, then passes those facts into the
owner. The owner translates those prepared facts into target register views,
q/f128 rendering, memory operands, and call-boundary records without
recomputing destination homes, stack frame slots, ABI result classification, or
f128 carrier choices.

File splitting was deliberately deferred because moving the owner out of
`calls.cpp` would require exporting several private AArch64 helper surfaces
without reducing the owner scope.

The final reviewer report,
`review/idea120_after_call_owner_final_review.md`, found no implementation
drift or testcase overfit. Its only closure blocker was a missing
`test_after.log`; the close proof was regenerated with the exact focused
five-test backend/AArch64 command, and the regression guard passed with 5/5
tests before, 5/5 tests after, and no new failures.

Residual follow-up: ideas 121 through 123 remain open for separate
preservation/publication and shared prepared-materializability contracts.
