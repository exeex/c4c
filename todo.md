Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 runtime aggregate-load repair complete: `lower_memory_load_inst` now
consumes an existing runtime `PointerAddressMap` aggregate/subobject view as an
aggregate load source. It declares result aggregate slots, copies each scalar
leaf from the prepared pointer-value address into the result slots, and
publishes the aggregate result alias. The repair is shape/type based and does
not match `00204.c`, `00216.c`, `myprintf`, or `foo`.

Fresh proof shows `c_testsuite_aarch64_backend_src_00216_c` advanced past the
old `load local-memory semantic family` diagnostic in `foo`. Its current first
bad fact is now `semantic lir_to_bir function 'test_zero_init' failed in alloca
local-memory semantic family`.

`c_testsuite_aarch64_backend_src_00204_c` remains on a load-family residual,
but the localized missing fact is now more precise: in `myprintf`, the failing
operation is `load ptr, ptr %t244`, where `%t244` is a phi of AArch64
expanded-`va_arg` register and stack paths and has no `PointerAddressMap` entry.
That phi-level runtime pointer-address preservation must happen before the
follow-on aggregate memcpy/load source can be consumed.

## Suggested Next

Execute the next Step 2 semantic slice: preserve/merge runtime
`PointerAddressMap` facts across same-shaped pointer phi nodes so the
`00204.c` AArch64 expanded-`va_arg` phi `%t244` remains addressable for the
subsequent pointer load. Keep the owner in semantic `lir_to_bir`; do not expand
into AArch64 codegen, printer, runner, expectation, or timeout behavior.

## Watchouts

- This owner is semantic admission/prepared-handoff only. Do not fold in
  AArch64 printer, assembler, linker, runtime, timeout, direct-call shuffle,
  direct vararg, address-of-local, `00164.c`, or `00214.c` residuals without a
  separate split.
- Do not reopen closed ideas 297, 298, or 311 from pass counts alone.
- `00216.c` is no longer on the load-family residual after this slice; its next
  observed residual is alloca-family in `test_zero_init`.
- `00204.c` is still on load-family, but not because the new aggregate load
  consumer rejected an existing map. `%t244` has no runtime pointer-address map
  entry after phi lowering.
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
representative c-testsuite AArch64 tests still fail:

- `c_testsuite_aarch64_backend_src_00204_c`: still reports `semantic
  lir_to_bir function 'myprintf' failed in load local-memory semantic family`;
  localized first bad fact is `load ptr, ptr %t244` with no `PointerAddressMap`
  entry for phi `%t244`
- `c_testsuite_aarch64_backend_src_00216_c`: now reports `semantic lir_to_bir
  function 'test_zero_init' failed in alloca local-memory semantic family`

Proof log: `test_after.log`.
