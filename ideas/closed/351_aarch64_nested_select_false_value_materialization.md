# AArch64 Nested Select False Value Materialization

Status: Closed
Created: 2026-05-20
Split From: ideas/closed/316_aarch64_frame_slot_layout_consistency.md
Closed: 2026-05-20

## Goal

Repair AArch64 scalar select/value materialization when a nested select chain
must preserve the accumulated false operand across the final condition check.

## Why This Exists

Idea 316 repaired the `00182.c` caller local-array frame underallocation and
advanced the representative past the saved-return-address overwrite. The new
first bad fact is not frame layout. Prepared `print_led` still preserves the
digit extraction and lookup semantics: it computes `urem x, 10`, stores each
digit to `d[n]`, and later selects `d[i]` for the LED line renderers.

Generated AArch64 lowers the final digit lookup select chain so the last
`cmp i, #0` clobbers or overwrites the accumulated false value. The final
`csel` receives operands that both name `d[0]`, so false paths that should
carry `d[1]` through `d[31]` collapse to the first element. For input
`1234567`, the runtime output renders seven `7` digits.

## In Scope

- Localize nested scalar select lowering where the selected false value is an
  accumulated result from earlier select arms.
- Repair value materialization or operand preservation so the final select can
  choose between the current true value and the previously accumulated false
  value after evaluating the final condition.
- Add focused backend coverage for nested select chains where the final
  condition is false and the selected value must come from a non-first false
  operand.
- Use `c_testsuite_aarch64_backend_src_00182_c` as external proof only after
  focused select/value materialization coverage owns the repair.

## Out Of Scope

- Frame-slot/frame-size layout consistency, local aggregate allocation, or
  saved-return-address overwrite repairs closed by idea 316.
- Unsigned div/rem producer publication, digit extraction semantics, LED
  formatting logic, local-array address formation, direct-call lowering, or
  prepared call/address materialization unless fresh evidence reaches those
  exact boundaries.
- Reopening the closed scalar select result publication owner from idea 345
  unless current evidence proves the same stack-home publication failure has
  returned.
- Expectation changes, unsupported-classification changes, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Fixing only `00182`, `print_led`, `topline`, `midline`, `botline`, the LED
  digit array, one temporary, one register, or one emitted instruction
  sequence.

## Acceptance Criteria

- The first bad boundary is localized to a concrete AArch64 nested
  select/value materialization or operand-preservation owner.
- Focused backend coverage fails without the repair and passes with it for a
  nested select chain whose final condition is false and whose result must use
  the accumulated false operand.
- Generated AArch64 keeps the accumulated false select value live across later
  condition checks or rematerializes it correctly before the final `csel`.
- `c_testsuite_aarch64_backend_src_00182_c` either passes or advances to a new
  first bad fact after the select/materialization repair.
- Adjacent scalar select publication, scalar operand materialization, and
  frame-layout guardrails remain stable.

## Closure Notes

Closed after repairing AArch64 scalar select publication so the accumulated
false operand is materialized into the final result before the true scratch is
filled. Focused coverage now proves a nested scalar select whose final false
operand comes from the previously accumulated stack-published select result.

Close proof used the existing matching focused regression logs:
`test_before.log` had 6/7 passing with
`c_testsuite_aarch64_backend_src_00182_c` failing by rendering seven `7`
digits, and `test_after.log` had 7/7 passing with no new failures. The
close-time regression guard passed. Supervisor broader backend guard
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 141/141.

The attempted full-suite baseline candidate was rejected because it introduced
five new failures despite net improvement, so it was not used as closure proof.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00182`, `print_led`, the LED line renderer calls, `d[0]`,
  `MAX_DIGITS`, one temporary, one register, one comparison, or one exact
  emitted instruction sequence instead of repairing nested select value
  materialization generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites, classification
  notes, or output-count changes without making generated consumers observe
  the selected false operand;
- broadens into frame layout, unsigned div/rem, direct-call lowering, semantic
  admission, aggregate storage, variadic lowering, or frontend work without
  fresh first-bad-fact evidence;
- leaves generated AArch64 able to overwrite or lose the accumulated false
  select value behind a renamed abstraction.
