Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 fixed-size local aggregate/array load repair tightened. `lower_memory_load_inst`
still admits an aggregate array load from a tracked local scalar array slot, but
the local-array aggregate-load helper now validates the source element slots
before declaration, then validates all declared target leaf slots and
source/target types before appending any load/store instructions. The remaining
pre-emission mutation is the required `declare_local_aggregate_slots` call for
the result aggregate; no BIR instructions or aggregate alias are emitted until
all post-declaration leaf checks pass.

Fresh proof shows `c_testsuite_aarch64_backend_src_00204_c` advanced past both
the direct global-source memcpy staging copy and the following fixed-size local
aggregate load:

```llvm
%t3 = load %struct.s9, ptr @s9
%t4 = alloca [2 x i64]
call void @llvm.memcpy.p0.p0.i64(ptr %t4, ptr @s9, i64 9, i1 false)
```

```llvm
%t5 = load [2 x i64], ptr %t4
```

The new `00204.c` first-bad fact is no longer semantic `lir_to_bir`. The
AArch64 assembly route reaches the machine-node printer and fails in
`f128_transport`: `target instruction spelling failed at function 0 block 0
instruction 1: cannot print AArch64 machine node family=f128_transport
opcode=f128_transport: f128 transport printer requires structured full-width
q-register authority`. `c_testsuite_aarch64_backend_src_00216_c` remains at
`semantic lir_to_bir function 'test_zero_init' failed in alloca local-memory
semantic family`.

## Suggested Next

Stop treating `00204.c` as blocked on Step 2 semantic local-memory admission:
it now reaches an AArch64 printer-side `f128_transport` residual, which is
outside this packet's owned files and the current local-memory prepared-handoff
scope. The remaining in-scope semantic representative is still `00216.c`
`test_zero_init` in the alloca local-memory semantic family, unless the
supervisor splits the new `f128_transport` printer residual into a separate
lifecycle item.

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
- The fixed-size local aggregate load `load [2 x i64], ptr %t4` is no longer
  the first bad fact after this slice.
- The current `00204.c` first bad fact is in AArch64 machine-node printing for
  `f128_transport`, not semantic local-memory admission or prepared handoff.
- The fixed-size local aggregate/array load path now plans all load/store pairs
  before emitting instructions; keep it fail-closed on source coverage, target
  leaf coverage, and source/target type mismatches.
- Existing `00204.c` x86 semantic/prepared dump helpers pass because they do
  not exercise the AArch64 expanded `va_arg` GEP lane. They are still useful
  guardrails, but they do not prove the AArch64 `myprintf` failure path.
- The existing untracked `review/311_aggregate_stack_copy_review.md` was left
  untouched.
- The existing untracked `review/312_global_memcpy_repair_review.md` was left
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

- `c_testsuite_aarch64_backend_src_00204_c`: advanced past the direct
  global-source memcpy residual and `load [2 x i64], ptr %t4`; it now reaches
  the machine-node printer and fails on `family=f128_transport`
- `c_testsuite_aarch64_backend_src_00216_c`: still reports `semantic
  lir_to_bir function 'test_zero_init' failed in alloca local-memory semantic
  family`

Proof log: `test_after.log`.
