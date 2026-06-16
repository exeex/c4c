Status: Active
Source Idea Path: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Avoidable Or Invalid IR Shapes

# Current Packet

## Just Finished

Step 3 extended the explicit AAPCS64 HFA `va_arg` materialization in
`src/codegen/lir/hir_to_lir/call/vaarg.cpp` to fp128 long-double HFA payloads.
Classified AArch64 HFAs now all route through `emit_aarch64_vaarg_hfa`; the
fp128-only fallback that emitted raw LLVM aggregate `va_arg` was removed.

The delegated three-target proof shows `c_testsuite_src_00204_c` now advances
past the former AArch64 instruction-selection crash and reaches execution. It
still fails as `[RUNTIME_MISMATCH]`, with HFA stdarg output corruption visible
across long double, double, and float cases. The two previously repaired
non-fp128 targets still pass.

A temporary IR check of `/tmp/c4c_00204.ll` for 00204 found no remaining
`= va_arg` or raw LLVM `va_arg` instruction; `myprintf` contains explicit
`vaarg.hfa.*` materialization blocks instead. No expectation files, unsupported
classifications, `plan.md`, or source ideas were touched.

## Suggested Next

Investigate the residual AArch64 HFA stdarg runtime mismatch in 00204, starting
from the semantic boundary between variadic HFA argument save-area layout and
`emit_aarch64_vaarg_hfa` lane extraction/overflow accounting. The next packet
should avoid narrowing to 00204 by name and should compare the generated
register-save offsets/overflow transitions against Clang for mixed HFA arities.

## Watchouts

- The crash blocker is gone for fp128 HFA `va_arg`; the remaining issue is
  runtime semantics, not backend instruction selection.
- The mismatch is not fp128-only after execution starts: the 00204 output shows
  corrupted HFA long-double, double, and float stdarg lines under mixed arity
  pressure.
- The helper assumes each lane occupies a 16-byte FP register-save slot and
  copies stack payloads using aggregate size rounded by element alignment; the
  next repair should validate that against the caller-side variadic HFA
  save-area layout.

## Proof

Ran:

`(cmake --build build_debug && ctest --test-dir build_debug -R '^(c_testsuite_src_00204_c|llvm_gcc_c_torture_src_920625_1_c|llvm_gcc_c_torture_src_pr44575_c)$' --output-on-failure) 2>&1 | tee test_after.log`

Result: build succeeded. The focused CTest subset reported 2/3 passing:
`llvm_gcc_c_torture_src_920625_1_c` passed,
`llvm_gcc_c_torture_src_pr44575_c` passed, and
`c_testsuite_src_00204_c` failed with `[RUNTIME_MISMATCH]` after producing an
executable and runtime output. Proof log: `test_after.log`.

Additional non-canonical inspection:

`build_debug/c4cll --target aarch64-linux-gnu --codegen llvm -o /tmp/c4c_00204.ll tests/c/external/c-testsuite/src/00204.c && rg -n "= va_arg| va_arg " /tmp/c4c_00204.ll || true`

Result: no raw LLVM `va_arg` instruction remained in the temporary 00204 IR.
