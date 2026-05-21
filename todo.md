Status: Active
Source Idea Path: ideas/open/352_aarch64_block_label_emission_ordering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add focused multi-block emission coverage and repair AArch64 label/order emission

# Current Packet

## Just Finished

Plan Step 2 completed: added focused AArch64 multi-block coverage for a
conditional branch whose later successor appears after a return block in
prepared order, and repaired conditional branch lowering so same-block
select-chain compare operands can be materialized before branch emission.

Implementation notes:

- `dispatch_prepared_block` can now use the same-block compare producer fallback
  even when the prepared condition value is register-homed, not only when it is
  stack-homed.
- `emit_value_publication_to_register` now handles same-block `SelectInst`
  producers through the existing select-chain materialization path, allowing
  fused compare branches with selected operands to emit real branch instructions.
- New coverage in `backend_aarch64_branch_control_lowering` builds and prints a
  four-block function ordered as conditional branch, return, later body, join;
  it asserts the conditional branch records two successors and the later body is
  labeled after the return.

Updated `00176` evidence:

- Generated `partition` now emits the missing `block_2` branch:
  `cmp w9, w13`, `b.lt .LBB90_6`, `b .LBB90_7`.
- The later swap body now has label `.LBB90_6` after the `block_3` return
  epilogue instead of appearing as unlabeled post-`ret` code.
- The c-testsuite case advances past the previous unlabeled-post-epilogue first
  bad fact, but still fails at runtime with `Segmentation fault`; this is the
  next concrete bad fact.

## Suggested Next

Next packet: localize the remaining `00176` runtime segfault after repaired
`partition` branch/label emission. Start from prepared/generated `partition`
with `.LBB90_6` present and compare the selected array value/index stack slots
and call argument values around `block_2`/`block_4`.

## Watchouts

- Do not rework call-argument publication unless fresh evidence shows another
  stale caller-clobbered post-call argument use.
- Do not special-case `00176`, `partition`, one block id, one label suffix, or
  one emitted return sequence.
- Keep the branch/label repair intact: the remaining `00176` failure is after
  `block_2` now emits a terminator and successor labels, not the original
  unlabeled post-return body.
- Return/epilogue insertion still does not appear to be the first bad owner:
  `block_3` is a real prepared return block, and the epilogue-before-`ret`
  placement is expected for that block.
- Keep `00181` out of this route unless its current stack-preserved
  symbol/local publication crash is fixed or bypassed by its owner and a fresh
  block-label first bad fact appears.

## Proof

Delegated proof run for this implementation packet:

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

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
