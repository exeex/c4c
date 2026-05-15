Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Hand Back To I128 Pair Lowering

# Current Packet

## Just Finished

Step 6 validated the prepared i128 runtime helper authority prerequisite and
recorded the handoff back to the parked i128 pair-lowering route.

Accepted prepared authority for supported i128 div/rem helpers:

- Source-operation mapping exists for i128 `SDiv`, `UDiv`, `SRem`, and `URem`
  with function/block/instruction identity, source opcode, result value
  identity, operand value identities, and source/result type facts.
- Helper family/kind and callee identity are structural:
  `__divti3`, `__udivti3`, `__modti3`, and `__umodti3` are carried on
  `PreparedI128RuntimeHelper` records rather than inferred in AArch64 target
  lowering.
- Low/high argument and direct-result lane ownership is populated from
  canonical `PreparedI128Carrier` facts by prepared value identity, preserving
  lane order, lane width, carrier kind, register-pair placement, and
  fail-closed diagnostics for missing or non-register carrier authority.
- Result ownership is explicit: current div/rem helpers are
  `direct_low_high_lanes`; memory-return ownership remains represented as a
  carrier shape but no currently supported helper family populates it.
- Helper boundary policy is explicit: records carry call-boundary,
  runtime-helper callee, caller-saved clobber, and source-operation identity
  resource facts.
- ABI/register-bank transition policy is explicit for current div/rem helpers:
  direct register-pair arguments and result, GPR argument/result banks, two
  source operands, two lanes per argument, two result lanes, and 8-byte lane
  width.
- Clobber facts reuse `PreparedClobberedRegister` and the same caller-saved
  clobber-set builder used by retained prepared call plans.

This completes the idea 248 prerequisite for supported div/rem helper
authority. Idea 236 can resume its Step 6 selected AArch64 helper-boundary
consumption packet using these prepared helper records, without synthesizing
helper calls from opcodes, fixed registers, rendered names, register adjacency,
or scalar-i64 substitutes.

## Suggested Next

Ask the plan owner to close or deactivate idea 248 as complete for its current
runbook, then reactivate idea 236 so its selected AArch64 i128 helper-boundary
step can consume the prepared helper authority.

## Watchouts

- Float/i128 conversion helper mapping remains explicitly deferred; it should
  not be treated as part of the div/rem handoff.
- Memory-return ownership is represented but not populated by the currently
  supported div/rem helpers. A future memory-return helper family must provide
  explicit destination identity, storage extent, alignment, slot, and offset
  ownership or fail closed.
- The prepared ABI policy records helper argument/result shape and register
  banks, not target-local marshaling registers. AArch64 selected helper records
  still need to consume the structured facts and decide their own selected-node
  representation.
- `PreparedCallPlan` remains retained-call-only; do not create fake call plans
  for helper-required i128 source operations.

## Proof

No new proof command was run for this validation-only todo handoff packet.

Accepted Step 5 focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, and `backend_prepared_printer`
passed, 3/3 tests. Proof log: `test_after.log`.

Accepted supervisor full-suite proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
