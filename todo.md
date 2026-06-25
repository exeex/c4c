Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 representative validation for `src/20030125-1.c` after
prepared RV64 FPR `bir.fpext float -> double` lowering.

The old generic `unsupported_instruction_fragment` / first-instruction
`bir.fpext` boundary is gone. The representative now reaches the later
structured floating-cast boundary:

```text
unsupported_floating_cast: RV64 object route supports only prepared F32-to-F64 FPR register casts
```

Changed files:
- `todo.md`

## Suggested Next

Implement the next narrow prepared FPR cast packet under Plan Step 4:
`bir.fptrunc double -> float` for explicit FPR register homes, or keep the
current `unsupported_floating_cast` diagnostic if the route deliberately defers
that support. Touch only `src/backend/mir/riscv/codegen/object_emission.cpp`,
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.

Suggested focused proof for an implementation packet:

```sh
cmake --build build --target backend_riscv_object_emission_test && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure > test_after.log
```

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- This validation packet intentionally did not implement `fptrunc`, floating
  calls, libm behavior, or stack-slot FPR materialization.
- The FPR cast lowerer requires `PreparedTargetRegisterIdentity` to identify
  RV64 FPR physical registers; it does not infer FPR numbers from names alone.
- The representative failure is inside the Step 4 FPR cast/call ABI bucket, but
  this packet is validation-only and must not edit implementation files.
- Re-run the `src/20030125-1.c` representative after any `fptrunc` slice to
  see whether the next boundary is another floating cast, a floating call ABI
  edge, or runtime mismatch.

## Proof

Delegated proof:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: exited `1`. This is acceptable for the validation-only packet because
the old generic unsupported-instruction / `fpext` boundary advanced to the
later structured `unsupported_floating_cast` boundary.

Proof log: `test_after.log`.
Representative case log:
`build/rv64_gcc_c_torture_backend/src_20030125-1.c/case.log`.
