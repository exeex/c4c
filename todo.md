Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 first semantic repair complete: `lower_memory_gep_inst` now normalizes
named non-`i64` dynamic byte offsets in the raw `i8` runtime-pointer GEP lane
by sign-extending supported integer offsets to `i64`, then publishing the same
result value alias and `PointerAddress` fact as the existing `i64` lane.

The repair is type/shape based, not testcase based: it does not match
`00204.c`, `myprintf`, or `__va_list_tag_`. Existing unsupported forms remain
fail-closed when the offset is not a supported integer value.

Fresh proof shows `c_testsuite_aarch64_backend_src_00204_c` advanced past the
old `gep local-memory semantic family` diagnostic. Its current first bad fact
is now the adjacent `load local-memory semantic family` diagnostic in
`myprintf`. `c_testsuite_aarch64_backend_src_00216_c` remains at the known
`load local-memory semantic family` diagnostic in `foo`.

## Suggested Next

Execute the next Step 2 semantic slice: teach
`src/backend/bir/lir_to_bir/memory/local_slots.cpp::lower_memory_load_inst` to
consume an existing runtime `PointerAddressMap` aggregate/subobject view as an
aggregate load/copy source. This should target the shared current load
diagnostic for `00204.c` and `00216.c`, without expanding into AArch64 codegen,
printer, runner, expectation, or timeout behavior.

## Watchouts

- This owner is semantic admission/prepared-handoff only. Do not fold in
  AArch64 printer, assembler, linker, runtime, timeout, direct-call shuffle,
  direct vararg, address-of-local, `00164.c`, or `00214.c` residuals without a
  separate split.
- Do not reopen closed ideas 297, 298, or 311 from pass counts alone.
- `00204.c` and `00216.c` are now both on load-family residuals. The next
  packet should preserve that narrowed ownership and avoid reopening the fixed
  GEP lane unless new evidence points back there.
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

Result: build succeeded; 6 focused backend dump/notes tests passed. The two
representative c-testsuite AArch64 tests still fail, but `00204.c` advanced
past the old GEP local-memory diagnostic:

- `c_testsuite_aarch64_backend_src_00204_c`: now reports `semantic lir_to_bir
  function 'myprintf' failed in load local-memory semantic family`
- `c_testsuite_aarch64_backend_src_00216_c`: still reports `semantic
  lir_to_bir function 'foo' failed in load local-memory semantic family`

Proof log: `test_after.log`.
