Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Missing Local-Memory Handoff Fact

# Current Packet

## Just Finished

Lifecycle switch complete: umbrella idea 295 Step 4 split a focused owner for
residual AArch64 semantic `lir_to_bir` local-memory admission/prepared-module
handoff represented by `00204.c` and `00216.c`.

## Suggested Next

Execute Step 1 from `plan.md`: localize the missing semantic or prepared
handoff fact behind the current diagnostics:

- `c_testsuite_aarch64_backend_src_00204_c`: `semantic lir_to_bir function
  'myprintf' failed in gep local-memory semantic family`
- `c_testsuite_aarch64_backend_src_00216_c`: `semantic lir_to_bir function
  'foo' failed in load local-memory semantic family`

Use this focused proof command when a packet needs validation:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

## Watchouts

- This owner is semantic admission/prepared-handoff only. Do not fold in
  AArch64 printer, assembler, linker, runtime, timeout, direct-call shuffle,
  direct vararg, address-of-local, `00164.c`, or `00214.c` residuals without a
  separate split.
- Do not reopen closed ideas 297, 298, or 311 from pass counts alone.
- No implementation files, tests, proof logs, `ideas/closed/*`, or
  `review/311_aggregate_stack_copy_review.md` were changed during activation.

## Proof

No validation run; this was lifecycle-only activation from the umbrella
inventory. The focused owner is based on the existing Step 1 backend-regex
evidence in `test_after.log`: 352 selected, 295 passed, and 57 failed.
