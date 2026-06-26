Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Residual Call-Result Stack Publication

# Current Packet

## Just Finished

Step 2-4 continuation: repaired scalar integer stack-slot call-result
publication in `fragment_for_prepared_call()` when prepared result banks are
`None` but the result has explicit source-register metadata, register source
storage, scalar integer BIR result type, matching stack-slot destination facts,
and late source-register publication evidence.

The focused object-emission fixture now covers the explicit
`value_bank=None` / `source_register_bank=None` call-result plan and still
rejects missing register-source publication, explicit FPR source bank,
multi-width destinations, missing or mismatched stack-slot destination facts,
pointer results, non-scalar results, and unsupported destination storage.

The single `src/20000217-1.c` RV64 gcc-torture backend object representative
now passes. The audited call-result blocker is fixed in this checkout.

## Suggested Next

Step 5 closure/handoff review is ready. The representative no longer exposes an
in-scope instruction-fragment residual for idea 377.

## Watchouts

- The admitted lowering is metadata-driven; it does not key on function names,
  value ids, frame-slot ids, concrete registers beyond prepared ABI/source
  metadata, instruction indexes, source syntax, or prepared-BIR text.
- Explicit `source_register_bank=None` is accepted only in the scalar integer
  stack-slot call-result publication path; explicit FPR/VREG-style cases remain
  fail-closed.
- A temporary diagnostic probe was used during implementation and removed before
  the final proof. The final source rebuild and tests came from clean source.

## Proof

Focused proof passed and wrote the canonical root proof log:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Representative proof passed:

```sh
ALLOWLIST=build/agent_state/377_step4_bank_none_20000217-1.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/377_step4_bank_none_20000217-1.runner.log 2>&1
```

Result: `src/20000217-1.c` passed (`total=1 passed=1 failed=0`).

Artifact paths:

- `test_after.log`
- `build/agent_state/377_step4_bank_none_20000217-1.allowlist.txt`
- `build/agent_state/377_step4_bank_none_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- diagnostic-only implementation artifacts under
  `build/agent_state/377_step4_bank_none_20000217-1.*`
