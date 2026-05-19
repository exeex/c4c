Status: Active
Source Idea Path: ideas/open/312_aarch64_lir_to_bir_local_memory_prepared_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Semantic Admission Or Prepared Handoff

# Current Packet

## Just Finished

Step 2 zero-sized/flexible trailing aggregate storage repair complete. Local
aggregate slot declaration now permits zero-sized array layouts to contribute
no scalar leaves instead of rejecting the aggregate, while still rejecting
invalid layouts, missing alignment, and zero-sized non-array layouts. Immediate
local `memset` lowering now preflights all scalar leaf stores before emission
and allows only zero-byte fills to cover an unrepresented trailing suffix.

This repairs the concrete `00216.c` `test_zero_init` zero-initialized local
aggregate with a zero-length array member:

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

Focused proof shows `00216.c` now advances past semantic `lir_to_bir` and
prepared handoff to backend assembly validation. The new `00216.c` first-bad
fact is out-of-scope AArch64 stack-offset assembly:

```asm
ldr x13, [sp, #1644]
```

The assembler reports `index must be an integer in range [-256, 255]` at
`build/c_testsuite_aarch64_backend/src/00216.c.s:1514:19`.

## Suggested Next

Stop treating `00216.c` as blocked on Step 2 semantic local-memory admission:
it now reaches backend assembly validation and fails on an AArch64 stack offset
range issue. The remaining `00204.c` representative also remains out-of-scope
for Step 2 semantic local-memory, at `f128_transport` printer authority.

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
- `00216.c` no longer fails in `alloca local-memory`; the current failure is
  backend AArch64 assembly validation for an out-of-range `[sp, #1644]` load.
- The zero-sized/flexible trailing aggregate repair is intentionally limited:
  zero-count arrays may contribute no scalar local slots, and only zero-filled
  unrepresented trailing bytes may be skipped by immediate local memset.
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
- `c_testsuite_aarch64_backend_src_00216_c`: advanced past semantic
  `lir_to_bir` and prepared handoff; it now fails backend assembly validation
  at `build/c_testsuite_aarch64_backend/src/00216.c.s:1514:19` with
  `ldr x13, [sp, #1644]` because the immediate stack offset exceeds the
  accepted range

Proof log: `test_after.log`.
