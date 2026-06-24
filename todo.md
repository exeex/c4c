# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Scalar Prepared Object Support

## Just Finished

- Step 2 restored and proved the RV64 `two_arg_helper.c` object runtime scan
  case.
- The existing scalar call support handled multiple small immediate GPR call
  args in `main`; proof exposed one real callee-side gap for `add_pair`: the
  body uses register-register `bir.add i32 %p.x, %p.y` with params homed in
  `a0` and `a1`.
- `src/backend/mir/riscv/codegen/object_emission.cpp` now has a minimal RV64
  R-type encoder and emits register-register `add` for GPR-backed prepared
  scalar values. Other binary op families remain unsupported unless already
  covered by the previous register/immediate slice.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` now covers the
  two-arg prepared object shape directly, including `add t0, a0, a1`, immediate
  args `5`/`7`, direct same-module call relocation placement, and the object
  text layout.
- `tests/backend/CMakeLists.txt` restored
  `backend_obj_runtime_rv64_two_arg_helper`; it links the compiler-produced
  `.o` and runs under qemu with expected return code 12.

## Suggested Next

- Continue Step 2 only if the supervisor wants the next RV64 candidate:
  inspect/prove `local_temp.c`, which is blocked on explicit frame-slot
  load/store support rather than scalar call support.
- Otherwise move to Step 3: AArch64 selected move-immediate plus immediate
  `Add`/`Sub` object encoding for `return_add_sub_chain.c`.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- `local_temp.c` remains out of scope until RV64 object emission has explicit
  frame-slot load/store support.
- `two_arg_helper.c` is now restored and green; do not broaden this result to
  local/frame-slot argument variants without a separate proof.
- AArch64 object emission is unchanged in this packet; its next small candidate
  is still `return_add_sub_chain.c`, not the frame/call-heavy `return_add.c`.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_obj_runtime_rv64_return_add|backend_obj_runtime_rv64_two_arg_helper|backend_rv64_runtime_two_arg_helper)$' --output-on-failure) > test_after.log 2>&1`
- Result: 4/4 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
