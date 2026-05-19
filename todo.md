Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 `00216.c` alloca/local-memory residual localized. The concrete source
construct is `test_zero_init`'s zero-initialized local aggregate with a
zero-length array member:

```c
struct SEC { struct SEA a; int r[0]; };
struct SEC c = { .a.j = 5 };
```

HIR records `struct SEC size=20 align=4`, with `a` at offset 0 size 16 and
`r: int[0]` at offset 16 size 0. The emitted LLVM/LIR-shaped operation sequence
in `test_zero_init` is:

```llvm
%lv.b = alloca %struct.SEB, align 4
%lv.c = alloca %struct.SEC, align 4
%lv.d = alloca %struct.SED, align 4
call void @llvm.memset.p0.i64(ptr %lv.c, i8 0, i64 20, i1 false)
%t2 = getelementptr %struct.SEC, ptr %lv.c, i32 0, i32 0
%t3 = getelementptr %struct.SEA, ptr %t2, i32 0, i32 1
store i32 5, ptr %t3
```

Minimal probes show `%struct.SEB` with `int r[1]` lowers, while `%struct.SEC`
with `int r[0]` and `%struct.SED` with `int r[]` both fail before BIR dump with
the same `test_zero_init` alloca local-memory family diagnostic. The narrow
repair surface is semantic local-memory coverage for aggregate storage that has
zero-sized trailing array/flexible-array members: `declare_local_aggregate_slots`
and/or immediate local `memset` coverage need to represent or tolerate the tail
padding bytes between the scalar leaves and the aggregate storage size. This
remains inside idea 312 semantic local-memory prepared-handoff scope; it is not
an AArch64 printer/runtime issue.

## Suggested Next

Execute the narrow Step 2 semantic repair for zero-sized trailing aggregate
storage coverage. Keep the change fail-closed and general: handle local
aggregate storage where the declared layout size exceeds scalar leaf coverage
because of zero-length or flexible trailing array members, so immediate zero
fills such as `memset(ptr %lv.c, 0, 20)` can lower without requiring a
testcase-specific exception.

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
- `00216.c`'s visible ctest family is `alloca local-memory`, but the localized
  failing operation after `%lv.c` declaration is the immediate zero `memset`
  over 20 bytes of `%struct.SEC`; the scalar leaves only cover the 16-byte
  `struct SEA a` prefix because `int r[0]` contributes no scalar leaf at the
  tail.
- `%struct.SEB` with `int r[1]` is not the first bad shape; a minimal `SEB`
  probe lowers. `%struct.SEC` and `%struct.SED` minimal probes fail before BIR
  dump with the same local-memory diagnostic.
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
  family`; localized to `%struct.SEC` zero-length trailing array storage
  coverage for the immediate zero `memset`

Proof log: `test_after.log`.
