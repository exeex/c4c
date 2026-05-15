# Prepared I128 Helper Marshaling ABI Binding

Status: Closed
Created: 2026-05-15

Parent Context: ideas/open/236_aarch64_i128_pair_lowering.md

## Goal

Expose prepared/shared helper marshaling and ABI register-binding facts needed
to print executable AArch64 calls for supported i128 div/rem helper-boundary
records.

## Why This Exists

Idea 236 now has structured i128 carriers, selected pair operations, prepared
div/rem helper authority, and selected `I128RuntimeHelperBoundaryRecord`
values. Step 8 still could not claim terminal div/rem helper-call printing:
the selected helper boundary lacks structured authority for how low/high i128
lanes are marshaled into helper ABI registers, how direct low/high results are
bound back to prepared carriers, and which call-clobber/live-value facts must
be preserved around the helper call.

Without this prerequisite, AArch64 printer work would have to hard-code helper
ABI registers, infer register adjacency, or synthesize call operands from
opcodes and rendered names.

## In Scope

- Define prepared/shared ABI argument register bindings for low/high i128
  helper lanes.
- Define prepared/shared direct-result register bindings for supported i128
  div/rem helper results.
- Preserve structured move/marshaling facts between prepared i128 carrier
  lanes and helper ABI registers.
- Preserve helper call-clobber, live-value preservation, and resource facts
  needed by selected helper-boundary consumers.
- Expose selected-call operand ownership needed for terminal `bl <callee>`
  output.
- Preserve fail-closed diagnostics for incomplete marshaling authority, wrong
  carrier shape, unsupported memory-return ownership, or unsupported helper
  family.

## Out Of Scope

- Do not add float/i128 conversion helper mapping.
- Do not add memory-return helper-family support unless a separate source idea
  supplies destination identity, storage extent, alignment, slot, and offset
  ownership.
- Do not implement generic call lowering, retained-call rewrites, binary128
  soft-float, atomics, intrinsics, inline asm, callee-save placement, or
  preserved-value extent work.
- Do not hard-code `x0`/`x1` or adjacent register assumptions inside AArch64
  printer or dispatch code.
- Do not lower helper-required i128 operations as scalar i64 shortcuts.

## Acceptance Criteria

- Supported i128 div/rem helper boundaries expose structured ABI argument and
  direct-result register bindings for every low/high lane.
- Selected helper-boundary consumers can identify the moves/marshaling needed
  between prepared carrier lanes and helper ABI registers without local
  register inference.
- Helper call-clobber, live-value preservation, and resource facts are explicit
  enough for terminal call emission to remain structured.
- Incomplete marshaling or unsupported helper shapes fail closed with explicit
  diagnostics.
- Idea 236 can resume with div/rem helper-boundary terminal printing consuming
  these facts rather than hard-coding helper ABI registers.

## Reviewer Reject Signals

- The route hard-codes helper ABI registers, fixed `x0`/`x1` pairs, or
  adjacent register assumptions in AArch64 printer or dispatch code.
- The route infers marshaling from rendered names, opcode spelling, fixture
  names, or helper callee strings instead of prepared/shared facts.
- The route claims progress by weakening unsupported diagnostics or expectation
  text while the same missing ABI register-binding or marshaling authority
  remains absent.
- The route lowers helper-required i128 operations as scalar i64 operations or
  bypasses prepared i128 carriers.
- The route broadens into float/i128 conversion helpers, memory-return helper
  families, binary128, atomics, intrinsics, inline asm, generic call lowering,
  callee-save placement, or preserved-value extent work.

## Closure Notes

Closed: 2026-05-15

The active runbook completed the prepared/shared marshaling and ABI
register-binding authority needed for supported direct-result i128 div/rem
helpers:

- supported direct-result i128 `SDiv`, `UDiv`, `SRem`, and `URem` helpers carry
  source-operation mapping, helper kind, callee identity, and result ownership
- low/high ABI argument and direct-result ABI bindings are structured for lhs,
  rhs, and result lanes
- marshal/unmarshal facts connect canonical `PreparedI128Carrier` lanes to
  helper ABI argument/result bindings without fixed-register inference
- helper resource and clobber policy is explicit for the runtime helper
  boundary
- helper instruction program points are evaluated against liveness intervals
  to build `PreparedCallPreservedValue` facts for values live across supported
  i128 div/rem helper boundaries
- selected-call ownership becomes true only when callee identity, resources,
  clobbers, ABI bindings, marshaling/unmarshaling, and live preservation are
  structurally complete

Float/i128 conversion helpers and memory-return helper families remain
deferred. Idea 236 can resume terminal helper-call printer consumption from
these structured facts and must keep incomplete live-preservation or
unsupported helper shapes fail-closed.

Close proof used the accepted full-suite regression artifact in
`test_before.log`: 3167/3167 tests passed.
