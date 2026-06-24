# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add RV64 Scalar Prepared Object Support

## Just Finished

- Step 2 completed the first RV64 scalar prepared-object capability slice for
  `return_add_sub_chain.c`.
- `src/backend/mir/riscv/codegen/object_emission.cpp` now resolves prepared
  named values whose homes are `RematerializableImmediate`, allows pure
  value-producing BIR instructions only when their results are rematerialized
  immediates, and emits small signed imm12 integer returns as `addi a0, zero,
  imm` plus `ret`.
- The RV64 object path still rejects params, locals/frame slots, globals/data,
  call args/results, memory instructions, and immediate returns outside the
  current imm12 fragment. This packet did not add asm fallback or testcase-name
  matching.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` now has a
  prepared scalar chain fixture mirroring `%t0 = 5`, `%t1 = 4`, and `ret %t1`,
  proving the object module contains `addi a0, zero, 4` followed by `ret`
  without relocations.
- `tests/backend/CMakeLists.txt` restored
  `backend_obj_runtime_rv64_return_add_sub_chain` as a green object runtime
  scan case beside the existing asm runtime case.

## Suggested Next

- Continue Step 2 with RV64 same-module scalar call support for
  `return_add.c`: prepared fixed-arity direct call arg setup for small integer
  immediates, scalar result capture from ABI `a0`, simple parameter homes in
  `a0`, and value returns from prepared register/immediate homes. Restore
  `backend_obj_runtime_rv64_return_add` only after it links and runs green.
- After the RV64 call/param slice, either extend the same route to
  `two_arg_helper.c` or move to Step 3 AArch64 selected move-immediate plus
  immediate `Add`/`Sub` object encoding for `return_add_sub_chain.c`, depending
  on supervisor routing.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.
- `local_temp.c` remains out of scope until RV64 object emission has explicit
  frame-slot load/store support.
- AArch64 object emission is unchanged in this packet; its next small candidate
  is still `return_add_sub_chain.c`, not the frame/call-heavy `return_add.c`.

## Proof

- Passed:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_riscv64_return_zero_writes_elf_obj|backend_obj_runtime_rv64_return_zero|backend_obj_runtime_rv64_return_add_sub_chain|backend_rv64_runtime_return_add_sub_chain)$' --output-on-failure) > test_after.log 2>&1`
- Result: 7/7 tests passed.
- Proof log: `test_after.log`.
- Passed: `git diff --check`.
