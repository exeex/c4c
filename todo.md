Status: Active
Source Idea Path: ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Low/High ABI Argument And Result Bindings

# Current Packet

## Just Finished

Step 2 added prepared/shared low/high ABI register-binding authority for
supported direct-result i128 div/rem helpers.

What changed:

- `PreparedI128RuntimeHelper` now carries structured `AbiRegisterBinding`
  facts beside existing canonical carrier lane bindings.
- Prepared helper records expose lhs/rhs low/high ABI argument bindings and
  result low/high ABI result bindings. Each binding records value identity,
  lane role/index, lane width, helper argument index where applicable, ABI
  register index, register bank/class, concrete register name, occupied
  register set, contiguous width, and register placement.
- The prepared producer populates these bindings from target ABI policy during
  helper enrichment in `prealloc.cpp`; AArch64 printer/dispatch code does not
  synthesize or recover helper registers.
- Unsupported targets, non-div/rem families, non-direct result ownership,
  missing callee identity, and incomplete target register binding state remain
  fail-closed through `missing_required_facts`.
- Prepared printer output now dumps the helper ABI binding section, and focused
  prepared tests prove low/high argument and result binding facts for supported
  direct-result div/rem helpers.

No helper marshaling/unmarshaling move records, selected helper-call ownership,
terminal `bl <callee>` helper-call output, fake `PreparedCallPlan` entries,
target-local fixed-register marshaling, scalar-i64 substitute, conversion
helper mapping, or memory-return helper support was added.

## Suggested Next

Execute Step 3: add structured helper marshaling facts. The next packet should
describe source carrier lane to helper ABI argument-register moves and helper
ABI result-register to result carrier lane moves as producer-owned facts,
consuming the Step 2 ABI bindings plus canonical `PreparedI128Carrier` lane
authority. Keep terminal helper-call printing fail-closed until selected-call
ownership and live-preservation facts are also present.

## Watchouts

- Step 2 only adds ABI register bindings. Lane-to-ABI marshaling and
  ABI-to-lane unmarshaling are still missing, so `I128RuntimeHelperBoundaryRecord`
  printer output must continue to reject executable helper calls.
- Do not reinterpret the new ABI bindings as proof that current carrier lanes
  already occupy helper ABI registers. They are target ABI destinations/sources
  for future move records.
- Generic `PreparedMoveBundle` and `PreparedAbiBinding` remain real-call
  surfaces. Helper div/rem operations need helper-specific marshaling facts
  before they can share terminal call behavior.
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

Supervisor full-suite acceptance also passed for this Step 2 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
