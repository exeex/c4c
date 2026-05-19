Status: Active
Source Idea Path: ideas/open/307_aarch64_large_scalar_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Illegal Immediate Path

# Current Packet

## Just Finished

Step 1: Localize The Illegal Immediate Path is complete. The illegal
`build/c_testsuite_aarch64_backend/src/00182.c.s:1193` line is in `main`
immediately before `bl print_led`:
`mov x0, #1234567`.

Responsible backend path:

- `src/backend/prealloc/call_plans.cpp` records the first `print_led`
  argument as `PreparedCallArgumentPlan::source_literal` from the prepared
  value home's `immediate_i32`.
- `src/backend/mir/aarch64/codegen/calls.cpp:748`
  `lower_before_call_immediate_binding` finds that immediate call argument,
  converts it with `make_scalar_call_argument_immediate`, and creates a
  `CallBoundaryMoveInstructionRecord` with `source_immediate` and destination
  ABI register `x0`.
- `src/backend/mir/aarch64/codegen/calls.cpp:1132`
  `lower_before_call_moves` appends that synthetic immediate binding before the
  real call instruction.
- `src/backend/mir/aarch64/codegen/calls.cpp:2034`
  `print_call_boundary_move` forms the final illegal string by writing the
  primary move mnemonic, destination register, and `#` plus
  `move.source_immediate->signed_value`.

Existing helper/reuse point: `src/backend/mir/aarch64/codegen/machine_printer.cpp`
already has `materialize_integer_constant_lines`, which emits legal `movz` /
`movk` sequences for 32-bit and 64-bit integer constants and is reused for
immediate memory-store values and compare setup. It is currently local to
`machine_printer.cpp`, so Step 2 should either move/share that helper or add an
equivalent target-local helper used by call-boundary immediate printing.

Closed-owner boundaries checked:

- Idea 299 closed scalar add/sub/bitwise instruction-immediate fallback and
  explicitly left call-boundary moves out of scope. This failure is not a
  scalar ALU operand fallback regression; it is an ABI call-argument immediate
  materialization path.
- Idea 306 closed symbol+offset address-register-width legality for `00182.c`.
  Its closure note already names the remaining `00182.c` failure as the
  unrelated `mov x0, #1234567` large-immediate assembler form, so this does not
  reopen symbol+offset address materialization.

Proposed Step 2 semantic repair point: handle non-encodable scalar integer
`source_immediate` values when printing/lowering AArch64 call-boundary moves,
emitting a legal constant-materialization sequence into the destination ABI
register for large constants while preserving the existing single `mov` path
only for values known legal for that form. Do this as target constant
materialization behavior, not by matching `00182.c`, `1234567`, or the exact
emitted text.

## Suggested Next

Execute Step 2 by implementing legal large-immediate materialization for
AArch64 call-boundary scalar integer immediates, preferably through a shared
constant-materialization helper rather than a one-off printer branch.

## Watchouts

- The generated-code evidence shows this specific residual is a call-argument
  immediate move, even though generic call-boundary move buckets remain out of
  scope. Keep the Step 2 patch limited to scalar integer constant
  materialization for immediate-to-GPR moves.
- Keep `00189.c` PIC/global-symbol relocation outside this owner unless
  generated-code evidence proves shared constant-materialization semantics.
- Keep runtime nonzero, mismatch, crash, timeout, and output-storm buckets
  parked under umbrella idea 295.
- Do not special-case `00182.c`, `1234567`, or the exact emitted instruction
  string.
- Do not edit expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, or proof-log policy.

## Proof

No tests were rerun by instruction. Localization used existing
`test_before.log` and `build/c_testsuite_aarch64_backend/src/00182.c.s`; no
`test_after.log` was created or modified.
