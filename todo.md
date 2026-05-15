Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Low/High Argument And Result Lane Authority

# Current Packet

## Just Finished

Step 3 extended prepared i128 runtime helper records with structural low/high
lane authority for div/rem helpers.

What changed:

- `PreparedI128RuntimeHelper` now carries optional lane bindings for
  `lhs.low`, `lhs.high`, `rhs.low`, `rhs.high`, `result.low`, and
  `result.high`.
- Each lane binding preserves value id/name, lane role, lane index, lane width,
  carrier kind, and either register-pair or memory-backed lane coordinates.
- Div/rem helper population now records source-operation mapping during
  regalloc, then enriches helper lane bindings after `populate_i128_carriers`
  has published canonical `PreparedI128Carrier` facts.
- Lane bindings are copied from each carrier's low/high lane fields by
  prepared value id; non-register or missing carriers preserve structured lane
  coordinates where available and remain fail-closed with helper-level
  diagnostics such as `lhs_requires_register_pair_carrier`,
  `rhs_requires_register_pair_carrier`, and
  `result_requires_register_pair_carrier`.
- Prepared printer output now exposes helper lane bindings and helper-level
  missing facts.
- Focused prepared tests cover lane identity/order/width from canonical
  `PreparedI128Carrier` output, structured memory-backed coordinates,
  register-pair diagnostic policy, existing i128 carrier behavior, and printer
  visibility.

No memory-return ownership, clobber/resource policy, AArch64 selected helper
nodes, terminal printer output, target-local helper synthesis, fixed-register
marshaling, or scalar-i64 substitutes were added.

## Suggested Next

Execute Step 4 as a prepared/shared memory-return ownership packet: identify
which supported i128 helper families require memory-return ownership and add
explicit destination storage facts with value identity, size, alignment,
slot/offset ownership, and fail-closed diagnostics for missing memory-return
authority.

Suggested focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Current helper lane records consume canonical `PreparedI128Carrier` low/high
  lanes. They can expose memory-backed i128 lane coordinates, but direct
  register-pair helper ABI authority is still diagnosed as missing when the
  carrier is not a complete register pair.
- Memory-return ownership remains absent; do not let idea 236 consume these
  helpers as complete call boundaries until Step 4 and Step 5 land.
- Float/i128 conversion helper mapping remains explicitly deferred from Step 2.
- `PreparedCallPlan` is still retained-call-only; do not synthesize fake call
  plans for helper operations in AArch64 dispatch.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, and `backend_prepared_printer`
passed, 3/3 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 3 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
