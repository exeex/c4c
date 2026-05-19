Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 reviewer-blocking fail-closed fixes complete for the current direct
global-source memcpy repair. Direct `LirMemcpyOp` lowering still admits an
immediate local-memory copy from a structured global source into an SSA local
destination slot/view when the source global has
`GlobalInfo::supports_linear_addressing` and enough storage bytes, but rejected
`@global` sources now return the global-specific route result directly instead
of falling through to generic pointer-value memcpy. `append_global_symbol_to_leaf_view`
now preflights the full chunk plan before emitting, so unsupported leaf coverage
cannot partially mutate `lowered_insts` before returning `false`.

Fresh proof still shows `c_testsuite_aarch64_backend_src_00204_c` advanced past
the old `stdarg` scalar/local-memory residual at the first fixed-size aggregate
staging memcpy:

```llvm
%t3 = load %struct.s9, ptr @s9
%t4 = alloca [2 x i64]
call void @llvm.memcpy.p0.p0.i64(ptr %t4, ptr @s9, i64 9, i1 false)
```

The new `00204.c` first-bad fact is the immediately following local load:

```llvm
%t5 = load [2 x i64], ptr %t4
```

That reports `semantic lir_to_bir function 'stdarg' failed in load
local-memory semantic family`. The missing fact is semantic admission for a
fixed-size aggregate/array load from an SSA local destination slot/view that was
just populated by the repaired immediate memcpy. `c_testsuite_aarch64_backend_src_00216_c`
remains at `semantic lir_to_bir function 'test_zero_init' failed in alloca
local-memory semantic family`.

## Suggested Next

Execute the next Step 2 semantic repair: teach
`lower_memory_load_inst`/local-memory load admission to consume the fixed-size
aggregate/array local slot/view produced by immediate memcpy staging, so
`load [2 x i64], ptr %t4` can publish an aggregate/scalarized value for the
following variadic call without c-testsuite-specific matching.

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
- The direct intrinsic memcpy with global source `src=@s9` is no longer the
  first bad fact after this slice.
- The current first bad fact is `load [2 x i64], ptr %t4`, before the later
  variadic `myprintf` call.
- Existing `00204.c` x86 semantic/prepared dump helpers pass because they do
  not exercise the AArch64 expanded `va_arg` GEP lane. They are still useful
  guardrails, but they do not prove the AArch64 `myprintf` failure path.
- The existing untracked `review/311_aggregate_stack_copy_review.md` was left
  untouched.
- Direct `@global` memcpy rejection is now intentionally fail-closed. Do not
  reintroduce pointer-value fallback for a source operand that begins with `@`.
- Global-to-leaf memcpy emission is intentionally atomic: keep full coverage
  planning before appending load/store instructions to `lowered_insts`.

## Proof

Ran the delegated proof:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

Result: build succeeded; 6 focused backend dump/notes tests passed. The two
representative c-testsuite AArch64 tests still fail:

- `c_testsuite_aarch64_backend_src_00204_c`: advanced past the old `myprintf`
  load-family residual and the direct global-source memcpy residual; it now
  reports `semantic lir_to_bir function 'stdarg' failed in load local-memory
  semantic family` at `load [2 x i64], ptr %t4`
- `c_testsuite_aarch64_backend_src_00216_c`: still reports `semantic
  lir_to_bir function 'test_zero_init' failed in alloca local-memory semantic
  family`

Proof log: `test_after.log`.
