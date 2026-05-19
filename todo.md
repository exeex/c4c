Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 localization complete for the new `00204.c` `stdarg`
scalar/local-memory residual after the pointer-phi repair.

The exact first-bad fact is the first fixed-size aggregate staging memcpy in
`stdarg`:

```llvm
%t3 = load %struct.s9, ptr @s9
%t4 = alloca [2 x i64]
call void @llvm.memcpy.p0.p0.i64(ptr %t4, ptr @s9, i64 9, i1 false)
```

The failing lowering function is
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp::lower_memory_memcpy_inst`.
It rejects this direct LIR `memcpy` before reaching
`try_lower_immediate_local_memcpy` because the direct path currently requires
both `dst` and `src` operands to be SSA values; here `dst=%t4` is SSA but
`src=@s9` is a global. The later `load [2 x i64], ptr %t4` and variadic
`myprintf` call are not yet the first bad facts.

Classification: this remains in idea 312 scope. It is still semantic
`lir_to_bir` local-memory admission/prepared-handoff work for the representative
`00204.c` path, before generated AArch64 artifacts exist. No lifecycle split is
needed for this residual.

## Suggested Next

Execute the next Step 2 semantic repair: teach direct `LirMemcpyOp` lowering to
admit a global source operand for immediate local-memory copies when the
destination is an SSA local slot/view and the global has structured linear
addressing facts. Preserve existing fail-closed behavior for unsupported global
storage, volatile copies, non-immediate sizes, and unrelated runtime `memcpy`
calls.

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
- The first bad fact is the direct intrinsic memcpy with global source
  `src=@s9`, not the following `load [2 x i64], ptr %t4` or the later variadic
  `myprintf` call.
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
  failed in scalar/local-memory semantic family`; localized first bad fact is
  `lower_memory_memcpy_inst` rejecting
  `llvm.memcpy.p0.p0.i64(ptr %t4, ptr @s9, i64 9, false)` because the direct
  memcpy path does not admit a global source operand
- `c_testsuite_aarch64_backend_src_00216_c`: now reports `semantic lir_to_bir
  function 'test_zero_init' failed in alloca local-memory semantic family`

Proof log: `test_after.log`.
