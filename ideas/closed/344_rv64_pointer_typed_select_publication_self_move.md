# RV64 Pointer-Typed Select Publication Self-Move

## Goal

Remove the redundant `mv t0, t0` emitted by the RV64 backend route for
`backend_codegen_route_riscv64_pointer_typed_select_publication`, so the
full-suite broad proof can run without carrying a known publication failure.

## Why This Exists

The consteval inline asm template string runbook reached Step 6 broad proof, but
full CTest failed one unrelated test:
`backend_codegen_route_riscv64_pointer_typed_select_publication`. The failure
matches the unaccepted `test_baseline.new.log` state rather than the accepted
`test_baseline.log` state, so it cannot be treated as monotonic baseline
progress for closing that runbook.

This idea isolates the backend-route failure instead of expanding the inline asm
template string plan into unrelated codegen work.

## In Scope

- Reproduce and inspect the RV64 pointer-typed select publication codegen route.
- Identify why the generated assembly retains the self-move `mv t0, t0`.
- Repair the real lowering, register assignment, or backend cleanup path that
  should remove or avoid the self-move.
- Preserve the current public test contract that forbids `mv t0, t0`.
- Validate the focused backend publication test before rerunning broader proof.

## Out Of Scope

- Weakening, disabling, renaming, or reclassifying the publication test.
- Accepting `test_baseline.new.log` as proof without supervisor disposition.
- Adding a named-case filter that only deletes this exact test's text output.
- Changing the consteval inline asm template string implementation.
- Broad backend rewrites unrelated to the select/publication self-move path.

## Acceptance Criteria

- `backend_codegen_route_riscv64_pointer_typed_select_publication` passes
  without emitting `mv t0, t0`.
- Nearby RV64 backend route tests that cover select chains or publication
  output still pass.
- The fix is semantic enough to explain why self-moves are avoided or removed
  generally along the affected route.
- No forbidden-snippet expectation is downgraded.
- A broader CTest run can be used by the supervisor as fresh proof for the
  previously blocked consteval inline asm template string close review.

## Completion Note

Closed after `f28e8ae44 Skip RV64 edge publication self moves` repaired the
prepared edge-publication path by skipping same-register publication moves
while preserving real differing-register moves. Close-gate regression proof
compared full-suite canonical logs: `test_before.log` captured the pre-fix
`backend_codegen_route_riscv64_pointer_typed_select_publication` failure at
`3344/3345`, and `test_after.log` captured the post-fix full-suite pass at
`3345/3345`. No forbidden-snippet expectation was weakened.

## Reviewer Reject Signals

- The route removes `mv t0, t0` only by weakening or deleting the forbidden
  snippet check.
- The route adds a filename, test-name, or exact-output special case for
  `riscv64_pointer_typed_select_publication`.
- The route hides the self-move behind different spelling or dead output while
  leaving the same redundant register-to-itself move behavior in the backend.
- The route claims progress only by updating baseline logs or expected output.
- The route performs a broad backend rewrite without focused evidence that it
  repairs the pointer-typed select publication failure.
- The route regresses nearby select-chain, pointer, or RV64 publication tests.
