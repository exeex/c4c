Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Helper Marshaling Facts

# Current Packet

## Just Finished

Step 3 added structured helper marshaling and unmarshaling facts for supported
i128 div/rem helpers.

What changed:

- `PreparedI128RuntimeHelper` now carries structured `MarshalingMove` facts for
  lhs/rhs low/high source lanes and direct-result low/high lanes.
- Source marshaling moves connect canonical `PreparedI128Carrier` lane
  authority to Step 2 helper ABI argument-register bindings with explicit
  `carrier_lane_to_abi_argument` direction and `before_call` phase.
- Result unmarshaling moves connect helper ABI result-register bindings to
  canonical result carrier lanes with explicit `abi_result_to_carrier_lane`
  direction and `after_call` phase.
- Marshaling facts preserve value identity, lane role/index, lane width,
  carrier register or memory slot/offset authority, ABI register identity,
  register placement, phase, and move operation kind.
- Memory-backed carrier lanes are represented structurally through slot and
  stack-offset fields; the producer does not require register-pair lanes just
  to describe marshal/unmarshal movement.
- Incomplete carrier lanes, missing ABI bindings, unsupported target register
  bindings, and lane identity mismatches still diagnose through helper
  `missing_required_facts`.
- Prepared printer output now dumps helper marshaling facts, and focused
  prepared tests prove source marshal, result unmarshal, and memory-backed
  carrier-lane facts.

No selected-call ownership, live-preservation authority, terminal `bl <callee>`
helper-call output, fake `PreparedCallPlan` entries, target-local fixed-register
marshaling, scalar-i64 substitute, conversion helper mapping, or memory-return
helper support was added.

## Suggested Next

Execute Step 4: add helper call-clobber, live-preservation, and selected-call
ownership authority. The next packet should decide when a helper boundary has
complete resource/clobber/live-preservation facts and explicit ownership of the
terminal helper call, while keeping printer output fail-closed until those
selected facts are consumed.

## Watchouts

- Step 3 describes marshal/unmarshal moves as prepared facts only. It still
  does not authorize terminal helper-call printing.
- `I128RuntimeHelperBoundaryRecord` printer output must remain fail-closed
  until selected-call ownership and live-preservation facts are structurally
  available and consumed.
- Do not convert helper div/rem operations into fake retained calls or generic
  `PreparedCallPlan` records. The helper-specific facts now carry ABI binding
  and marshal/unmarshal authority directly.
- Float/i128 conversion helpers and memory-return helper families remain
  deferred.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, `backend_prepared_printer`,
`backend_aarch64_target_instruction_records`,
`backend_aarch64_instruction_dispatch`, and `backend_aarch64_machine_printer`
passed, 6/6 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 3 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
