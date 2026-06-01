# AArch64 Local Dispatch Block Route

Ownership class: target-owner split with keep-local preservation

## Goal

Preserve the local AArch64 prepared-block dispatch route while narrowing any
remaining public block entry points, diagnostics, hook wiring, and sequencing
after the larger helper splits have landed.

## Why This Exists

Idea 59 classified the core block dispatch route as keep-local: AArch64 still
owns block traversal, unsupported diagnostics, BIR context lookup, hook wiring,
before-return publication sequencing, branch-fusion integration, and final
machine-instruction ordering. This idea exists to keep that local route precise
after semantic producer, publication, and value-materialization authority has
left broad dispatch files.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Public block entry points including `make_block_lowering_context` and
  `dispatch_prepared_block`.
- Routing and diagnostics including `classify_instruction`, unsupported
  diagnostics, BIR block/context lookup, and `make_bir_machine_instruction`.
- Before-return publication sequencing, branch-fusion hook wiring, and final
  machine-instruction sequencing.

## Out Of Scope

- Taking semantic producer or publication decisions back into block dispatch.
- Moving AArch64 block traversal or target diagnostics to shared code.
- Running this as the first dispatch contraction slice before major helper
  splits are decided.
- Attempting to solve all dispatch-family contraction in one change.

## Acceptance Criteria

- The block route remains local and explicitly limited to traversal, routing,
  diagnostics, hook wiring, before-return publication sequencing, and final
  machine-instruction sequencing.
- Any split-owner cleanup moves helpers to precise local owners without
  reintroducing producer, publication, or value-materialization authority into
  `dispatch.cpp`.
- Unsupported diagnostics remain durable and are not weakened to accommodate
  moved helpers.
- Focused proof covers block traversal/routing, unsupported instruction and
  terminator diagnostics, before-return publication, branch-fusion hooks, and
  representative prepared-block lowering.

## Reviewer Reject Signals

- Block dispatch starts owning semantic producer or publication decisions.
- Diagnostics are weakened to pass moved helpers.
- Sequencing changes before dependent owner splits are complete.
- The slice attempts to solve all dispatch-family contraction at once.
- The implementation claims narrowing while leaving the same broad public block
  route and helper authority unchanged.

## Closure Note

Closed after the local prepared-block dispatch route was kept as an explicit
AArch64 owner while unused dispatch scaffolding was removed and stack-home
lookup routing was narrowed through the publication owner.

Evidence from the landed slice:

- Unused AArch64 dispatch scaffolding was deleted.
- Remaining stack-home lookup routing uses the publication owner instead of
  broadening `dispatch.cpp`.
- The local block route remains limited to traversal, routing, diagnostics,
  hook wiring, before-return publication sequencing, and final instruction
  ordering.
- No semantic producer, publication, or value-materialization authority was
  moved back into the local dispatch route.
- The remaining evidence-only local-slot offset question was split to idea 67.
