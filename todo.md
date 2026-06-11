Status: Active
Source Idea Path: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Reproduce and Classify the Two Failures Separately

# Current Packet

## Just Finished

Completed Step 2: Reproduce and Classify the Two Failures Separately.

Supervisor baseline comparison:
- `test_before.log` used the same delegated command and recorded
  `STEP2_STATUS string_authority_guard=8
  c_testsuite_aarch64_backend_src_00040_c=0`.
- The current run in `test_after.log` records the same status line:
  `STEP2_STATUS string_authority_guard=8
  c_testsuite_aarch64_backend_src_00040_c=0`.

Independent reproduction results:
- `string_authority_guard` reproduced as a deterministic narrow guard failure
  in the current checkout: CTest exit status `8`, `0% tests passed, 1 tests
  failed out of 1`.
- The guard diagnostic is:
  `src/backend/mir/aarch64/codegen/calls.cpp:8062:
  find_prepared_indirect_callee_stored_value_source_fallback:
  pattern=lookup-helper-rendered-compatibility: classify this exact
  path+symbol in scripts/string_authority_classifications.json or replace it
  with structured authority`.
- `c_testsuite_aarch64_backend_src_00040_c` did not reproduce as a timeout in
  the isolated Step 2 run: CTest exit status `0`, passed in 2.93 sec, `100%
  tests passed, 0 tests failed out of 1`.
- Classify `00040` as timeout/noise for this packet unless a later broader run
  reproduces it; keep it separate from the deterministic string-authority
  route.

## Suggested Next

Execute Step 3: isolate the first bad string-authority step by manually
checking the ordered Phase D follow-up readiness and lifecycle naming commits
with the narrow `string_authority_guard` command. Keep the `00040`
timeout/noise classification separate.

## Watchouts

- The Step 2 isolated proof reproduced only `string_authority_guard`; `00040`
  passed and should not drive the next route unless it reappears in broader
  validation.
- Do not infer first-bad from `1d1c506f0`; Step 1 found
  `string_authority_guard` already failing in
  `log/baseline_521cf148fa7660766f5bfa8c0932833bfa987311.log`.
- Do not weaken, disable, or bypass `string_authority_guard`.
- Do not delete, rename, or broadly rewrite Phase D or Phase E docs just to
  satisfy the guard.
- Do not open draft 155 or proceed with true Phase E retirement under this
  attribution plan.
- Missing artifact for exact first-bad: no full-suite archived baseline exists
  between `3486d702e` at 12:34 and `521cf148f` at 13:39.

## Proof

Ran the supervisor-delegated exact proof command and captured all output in
`test_after.log`:

```sh
(
  set -o pipefail
  cmake --build --preset default && \
  ctest --test-dir build --output-on-failure -R '^string_authority_guard$' ; \
  string_status=$? ; \
  ctest --test-dir build --output-on-failure -R '^c_testsuite_aarch64_backend_src_00040_c$' ; \
  c00040_status=$? ; \
  printf '\nSTEP2_STATUS string_authority_guard=%s c_testsuite_aarch64_backend_src_00040_c=%s\n' "$string_status" "$c00040_status" ; \
  test "$string_status" -eq 0 -a "$c00040_status" -eq 0
) > test_after.log 2>&1
```

The command exited `1` because `string_authority_guard` failed while `00040`
passed. The supervisor-selected proof is sufficient for Step 2 reproduction
and classification, and `test_after.log` is the proof log path.
