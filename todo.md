# Current Packet

Status: Active
Source Idea Path: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Hand Back To Object Route Scan

## Just Finished

- Completed Step 4 expanded object-route scan validation for child 337.
- Proved the combined restored scalar object baseline is green at 33/33 tests.
- The validated baseline includes the RV64 object runtime scan cases restored
  in Step 2:
  `backend_obj_runtime_rv64_two_arg_local_arg`,
  `backend_obj_runtime_rv64_two_arg_second_local_arg`,
  `backend_obj_runtime_rv64_two_arg_both_local_arg`, and
  `backend_obj_runtime_rv64_local_arg_call`.
- The validated baseline also includes the AArch64 object-byte scan case
  restored in Step 3:
  `backend_cli_aarch64_two_arg_helper_writes_elf_obj`.
- Existing object model, RV64/AArch64 object-emitter unit tests, CLI object
  diagnostics, RV64 object runtime scalar cases, and AArch64 asm scalar smoke
  cases remain green in the same proof.

## Suggested Next

- Proceed to Step 5 handoff/closure for child 337.
- Step 5 should summarize the added target-emitter capabilities and hand back
  to `ideas/open/334_object_route_scan_and_default_readiness.md` with this
  33/33 expanded scan proof as the resumed baseline.
- Suggested Step 5 action: call the plan owner to close child 337 and resume
  parent idea 334 if it agrees the child source idea is complete.

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- AArch64 `two_arg_helper.c` is object-byte proof only in this child; AArch64
  runtime remains out of scope.
- Keep RV64 globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of this child.
- Parent idea 334 should resume only after this child restores the next object
  scan candidates as green native object-output tests.

## Proof

- Ran Step 4 expanded scan proof:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: 33/33 tests passed. Proof log: `test_after.log`.
