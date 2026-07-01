Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Reclassify Remaining Param-Home Rows

# Current Packet

## Just Finished

Step 7 of `plan.md` probed the three remaining `unsupported_param_home` bucket
rows and reclassified them using fresh prepared dumps plus RV64 object probes.

Artifacts:

- `build/agent_state/512_step7_remaining_rows/20010518-1/`
- `build/agent_state/512_step7_remaining_rows/pr27073/`
- `build/agent_state/512_step7_remaining_rows/pr69447/`

Row classification:

- `tests/c/external/gcc_torture/src/20010518-1.c` now consumes the stack-passed
  parameter contract. The prepared dump publishes caller ABI bindings for
  indexes 8-12 with stack offsets `0`, `8`, `16`, `24`, and `32`; callee
  homes `%p.i`, `%p.j`, `%p.k`, `%p.l`, and `%p.m` are stack-slot homes. The
  object probe no longer reaches `unsupported_param_home`; it fails at
  `unsupported_move_bundle_target_shape` in `add` before instruction 0 because
  generic stack-to-stack move bundles are still not materialized by RV64.
- `tests/c/external/gcc_torture/src/pr27073.c` now consumes the same contract.
  The prepared dump publishes caller ABI bindings for indexes 8-9 with stack
  offsets `0` and `8`; callee homes `%p.s4` and `%p.s5` are stack-slot homes.
  The object probe fails after the parameter-home gate at
  `unsupported_move_bundle_target_shape` in `foo` before instruction 1.
- `tests/c/external/gcc_torture/src/pr69447.c` now consumes the same contract.
  The prepared dump publishes caller ABI binding index 8 with stack offset `0`;
  callee home `%p.u64_3` is a stack-slot home. The object probe fails after the
  parameter-home gate at `unsupported_move_bundle_target_shape` in `foo` before
  instruction 14.

No focused backend coverage was added in this packet because the probed rows do
not expose a new same-contract producer gap. They all show the stack-parameter
contract is now present and consumed; the remaining failures belong to the
existing RV64 generic move-bundle/materialization family.

## Suggested Next

Step 8 should run the requested validation and handoff checks for idea 512,
including the focused stack-parameter subset and the broader backend subset the
supervisor selects.

## Watchouts

- `20001017-1.c` remains blocked by the existing `fpr:fs1` callee-saved
  stack-frame diagnostic, not by the stack-passed parameter-home consumer.
- `20010518-1.c`, `pr27073.c`, and `pr69447.c` are no longer parameter-home
  rows at current `HEAD`; their remaining blocker is
  `unsupported_move_bundle_target_shape` for stack-to-stack value moves.
- Follow-up idea candidate: RV64 prepared move-bundle materialization for
  generic stack-to-stack value moves, with care not to fold it into the
  stack-parameter-home source idea.
- Keep varargs, F128, memory aggregate, dynamic stack, and unrelated RV64
  instruction-lowering gaps out of this source idea unless the supervisor or
  plan owner creates a separate follow-up.

## Proof

- `cmake --build build --target c4cll` passed.
- For each row, prepared dumps and object probes were captured with:
  `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <source>` and
  `build/c4cll --codegen obj --target riscv64-linux-gnu <source> -o <row>.o`.
- Prepared dumps return rc 0 for all three rows.
- Object probes return rc 2 for all three rows at
  `unsupported_move_bundle_target_shape`, not `unsupported_param_home`.
- This was a probe and `todo.md`-only packet, so no CTest subset was run and
  `test_after.log` was not overwritten.
