Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 prepared RV64 FPR `bir.fptrunc double -> float` lowering
for explicit prepared FPR register homes.

The existing prepared floating-cast lowerer now admits both `FPExt F32 -> F64`
and `FPTrunc F64 -> F32` when the source and destination values resolve to
prepared RV64 FPR register homes. Unsupported floating casts still report a
structured `unsupported_floating_cast` diagnostic naming the supported pairs.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

## Suggested Next

Run Plan Step 4 representative validation for `src/20030125-1.c` to confirm the
old `fptrunc` boundary advanced and to classify the next boundary as another
FPR cast, a floating call ABI edge, or a runtime mismatch.

Suggested representative proof:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- The FPR cast lowerer requires `PreparedTargetRegisterIdentity` to identify
  RV64 FPR physical registers; it does not infer FPR numbers from names alone.
- This packet intentionally did not implement floating calls, libm behavior, or
  stack-slot FPR materialization.
- Non-supported floating cast pairs and non-FPR homes should continue to fail
  through the structured floating-cast diagnostic instead of widening into
  implicit moves or testcase-specific behavior.

## Proof

Delegated proof:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

Result: passed. The focused object-emission test now covers `fcvt.d.s` for
prepared `FPExt F32 -> F64`, `fcvt.s.d` for prepared `FPTrunc F64 -> F32`, and
the retained unsupported floating-cast diagnostic.

Proof log: `test_after.log`.
