# Current Packet

Status: Active
Source Idea Path: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Scalar Object Rejections

## Just Finished

- Step 1 inspected the scalar object-route rejections from the failed 22/28
  scan proof without implementation edits.
- RV64 target-owned seam:
  `src/backend/mir/riscv/codegen/object_emission.cpp` rejects before ELF
  writing in `prepared_function_to_object_function(...)`, reached through
  `build_rv64_prepared_text_object_module(...)` and
  `write_rv64_prepared_relocatable_elf_object(...)`.
- RV64 rejecting checks:
  `prepared_function_to_object_function(...)` currently rejects variadic
  functions, any params, any local slots, atomic ops, multi-block functions,
  and any non-`bir::CallInst` instruction. Its call fragment path only accepts
  direct void calls with no args and no result. Its return path only accepts
  void returns or immediate zero integer returns. Backend facade code maps this
  null object-module result to `RISC-V backend object route unsupported prepared
  module shape`.
- RV64 representative prepared shapes:
  `return_add.c` has `add_three(i32 %p.x)` with `%t0 = bir.add i32 %p.x, 3`
  and `bir.ret i32 %t0`; `main` calls `add_three(i32 2)` with an immediate
  arg in `a0` and a result returned from `a0` into `%t0`. This trips params,
  non-call instruction, call arg/result, and value-return rejection.
- RV64 representative prepared shapes:
  `two_arg_helper.c` has `add_pair(i32 %p.x, i32 %p.y)` with params in `a0`
  and `a1`, `%t0 = bir.add i32 %p.x, %p.y`, and a return move from `%t0` to
  ABI `a0`; `main` calls it with immediate args `5` and `7` and a result. This
  needs parameter, two-arg call, add, and value-return support.
- RV64 representative prepared shapes:
  `return_add_sub_chain.c` has no params, calls, or locals; `main` contains
  `bir.add i32 2, 3`, `bir.sub i32 %t0, 1`, prepared storage `%t0 = 5` and
  `%t1 = 4`, and a return move from `%t1` to ABI `a0`. It is the smallest
  candidate because the prepared result is rematerializable as an immediate
  return, while the current object predicate rejects the non-call ALU
  instructions and nonzero value return.
- RV64 representative prepared shapes:
  `local_temp.c` has a local slot `%lv.x`, frame size 4, store/load frame-slot
  accesses, `%t0` in `t0`, and a return move from `%t0` to ABI `a0`. This
  needs frame-slot load/store support and should stay out of the first slice.
- AArch64 target-owned seam:
  `src/backend/mir/aarch64/codegen/object_emission.cpp` rejects selected
  machine instructions in `fragment_for_machine_instruction(...)`, reached
  through `object_function_from_machine(...)`,
  `build_aarch64_text_object_module(...)`, and the backend object facade.
- AArch64 rejecting dispatch:
  `fragment_for_machine_instruction(...)` currently emits only selected
  `ReturnInstructionRecord` as `ret` and selected direct calls as `bl` plus
  relocation. Other selected machine instructions return the diagnostic
  `unsupported AArch64 machine instruction for object emission`, which the
  backend surfaces as `AArch64 backend object route unsupported machine module
  shape: unsupported AArch64 machine instruction for object emission`.
- AArch64 representative instruction shapes:
  `return_add_sub_chain.c` lowers to scalar immediates and ALU operations:
  `mov w0, #2`, `add w0, w0, #3`, `sub w0, w0, #1`, `ret`. The object writer
  is missing encoding support for the selected move-immediate/call-boundary
  move shape and selected immediate `Add`/`Sub`; return already works.
- AArch64 representative instruction shapes:
  `return_add.c` includes `add w13, w0, #3`, call-boundary moves, stack frame
  setup/teardown, LR save/restore memory ops, `bl add_three`, and returns.
  Direct call and return already have object fragments; scalar ALU,
  call-boundary move, frame setup/teardown, and load/store encodings are the
  blocking gaps.

## Suggested Next

- Step 2 should take the smallest semantic implementation slice: extend the
  RV64 prepared object writer to emit nonzero integer immediate returns for
  prepared values that are already rematerialized as constants, proving
  `return_add_sub_chain.c` through the object route without params, calls,
  locals, globals, assembly fallback, or filename-specific handling.
- Step 2 owned files should be limited to
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.hpp` only if a test-visible
  helper is needed, `tests/backend/mir/backend_riscv_object_emission_test.cpp`,
  `tests/backend/CMakeLists.txt`, and `todo.md`.
- Exact Step 2 proof command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_riscv64_return_zero_writes_elf_obj|backend_obj_runtime_rv64_return_zero|backend_obj_runtime_rv64_return_add_sub_chain|backend_rv64_runtime_return_add_sub_chain)$' --output-on-failure) > test_after.log 2>&1`
- Later slices should handle RV64 call args/results and params for
  `return_add.c`/`two_arg_helper.c`, RV64 frame-slot load/store for
  `local_temp.c`, and AArch64 selected move-immediate plus immediate
  `Add`/`Sub` encoding for `return_add_sub_chain.c` before the frame/call-heavy
  `return_add.c` object-byte scan.

## Watchouts

- Do not use textual assembly, external assemblers, or asm fallback to satisfy
  `--codegen obj`.
- Do not add expected-failure scan labels for the blocked scalar cases.
- Keep broad RV64 globals/data, x86 object output, c-testsuite defaults, object
  stdout, and object semantic-BIR mode out of this focused child.

## Proof

- Inspection-only packet; no build required.
- Passed: `git diff --check`.
