# Current Packet

Status: Active
Source Idea Path: ideas/open/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Local-Call And RegReg Object Rejections

## Just Finished

- Completed Step 1 inspection for child 337 without implementation or test
  edits.
- RV64 rejection seam is target-owned in
  `src/backend/mir/riscv/codegen/object_emission.cpp`:
  `prepared_function_to_object_function(...)` rejects any function that has both
  a call and a nonzero prepared frame with `if (has_call && *stack_frame_bytes >
  0) return std::nullopt;`, and `fragment_for_prepared_call(...)` currently
  accepts only `PreparedStorageEncodingKind::Immediate` call arguments.
- RV64 prepared record shapes for the candidate cases:
  `two_arg_local_arg.c` has `main` frame size 4, one frame slot, call
  `add_pair` at block 0 inst 2, arg0 `source_encoding=register`
  `t0 -> a0`, arg1 immediate `7 -> a1`, result `a0 -> s1`;
  `two_arg_second_local_arg.c` has frame size 4, one frame slot, arg0
  immediate `5 -> a0`, arg1 register `t0 -> a1`, result `a0 -> s1`;
  `two_arg_both_local_arg.c` has frame size 8, two frame slots, arg0 register
  `t0 -> a0`, arg1 register `s1 -> a1`, result `a0 -> s2`;
  `local_arg_call.c` has frame size 4, one frame slot, call `add_one` at block
  0 inst 2, arg0 register `t0 -> a0`, result `a0 -> s1`.
- The RV64 local-store/load pieces are already represented as supported
  frame-slot accesses for these cases: stores and loads use default address
  space, frame slots, offset 0 or 4, size 4, align 4, and base-plus-offset.
  The missing object capability is composing local frame emission with call
  frame preservation and accepting register-sourced GPR call arguments.
- AArch64 rejection seam is target-owned in
  `src/backend/mir/aarch64/codegen/object_emission.cpp`:
  `fragment_for_machine_instruction(...)` dispatches scalar records through
  `scalar_add_sub_immediate_fragment(...)`, which only accepts add/sub where
  the RHS is an immediate. `two_arg_helper.c` generates `add w13, w0, w1`,
  then `mov x0, x13`, so object output fails with
  `AArch64 object emission supports only selected immediate add/sub scalar
  instructions`.
- The first minimal implementation slice should be RV64-first: support a
  same-module scalar call in a function with a small prepared local frame and
  one or more register-sourced GPR call arguments, then restore the four RV64
  object-runtime cases. Leave AArch64 register-register ALU for Step 3.

## Suggested Next

- Step 2 owned files: `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.hpp` only if a declaration is
  needed, `tests/backend/mir/backend_riscv_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Implement RV64 object support by extending the existing prepared route, not
  by adding filename/function matching: allow the call-frame path to coexist
  with a small local frame, preserve `ra` at a non-overlapping stack offset,
  continue using prepared frame-slot load/store fragments for local temps, and
  let `fragment_for_prepared_call(...)` materialize register-sourced GPR
  arguments with `append_rv64_move(destination_arg_reg, source_reg)` alongside
  the existing immediate argument path.
- Restore the RV64 object-runtime tests beside their asm counterparts:
  `backend_obj_runtime_rv64_two_arg_local_arg`,
  `backend_obj_runtime_rv64_two_arg_second_local_arg`,
  `backend_obj_runtime_rv64_two_arg_both_local_arg`, and
  `backend_obj_runtime_rv64_local_arg_call`.
- Exact recommended Step 2 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_obj_runtime_rv64_(return_zero|return_add|two_arg_helper|return_add_sub_chain|local_temp|two_arg_local_arg|two_arg_second_local_arg|two_arg_both_local_arg|local_arg_call)|backend_rv64_runtime_(two_arg_local_arg|two_arg_second_local_arg|two_arg_both_local_arg|local_arg_call))$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Do not add expected-failure scan labels or weaken object/asm expectations.
- RV64 must not regress the already-green no-local call cases:
  `return_add.c` and `two_arg_helper.c`; they currently rely on same-module
  direct calls, immediate arguments, register results, and no local frame.
- `two_arg_both_local_arg.c` uses a callee-saved register source (`s1`) as a
  call argument and stores the result in another callee-saved register (`s2`);
  Step 2 should avoid clobbering these when adding the combined call/local
  stack layout.
- AArch64 `two_arg_helper.c` is not a Step 2 dependency. Its focused gap is a
  selected register-register scalar add encoding for `add w13, w0, w1`; the
  existing call-boundary move and direct-call object support already covers the
  surrounding `mov` and `bl` records.
- Keep RV64 globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, AArch64 runtime, x86 object
  output, object stdout, c-testsuite object defaults, and semantic-BIR object
  mode out of this child.
- Parent idea 334 should resume only after this child restores the next object
  scan candidates as green native object-output tests.

## Proof

- Inspection-only packet; no build required by the delegated proof.
- Used `c4c-clang-tool-ccdb` symbol listing for target object-emitter files and
  `./build/c4cll --dump-prepared-bir` / `--codegen asm|obj` dry runs for
  candidate shape inspection. Dry runs wrote only `/tmp` outputs.
- Ran `git diff --check`.
