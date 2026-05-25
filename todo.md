Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Or Narrow Pure Printer Ownership

# Current Packet

## Just Finished

Step 2 moved the remaining pure `print_call_boundary_move` printer island into
`machine_printer.cpp` at the `CallBoundaryMoveInstructionRecord` payload dispatch
boundary. `calls.hpp` no longer exports `print_call_boundary_move`, and
`calls_printing.cpp` no longer implements the printer-only call-boundary move
spelling helpers.

The dependency split was viable: frame-slot load/address spelling, aggregate
lane publication spelling, scalar immediate materialization checks, scalar FPR
immediate spelling, and f128 q-register spelling moved as printer-local helpers.
Call-boundary move construction, selection/status, operand/effect publication,
and prepared provenance remain calls-owned.

## Suggested Next

Supervisor should review the Step 2 slice for acceptance/commit readiness and
decide whether Step 2 is exhausted or needs a follow-up route check.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into whole dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- Keep AArch64-specific register, memory, frame-slot, immediate, and assembly
  spelling in AArch64 code; the target of this checkpoint is ownership of
  printer and effect-publication responsibilities.
- Keep source-idea progress tied to deleted duplication, moved ownership, or a
  sharper emission-only boundary.
- Do not move `effect_from_prepared_call_preserved_value` to the printer.
  Blocker: the prepared preserved-value route is currently converted into
  machine-node `preserves` by `make_call_instruction`; there is no separate
  printer-owned fact that can replace that effect publication.
- `calls_moves.cpp` still owns construction-side frame-slot address
  printability checks used to decide whether a call-boundary move can be
  represented; this packet intentionally moved only printer spelling helpers.

## Proof

Step 2 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_machine_printer|aarch64_call_boundary_owner|call_boundary_effect_plan|prepared_printer|aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Proof log: `test_after.log`. The focused subset passed 5/5 tests.

Supervisor broader backend guard also passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Matching `test_before.log`/`test_after.log` backend comparison passed with
162/162 tests before and after, with no new failures.
