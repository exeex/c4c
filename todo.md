# Current Packet

Status: Active
Source Idea Path: ideas/open/582_rv64_va_start_stack_backed_destination.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Va Start Destination Owner

## Just Finished

Step 1 from `plan.md` is complete for `src/va-arg-21.c`.

Fresh proof rebuilt `c4cll`, regenerated the prepared dump, and reran the RV64
object route. The current owner remains
`unsupported_variadic_helper_lowering`, with the concrete route diagnostic:

`RV64 va_start helper requires destination va_list address in a prepared GPR home`

Route coordinates and prepared helper facts:

- Function: `doit`
- Helper: `va_start`
- Helper operand 1: `block=0 inst=7`,
  `dst_va_list=%t1:stack_slot:slot=#32:offset=288`,
  `dst_va_list_addr=%t1:stack_slot:slot=#17:offset=136`
- Helper operand 2: `block=0 inst=26`,
  `dst_va_list=%t6:stack_slot:slot=#33:offset=296`,
  `dst_va_list_addr=%t6:stack_slot:slot=#18:offset=144`
- Storage evidence: `%t1` and `%t6` address values are `encoding=frame_slot`
  GPR stack homes at `slot#17+stack136` and `slot#18+stack144`, not prepared
  GPR homes.
- Case-local `prepared-f128-carriers` and `prepared-f128-runtime-helpers`
  sections remain empty; f128-looking libc declarations are not the active
  owner.
- Code-path clue: the emitted diagnostic maps to the RV64 `va_start`
  destination-address home validation in
  `src/backend/mir/riscv/codegen/object_emission.cpp`.

## Suggested Next

Execute Step 2 from `plan.md`: add focused RV64 backend coverage for the
semantic `va_start` helper shape where the destination va_list address is
stack-backed and must not be matched by representative filename or source text.

## Watchouts

- Do not treat f128-looking libc declarations as the owner without prepared
  carrier/helper evidence.
- Do not match `src/va-arg-21.c` by filename, function, block, or source text.
- The focused test should model the semantic helper operands: `va_start`
  destination values in prepared stack slots and `dst_va_list_addr` values
  whose current homes are stack-backed instead of GPR materialized.
- Preserve unrelated open ideas, including untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`.

## Proof

Proof log: `test_after.log`.

Commands:

- `cmake --build --preset default --target c4cll`: passed, rc 0
- `build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c`: passed, rc 0
- RV64 object-route harness for `tests/c/external/gcc_torture/src/va-arg-21.c`:
  failed as expected for classification, rc 1, at the current
  `unsupported_variadic_helper_lowering` owner.

Artifacts:

- `build/agent_state/582_rv64_va_start_stack_backed_destination/step1/summary.md`
- `build/agent_state/582_rv64_va_start_stack_backed_destination/step1/src_va-arg-21.c/dump-prepared-bir.txt`
- `build/agent_state/582_rv64_va_start_stack_backed_destination/step1/src_va-arg-21.c/object-route.err`
