Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify 00189 Runtime/Call-Argument Crash

# Current Packet

## Just Finished

Lifecycle activation created a new active runbook for the remaining open
umbrella inventory after focused idea 308 closed. No implementation,
test, expectation, allowlist, CTest, runner, timeout, or proof-log files were
changed.

## Suggested Next

Execute Step 1 from `plan.md`: classify the post-308 `00189.c`
`RUNTIME_NONZERO exit=Segmentation fault` as a runtime/call-argument failure.
Use current `test_before.log`, `tests/c/external/c-testsuite/src/00189.c`,
and `build/c_testsuite_aarch64_backend/src/00189.c.s` as evidence. Confirm the
old extern-data-symbol PIC relocation failure stays closed before deciding
whether to split a new focused owner.

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 308 from counts alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Treat `00189.c` as returned to the runtime/call-argument path unless
  generated-code evidence contradicts idea 308's closure boundary.
- Do not run a fresh broad backend regex command unless the supervisor
  delegates that proof command with timeout and stale-process guidance.

## Proof

No build or tests were run for this lifecycle-only activation.
