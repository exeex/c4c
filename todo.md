Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Missing Local-Memory Handoff Fact

# Current Packet

## Just Finished

Step 1 localization complete: the current `00204.c` / `00216.c`
semantic failures are both in the local-memory pointer-address handoff lane,
but at adjacent surfaces.

`00204.c` fails in AArch64 `myprintf` after target-specific `va_arg`
expansion creates runtime pointer arithmetic over `struct __va_list_tag_`
fields. The first missing fact is in
`src/backend/bir/lir_to_bir/memory/addressing.cpp::lower_memory_gep_inst`:
dynamic `i8` GEP from a runtime pointer currently requires a named dynamic
offset to already be `i64`; the AArch64 `va_arg` path supplies an `i32`
loaded offset such as the `__gr_offs` field, so the GEP does not publish the
resulting `PointerAddress`/value alias fact and reports `gep local-memory
semantic family`.

`00216.c` fails later in `foo` on aggregate loads through pointer-derived
subobject addresses, e.g. `getelementptr` from parameter pointers followed by
`load %struct.S` / `load %struct.T` / `load %struct.in6_addr`. The owning
surface is `src/backend/bir/lir_to_bir/memory/local_slots.cpp::lower_memory_load_inst`:
aggregate load admission currently accepts local aggregate slots and globals,
but not a `PointerAddressMap` runtime aggregate/subobject view already
published by GEP. That leaves the semantic route without a prepared aggregate
copy/load fact and reports `load local-memory semantic family`.

Shared classification: both representatives are missing structured
local-memory handoff over runtime pointer-address facts. They are not the same
single instruction shape: `00204.c` needs dynamic byte-offset GEP to publish a
runtime `PointerAddress` from an `i32` offset, while `00216.c` needs aggregate
load lowering to consume an existing runtime `PointerAddress` as an aggregate
copy source.

## Suggested Next

Execute Step 2 from `plan.md` in the smallest semantic slice: repair
`lower_memory_gep_inst` so dynamic `i8` runtime-pointer GEP with a named
non-`i64` integer offset, especially the AArch64 `va_arg` `i32` offset, is
widened or otherwise normalized into a valid BIR pointer-add and publishes the
same `PointerAddress`/value alias fact as the existing `i64` lane. Keep this
general to typed integer offsets; do not match `00204.c`, `myprintf`, or
`__va_list_tag_`.

Use this focused proof command:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

## Watchouts

- This owner is semantic admission/prepared-handoff only. Do not fold in
  AArch64 printer, assembler, linker, runtime, timeout, direct-call shuffle,
  direct vararg, address-of-local, `00164.c`, or `00214.c` residuals without a
  separate split.
- Do not reopen closed ideas 297, 298, or 311 from pass counts alone.
- `00204.c` and `00216.c` are adjacent but not identical repairs. If the GEP
  publication slice advances `00204.c` while `00216.c` still fails with the
  same load diagnostic, the next coherent packet is aggregate load consumption
  from `PointerAddressMap` in `lower_memory_load_inst`, not an expectation or
  runner change.
- Existing `00204.c` x86 semantic/prepared dump helpers pass because they do
  not exercise the AArch64 expanded `va_arg` GEP lane. They are still useful
  guardrails, but they do not prove the AArch64 `myprintf` failure path.
- The existing untracked `review/311_aggregate_stack_copy_review.md` was left
  untouched.

## Proof

Ran the delegated proof:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

Result: build succeeded; 6 focused backend dump/notes tests passed; the two
representative c-testsuite AArch64 tests still fail with the localized
semantic diagnostics:

- `c_testsuite_aarch64_backend_src_00204_c`: `semantic lir_to_bir function
  'myprintf' failed in gep local-memory semantic family`
- `c_testsuite_aarch64_backend_src_00216_c`: `semantic lir_to_bir function
  'foo' failed in load local-memory semantic family`

Proof log: `test_after.log`.
