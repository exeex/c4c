# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Scalar Prepared Object Support

## Just Finished

- Step 2 extended RV64 prepared object emission for the simple same-module
  scalar call shape needed by `return_add.c`.
- `src/backend/mir/riscv/codegen/object_emission.cpp` now supports GPR-backed
  parameter homes, register/immediate `Add` and `Sub` fragments, small
  immediate call argument setup, direct call relocation with a nonzero fragment
  offset, call result register moves, register-valued returns, and a minimal
  call frame that saves/restores `ra` for functions containing calls.
- The implementation remains target-owned and fail-closed: it still requires
  small imm12 integer immediates, GPR register homes, direct fixed-arity calls,
  no outgoing stack argument area, no memory return, and no local/frame-slot or
  global/data object support.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` now covers the
  prepared `add_three`/`main` shape directly, including parameter in `a0`,
  immediate arg `2`, result from `a0` to `t0`, `ra` frame setup, and same-module
  call relocation placement.
- `tests/backend/CMakeLists.txt` restored
  `backend_obj_runtime_rv64_return_add`; it links the compiler-produced `.o`
  and runs under qemu with expected return code 5.

## Suggested Next

- Continue Step 2 with the next RV64 scalar call candidate,
  `two_arg_helper.c`, by extending the same prepared-call path to multiple
  small immediate GPR args and proving/restoring only
  `backend_obj_runtime_rv64_two_arg_helper` if it falls out cleanly.
- If the supervisor chooses to leave further RV64 widening for later, Step 3's
  next small AArch64 packet remains selected move-immediate plus immediate
  `Add`/`Sub` object encoding for `return_add_sub_chain.c`.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- `local_temp.c` remains out of scope until RV64 object emission has explicit
  frame-slot load/store support.
- `two_arg_helper.c` is not restored in this packet; restore it only after a
  focused proof confirms the multi-arg prepared call shape.
- AArch64 object emission is unchanged in this packet; its next small candidate
  is still `return_add_sub_chain.c`, not the frame/call-heavy `return_add.c`.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_cli_riscv64_return_zero_writes_elf_obj|backend_obj_runtime_rv64_return_zero|backend_obj_runtime_rv64_return_add|backend_obj_runtime_rv64_return_add_sub_chain|backend_rv64_runtime_return_add|backend_rv64_runtime_return_add_sub_chain)$' --output-on-failure) > test_after.log 2>&1`
- Result: 7/7 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
