Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Helper Clobber, Resource, And ABI Authority

# Current Packet

## Just Finished

Step 5 added explicit prepared i128 runtime helper boundary policy for the
currently supported div/rem helper family.

What changed:

- `PreparedI128RuntimeHelper` now carries structured resource policy facts for
  call-boundary use, runtime-helper callee use, caller-saved clobbers, and
  source-operation identity preservation.
- Helper records now carry structured ABI/register-bank transition facts:
  direct register-pair argument/result transition, GPR argument/result banks,
  two source operands, two low/high lanes per operand, two result lanes, and
  8-byte lane width.
- Helper clobber authority reuses the existing prepared call clobber vocabulary
  by publishing `PreparedClobberedRegister` records from the same caller-saved
  clobber set builder used by retained `PreparedCallPlan` records.
- Boundary enrichment remains producer-side in prepared state after canonical
  i128 carriers exist; no AArch64 helper node, printer output, target-local
  helper synthesis, fixed-register marshaling, or scalar-i64 substitute was
  added.
- Prepared printer output now dumps helper resources, ABI transition policy,
  register banks, lane counts, lane width, and clobber summaries.
- Focused tests prove div/rem helpers expose resource, ABI/register-bank, and
  clobber facts while preserving direct-result ownership and existing
  fail-closed carrier diagnostics.

## Suggested Next

Execute Step 6 validation and handoff: prove the prepared i128 helper
authority prerequisite, summarize supported div/rem helper mapping, low/high
lane ownership, direct-result ownership, clobber/resource policy, and ABI
transition facts, then state whether idea 236 can resume its selected AArch64
i128 helper-boundary packet.

Suggested focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Supported div/rem helpers are still direct low/high result-lane helpers; the
  memory-return carrier shape exists but no current supported family populates
  memory-return ownership.
- Helper lanes still consume canonical `PreparedI128Carrier` records; do not
  infer lanes from raw regalloc assignment order, rendered register names,
  register adjacency, or fixed ABI registers.
- The boundary policy records helper ABI shape and register banks, not actual
  target-local marshaling registers. Later AArch64 selected records must
  consume these facts without synthesizing helper calls from opcodes alone.
- Float/i128 conversion helper mapping remains explicitly deferred.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, and `backend_prepared_printer`
passed, 3/3 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 5 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
