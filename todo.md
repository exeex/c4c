# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resume Expanded Scan Baseline And Candidate Map

## Just Finished

- Closed child 337 and resumed parent idea 334 from the expanded object-route
  scan baseline.
- Baseline proof available in `test_before.log` and rerun in `test_after.log`:
  33/33 tests passed with non-decreasing regression guard.

## Suggested Next

- Execute Step 1 by inspecting the current backend object/asm/runtime scan
  surface and recording the next conservative scan candidates or blockers.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep globals/data, arrays, pointers, aggregates, broad memory/control flow,
  broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of the first resumed scan packet unless Step 1 deliberately maps
  them as future policy boundaries.

## Proof

- Close-time expanded scan proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 33/33 tests passed.
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 33 before and 33 after.
