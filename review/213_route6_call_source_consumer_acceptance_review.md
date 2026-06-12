# Idea 213 Route 6 Call Source Consumer Acceptance Review

- Delegated role: `c4c-reviewer`
- Active source idea: `ideas/open/213_route6_call_source_consumer.md`
- Active plan: `plan.md`
- Review base: `59e5646c0` (`[plan] Activate Route 6 call source consumer plan`)
- Base rationale: this is the active plan activation checkpoint for idea 213. Later lifecycle commits in the range select the consumer and record execution/proof status; they do not reset the source idea or create a reviewer checkpoint.
- Reviewed range: `59e5646c0..HEAD`
- Commits since base: 4
- Report path: `review/213_route6_call_source_consumer_acceptance_review.md`

## Findings

No blocking findings.

Low: x86 route-debug byte-stability is recorded as unavailable in this build, not proven.

`todo.md` correctly avoids claiming an x86 proof and records that `build/CMakeCache.txt` has `C4C_ENABLE_X86_BACKEND_TESTS:BOOL=OFF`. `tests/backend/bir/CMakeLists.txt:781` gates `backend_x86_route_debug_test` and `backend_x86_route_debug` behind that option, and `ctest -N` for the requested subset listed only the four non-x86 tests. Because the reviewed diff does not touch x86 files or expected strings, this is acceptable as a residual proof limitation rather than a blocker for this AArch64-selected consumer slice. If the supervisor wants closure-grade x86 byte-stability, that should be a follow-up run in an x86-enabled build.

## Route And Contract Review

The implementation matches the source idea. The selected consumer is the primary non-`result_lanes_only` AArch64 call-result source-register publication in `record_call_result_source_register(...)`, matching the Step 1 selection recorded in `todo.md`. The production path builds a `Route6CallUseSourceIndex` lazily from the retained BIR function at `src/backend/mir/aarch64/codegen/dispatch.cpp:520` through `src/backend/mir/aarch64/codegen/dispatch.cpp:530` and passes it only to the primary publication call at `src/backend/mir/aarch64/codegen/dispatch.cpp:707`; the result-lane-only pass at `src/backend/mir/aarch64/codegen/dispatch.cpp:720` remains on the prepared path.

Route 6 does not own ABI/register/home/move/wrapper/final-record/printer/emission policy. The new evidence helper checks call key uniqueness, prepared destination value identity, and Route 6 result-value availability at `src/backend/mir/aarch64/codegen/calls.cpp:8699` through `src/backend/mir/aarch64/codegen/calls.cpp:8775`. The actual scalar register publication remains driven by prepared `PreparedCallPlan::result`, value-home lookup, prepared register bank/view conversion, and prepared occupied-register names at `src/backend/mir/aarch64/codegen/calls.cpp:8798` through `src/backend/mir/aarch64/codegen/calls.cpp:8868`.

Fail-closed behavior is covered. The focused AArch64 test enumerates matching, null index, missing fact, invalid boundary, duplicate/conflict, and route/prepared mismatch cases at `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:544` through `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:551`. It mutates the Route 6 index for missing, invalid, and duplicate/conflicting facts at `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:666` through `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:690`, expects `Agreed` only for the matching case at `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:704` through `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:707`, and verifies the prepared `x0` call-result source publication remains intact at `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:712` through `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp:723`.

I found no testcase-overfit pattern. The diff does not add named-case production dispatch, printed-LLVM substring probes, minimal backend shortcut emitters, expectation rewrites, unsupported downgrades, or baseline refreshes. The only test change adds white-box coverage for the selected consumer's agreement/fallback contract.

## Proof Review

`test_after.log` contains a fresh green four-test CTest proof:

```bash
backend_aarch64_instruction_dispatch
backend_aarch64_call_boundary_owner
backend_prepared_printer
backend_prepared_lookup_helper
```

The recorded command included building the corresponding targets before CTest, but only the CTest output was tee'd into `test_after.log`. That is acceptable for this review because the tests ran and passed, and the diff is narrow. No x86 proof was claimed.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `continue current route`

Acceptance-ready for supervisor-side validation and commit. Residual risk is limited to the x86 route-debug byte-stability proof being unavailable in this configured build rather than independently proven.
