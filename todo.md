# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Split Next Object Scan Packet

## Just Finished

- Completed Step 2 by adding the parent-owned RV64 c-testsuite object-runtime
  scan registrations for dry-run-supported cases with already-green AArch64
  object-byte counterparts.
- Added `backend_obj_runtime_rv64_cts_00011` for
  `tests/c/external/c-testsuite/src/00011.c`.
- Added `backend_obj_runtime_rv64_cts_00012` for
  `tests/c/external/c-testsuite/src/00012.c`.
- Both use the existing `c4c_add_backend_rv64_object_runtime_case(...)` helper,
  expected run code `0`, and the existing `c_testsuite smoke scan` labels
  layered with helper-added `backend_runtime object rv64 qemu case` labels.
- The expanded object-route proof is now green at 41/41 tests.

## Suggested Next

- Step 3 should make the parent default-readiness decision from the 41/41
  expanded scan baseline.
- Recommendation: keep object output as an explicit dual-route option for now
  rather than making it the default. The scan evidence is stronger, but
  remaining balanced frontier gaps still include AArch64 saved-callee/local-call
  frame support, branch/control-flow families, globals/data, pointers,
  aggregates, byval, indirect calls, AArch64 runtime, x86 object output,
  object stdout, c-testsuite defaults, and semantic-BIR object mode.
- Suggested Step 3 owned file: `todo.md` only unless the supervisor chooses to
  update source intent or lifecycle state.

## Watchouts

- Preserve existing asm-route and object-route selectors.
- Do not add expected-failure labels or weaken object/asm expectations.
- Do not add RV64-only `00003.c` in the first resumed parent packet even
  though it dry-ran green; AArch64 still needs saved-callee/local-call-frame
  object support for the same frontier.
- If the next desired balanced scan target is c-testsuite `00003.c`,
  `param_slot.c`, or any `two_arg_*_rewrite.c`, split a focused AArch64
  saved-callee/local-call-frame object-emitter child before registering those
  cases.
- Keep branch/control-flow, branch/global families, AArch64 runtime,
  globals/data, pointers, aggregates, byval, indirect calls, x86 object output,
  object stdout, c-testsuite defaults, and semantic-BIR object mode out of the
  next parent-owned packet.

## Proof

- Ran the delegated proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
- Result: passed, 41/41 tests.
- Proof log: `test_after.log`.
