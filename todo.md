Status: Active
Source Idea Path: ideas/open/352_aarch64_block_label_emission_ordering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Block Emission Boundary

# Current Packet

## Just Finished

Plan Step 1 completed: localized the `00176` `partition` boundary to AArch64
conditional-terminator dispatch losing `block_2`'s prepared successors before
shared MIR label printing, not to return lowering itself.

Prepared evidence gathered with:

`./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function partition tests/c/external/c-testsuite/src/00176.c`

Key prepared facts:

- `partition` prepared block order is `entry`, `for.cond.2`, `for.latch.2`,
  `block_2`, `block_3`, `block_4`, `block_5`.
- `block_2` has `terminator=cond_branch true=block_4 false=block_5`.
- `block_3` has `terminator=return`.
- `block_4` has `terminator=branch target=block_5`.
- `block_5` has `terminator=branch target=for.latch.2`.

Generated evidence gathered with:

`./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00176.c -o /tmp/c4c_00176.s`

Generated `partition` emits the first loop branch correctly, then emits
`block_2` body without any conditional branch to `block_4`/`block_5`; assembly
falls directly into `.LBB90_4`, the `block_3` return block. After `ret`, the
later `block_4` `swap` body is emitted with no `.LBB90_6`-style label, then
branches to labeled `block_5` (`.LBB90_7`).

Exact owner boundary:

- `src/backend/mir/aarch64/codegen/traversal.cpp:64`
  `lower_prepared_functions` preserves prepared block order and calls
  `dispatch_prepared_block` once per prepared block.
- `src/backend/mir/aarch64/codegen/dispatch.cpp:6324`
  `dispatch_prepared_block` owns terminator lowering and only records
  `block.successors = make_conditional_branch_successors(context)` when a
  conditional branch instruction is actually returned by one of the conditional
  lowering helpers.
- For `00176` `partition` `block_2`, no conditional terminator instruction is
  emitted, so no `block_2 -> block_4/block_5` successors are recorded.
- `src/backend/mir/printer.hpp:61` `print_machine_function` emits block labels
  only for labels present in the accumulated successor target set. Because
  `block_4` is only reached through the missing `block_2` successor edge, its
  label is omitted.
- `src/backend/mir/aarch64/codegen/prologue.cpp:156`
  `insert_prepared_frame_boundary_nodes` correctly inserts the epilogue before
  the `block_3` return instruction; the bad fact is that later reachable
  `block_4` code was left unlabeled/unreachable after that valid return block.

## Suggested Next

First implementation packet for Step 2/3: add focused AArch64 backend coverage
for a prepared multi-block function where a conditional branch block targets a
later block that follows a return block in prepared order. The test should fail
if the conditional branch instruction or successor metadata is missing, and
should assert that the later reachable block receives a label and is not emitted
as unlabeled post-return code.

Likely implementation files/helpers for the repair packet:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
  `dispatch_prepared_block`: fail closed or preserve prepared successor facts
  when conditional terminator lowering fails, rather than silently leaving a
  block without terminator/successors.
- `src/backend/mir/aarch64/codegen/comparison.cpp`
  `lower_prepared_conditional_branch_terminator`,
  `make_conditional_branch_successors`, and the fused/materialized branch
  helpers: determine why `partition` `block_2` does not produce a branch.
- `src/backend/mir/printer.hpp` `print_machine_function`: label policy is the
  visible emission boundary; consider only as a shared MIR contract repair if
  the chosen route is to label all non-entry blocks or all non-fallthrough
  blocks independent of successor-record loss.

## Watchouts

- Do not rework call-argument publication unless fresh evidence shows another
  stale caller-clobbered post-call argument use.
- Do not special-case `00176`, `partition`, one block id, one label suffix, or
  one emitted return sequence.
- Avoid fixing only the label printer while leaving a prepared conditional
  branch block with no emitted terminator; that would hide the reachability
  break instead of repairing the CFG-to-assembly handoff.
- Return/epilogue insertion is not the first bad owner for this packet:
  `block_3` is a real prepared return block, and the epilogue-before-`ret`
  placement is expected for that block.
- Keep `00181` out of this route unless its current stack-preserved
  symbol/local publication crash is fixed or bypassed by its owner and a fresh
  block-label first bad fact appears.

## Proof

Delegated proof to run for this localization packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_aarch64_return_lowering|backend_aarch64_function_traversal|backend_prepare_block_only_control_flow|c_testsuite_aarch64_backend_src_00176_c)$' | tee test_after.log`

Result: build had no work to do; CTest subset was 5/6. Passing tests:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_return_lowering`,
`backend_aarch64_function_traversal`, and
`backend_prepare_block_only_control_flow`. Failing representative:
`c_testsuite_aarch64_backend_src_00176_c` with
`[RUNTIME_NONZERO] ... exit=Segmentation fault`.

Proof log: `test_after.log`.

Narrow proof subset recommended for the first implementation packet remains
the same six-test subset above, with
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_function_traversal`, and
`c_testsuite_aarch64_backend_src_00176_c` as the critical signals.
