# RISC-V Prepared Edge Publication Stack Destination Support

## Goal

Add RISC-V prepared edge-publication support for source-to-`StackSlot`
destination moves as a destination-home initiative separate from
register-destination consumers.

## Why This Exists

Idea 24 was deliberately scoped to register destinations. Source-to-stack
destination moves need destination-specific store/addressing policy and should
not be claimed by the register-destination consumer route.

## In Scope

- Define RISC-V-local stack-slot destination store/address emission for
  prepared edge-publication moves.
- Consume shared prepared `edge_publications` as the only semantic authority
  for the edge move.
- Decide which source homes are safe to support first for stack destinations,
  then implement them with explicit fail-closed behavior for the rest.
- Add focused tests that prove shared publication lookup drives stack
  destination moves and that unsupported source homes fail closed.
- Preserve existing register-destination consumers.

## Out Of Scope

- Pointer-base source materialization unless explicitly selected by this
  destination-home route.
- RISC-V-local edge fact discovery.
- Moving RISC-V stack-destination store policy into shared prepare, BIR, or
  target-neutral helpers.
- Broad RISC-V codegen or memory-layout rewrites unrelated to stack
  destination edge-publication moves.

## Acceptance Criteria

- At least one source-to-`StackSlot` RISC-V prepared edge-publication move is
  implemented through shared lookup authority.
- Tests fail if shared publication facts are absent or ignored.
- Unsupported source or destination homes remain explicit and fail closed.
- Target-specific stack destination emission remains inside the RISC-V backend.
- Validation covers focused RISC-V stack-destination publication tests,
  relevant shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches testcase names, fixture labels, edge names, or prepared
  value ids instead of implementing a semantic stack-destination rule.
- The patch scans predecessor or successor blocks or creates local edge-copy
  facts instead of consuming shared `edge_publications`.
- The patch weakens existing register-destination expectations or marks a
  targeted path unsupported to claim progress.
- The patch moves RISC-V store/addressing policy into shared prepare, BIR, or
  target-neutral helpers.
- The patch claims broad RISC-V edge-publication support without proving the
  selected stack-destination source homes.
- The patch is only a helper rename, expectation rewrite, or classification
  change while source-to-`StackSlot` destinations remain unsupported.

## Closure Note

Closed after implementing focused RISC-V `Register -> StackSlot`
edge-publication consumption through shared `edge_publications` authority.
The supported stack destination form requires a concrete destination
`StackSlot` offset and `size_bytes == 4`, emitted target-locally as
`sw <src>, <offset>(sp)`.

Validation accepted for closure:

- focused RISC-V prepared edge-publication subset passed, 5/5 selected tests
- matching focused regression guard passed, 5/5 before and 5/5 after
- backend bucket evidence in `test_before.log` passed, 163/163 tests
- full-suite baseline evidence in `test_baseline.log` passed, 3411/3411 tests
- `review/idea27_riscv_stack_destination_edge_publication_review.md` reported
  no blocking findings

Unsupported source-to-stack forms remain fail-closed and are preserved in
`ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md`.
