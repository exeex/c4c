# Current Packet

Status: Active
Source Idea Path: ideas/open/582_rv64_va_start_stack_backed_destination.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Representative Route Proof

## Just Finished

Step 4 from `plan.md` is complete.

Reran the retained representative
`tests/c/external/gcc_torture/src/va-arg-21.c` after the Step 3 helper
materialization repair.

Fresh route result:

- `cmake --build --preset default --target c4cll`: rc `0`
- prepared dump: rc `0`
- RV64 object route: rc `1`

The representative no longer stops at the old
`unsupported_variadic_helper_lowering` reason. It now advances to a downstream
unsupported instruction fragment:

`unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering; function=doit; block=entry; block_index=0; instruction_index=2; instruction_kind=CallInst; owner=ptr %t1`

Prepared BIR context identifies the new owner instruction as:

`%t1 = bir.call ptr malloc(i64 4)`

This is before the later `llvm.va_start.p0` helper calls at instruction indexes
`7` and `26`.

Prepared helper operands still show the intended stack-backed
destination-address shape:

- `block=0 inst=7`: `dst_va_list=%t1:stack_slot:slot=#32:offset=288`,
  `dst_va_list_addr=%t1:stack_slot:slot=#17:offset=136`
- `block=0 inst=26`: `dst_va_list=%t6:stack_slot:slot=#33:offset=296`,
  `dst_va_list_addr=%t6:stack_slot:slot=#18:offset=144`

The case-local f128 sections remain empty:

- `--- prepared-f128-carriers ---`
- `--- prepared-f128-runtime-helpers ---`

Artifacts are under
`build/agent_state/582_rv64_va_start_stack_backed_destination/step4/`,
including per-command `.cmd`, `.rc`, `.out`, `.err`, the prepared dump, and
`summary.md`.

## Suggested Next

Execute Step 5 from `plan.md`: run focused backend proof and the broader
backend subset selected by the supervisor, then record whether the source idea
is ready for plan-owner closure evaluation.

## Watchouts

- The representative still exits nonzero, but the remaining owner is a later
  ordinary call-lowering gap for `malloc`, not the old `va_start`
  destination-address helper owner.
- Preserve the semantic helper support and fail-closed checks from Step 3.
- Do not expand this plan into the downstream `malloc` call-lowering owner;
  that is outside the `va_start` destination-address boundary.
- Preserve unrelated open ideas, including untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`.

## Proof

Proof log: `test_after.log`.

Commands:

- `cmake --build --preset default --target c4cll`
- `build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c`
- `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-21.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/582_rv64_va_start_stack_backed_destination/step4/src_va-arg-21.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/582_rv64_va_start_stack_backed_destination/step4/src_va-arg-21.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/582_rv64_va_start_stack_backed_destination/step4/src_va-arg-21.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

Result: build and prepared dump passed; object route failed at the downstream
`unsupported_instruction_fragment` owner named above, proving advancement past
the old `unsupported_variadic_helper_lowering` owner.
