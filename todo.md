Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Memory-Return Ownership Where Needed

# Current Packet

## Just Finished

Step 4 added explicit prepared i128 helper result ownership policy while
preserving the current div/rem helpers as direct low/high result-lane helpers.

What changed:

- `PreparedI128RuntimeHelper` now distinguishes `missing`,
  `direct_low_high_lanes`, and `memory_return` result ownership policy.
- Div/rem helper mappings set `result_ownership=direct_low_high_lanes`, keeping
  their result authority tied to structured low/high result lane bindings.
- Added a structured `MemoryReturnOwnership` carrier shape on helper records
  for future memory-return helpers: destination value id/name, size, alignment,
  slot id, and stack offset.
- Helper lane enrichment now validates result ownership: direct helpers require
  result low/high lanes, memory-return helpers fail closed until explicit
  destination ownership is populated, and missing policy diagnoses explicitly.
- Prepared printer output exposes `result_ownership` and `memory_return`
  fields, with div/rem printing `memory_return=<none>`.
- Focused prepared tests prove div/rem direct-result policy and printer
  visibility alongside the existing lane/carrier diagnostics.

No clobber/resource policy, AArch64 selected helper nodes, terminal printer
output, target-local helper synthesis, fixed-register marshaling, or
scalar-i64 substitutes were added.

## Suggested Next

Execute Step 5 as a prepared/shared helper clobber/resource/ABI policy packet:
add explicit helper boundary policy for supported i128 helper families,
including call-clobber/resource facts and ABI/register-bank transition facts
needed by later AArch64 selected helper records.

Suggested focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Currently supported div/rem helpers are explicitly direct-result helpers, not
  memory-return helpers.
- The memory-return carrier shape exists, but no supported helper family
  populates it yet; a future memory-return helper must provide destination
  slot/offset/size/alignment facts or fail closed.
- Helper lane records still consume canonical `PreparedI128Carrier` facts and
  diagnose non-register result/argument lanes as missing direct register-pair
  ABI authority.
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

Supervisor full-suite acceptance also passed for this Step 4 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
