# Current Packet

Status: Active
Source Idea Path: ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Expanded AArch64 Object Frontier

## Just Finished

- Completed Step 4 expanded AArch64 object frontier validation.
- The child now restores native AArch64 object-byte smokes for c-testsuite
  `00011.c` and `00012.c` in addition to the parent baseline's `00001.c` and
  `00002.c` smokes.
- The combined object-route proof is green at 39/39 tests and remains
  monotonic against the rolled 38/38 baseline.
- Validated restored child selectors:
  `backend_cli_aarch64_cts_00011_writes_elf_obj` and
  `backend_cli_aarch64_cts_00012_writes_elf_obj`.
- Existing AArch64 object-emitter unit coverage, AArch64 scalar object-byte
  smokes, RV64 object-runtime baseline selectors, CLI object diagnostics, and
  AArch64 asm smokes remain green in the same proof.

## Suggested Next

- Request plan-owner closure for child 338 and resume parent idea 334 from the
  39/39 expanded object-route baseline.
- Parent 334 can continue default-readiness work with this updated baseline,
  but should keep saved-callee/local-call-frame expansion and branch/global
  control-flow frontiers as explicit follow-up boundaries.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- Keep AArch64 runtime, broad branch/control-flow, globals/data, pointers,
  aggregates, byval, indirect calls, RV64, x86, object stdout, c-testsuite
  defaults, and semantic-BIR object mode out of this child.
- `00003.c`, `param_slot.c`, and the `two_arg_*_rewrite.c` family still need
  saved-callee/local-call-frame expansion beyond the leaf frame-slot memory
  slice completed here.
- Selected scalar multiply for the isolated `00012.c` shape is now supported;
  broader branch/control-flow and branch/global c-testsuite families are still
  outside this child.
- Parent 334 should resume only after this child repairs the AArch64 target
  object frontier or records a precise blocker.

## Proof

- Ran Step 4 expanded validation:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: passed, 39/39 tests. Proof log: `test_after.log`.
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 38 before and 39 after.
