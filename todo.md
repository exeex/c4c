# Current Packet

Status: Active
Source Idea Path: ideas/open/334_object_route_scan_and_default_readiness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Default-Readiness Decision

## Just Finished

- Completed Step 3 default-readiness decision from the expanded 41/41
  object-route scan baseline.
- Current recorded evidence:
  - `test_before.log` is rolled to the expanded 41/41 baseline after
    `155ba930 Add RV64 c-testsuite object smoke scan`.
  - The selected proof command remains:
    `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1`
  - Result recorded in the rolled baseline: 41/41 tests passed.
- Object-route evidence now includes:
  - Backend object model and target object-emitter unit coverage:
    `backend_object_model_records`, `backend_riscv_object_emission`, and
    `backend_aarch64_object_emission`.
  - CLI object-byte output and diagnostics through `backend_cli_.*obj`,
    including AArch64 c-testsuite `00001.c`, `00002.c`, `00011.c`, and
    `00012.c`.
  - RV64 native object runtime through `backend_obj_runtime_.*`, including
    c-testsuite `00001.c`, `00002.c`, `00011.c`, and `00012.c`.
- Asm-route evidence remains selected in the same baseline:
  - RV64 asm runtime counterparts for the scalar/local-call baseline.
  - AArch64 asm external return smokes for zero/add/add-sub-chain.
  - The route still preserves asm coverage rather than replacing it with
    object-only proof.
- Recommendation: keep `--codegen obj` as an explicit dual-route option for
  now; do not make native object output the default backend route yet.
- Rationale: object-route evidence is substantially stronger than the original
  CLI smoke, but default readiness is not yet balanced enough across target
  features and policies.

## Suggested Next

- Request plan-owner review for parent idea 334.
- Suggested lifecycle decision: parent 334 can close as a completed
  scan/default-readiness decision if the plan owner accepts that the durable
  output is the recorded recommendation to keep object output dual-route for
  now, with remaining gaps left as follow-up boundaries.
- If the plan owner decides 334 should remain active, the next execution should
  split a focused AArch64 saved-callee/local-call-frame object-emitter child
  before adding balanced `00003.c`, `param_slot.c`, or `two_arg_*_rewrite.c`
  scan cases.

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
- Remaining boundaries supporting the "not default yet" recommendation:
  AArch64 saved-callee/local-call-frame support, branch/control-flow,
  branch/global c-testsuite families, AArch64 runtime, globals/data, pointers,
  aggregates, byval, indirect calls, x86 object output, object stdout,
  c-testsuite object defaults, and object semantic-BIR mode.

## Proof

- Decision-only packet; no build required.
- Verified `test_before.log` records the rolled 41/41 baseline.
- Ran `git diff --check`; result: passed.
