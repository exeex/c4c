Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 runtime pointer-phi repair complete: scalar pointer phi lowering now
publishes a runtime `PointerAddressMap` entry for phi results when the incoming
pointer facts have the same address shape. Opaque runtime pointers with raw
`i8` spelling and plain pointer SSA lanes merge into an opaque phi-base address
fact, while typed/layout pointer facts still require matching shape and missing
structured lanes remain fail-closed.

Fresh proof shows `c_testsuite_aarch64_backend_src_00204_c` advanced past the
old `myprintf` load-family residual at `load ptr, ptr %t244`. The new observed
`00204.c` residual is later, in `stdarg`: `semantic lir_to_bir function
'stdarg' failed in scalar/local-memory semantic family`.

`c_testsuite_aarch64_backend_src_00216_c` remains at the previously observed
`semantic lir_to_bir function 'test_zero_init' failed in alloca local-memory
semantic family` residual.

## Suggested Next

Execute the next Step 2 localization slice for the new `00204.c` `stdarg`
scalar/local-memory residual. Start around the fixed-size aggregate staging
sequence in `stdarg` (`load %struct.s9`, `alloca [2 x i64]`, memcpy, then
`load [2 x i64]`) and determine whether this is still semantic local-memory
handoff or should be split away from idea 312.

## Watchouts

- This owner is semantic admission/prepared-handoff only. Do not fold in
  AArch64 printer, assembler, linker, runtime, timeout, direct-call shuffle,
  direct vararg, address-of-local, `00164.c`, or `00214.c` residuals without a
  separate split.
- Do not reopen closed ideas 297, 298, or 311 from pass counts alone.
- `00216.c` is no longer on the load-family residual after this slice; its next
  observed residual is alloca-family in `test_zero_init`.
- `00204.c` is no longer on the `myprintf` `load ptr, ptr %t244` load-family
  residual after this slice.
- The new `00204.c` residual is in `stdarg`, not `myprintf`; do not treat it as
  proof that the pointer-phi preservation failed.
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

- `c_testsuite_aarch64_backend_src_00204_c`: advanced past the old `myprintf`
  load-family residual and now reports `semantic lir_to_bir function 'stdarg'
  failed in scalar/local-memory semantic family`
- `c_testsuite_aarch64_backend_src_00216_c`: now reports `semantic lir_to_bir
  function 'test_zero_init' failed in alloca local-memory semantic family`

Proof log: `test_after.log`.
