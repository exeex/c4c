Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broaden Within ABI Consumer Authority

# Current Packet

## Just Finished

Completed the Step 3 residual refresh after the producer gaps were split to
ideas 624, 625, and 626. No implementation files were touched.

Fresh `build/c4cll --codegen obj --target riscv64-linux-gnu -o /tmp/...`
probes classify the current Step 3 residuals as follows:

- Prepared return rows `src/20001130-2.c` and `src/20080719-1.c` still stop at
  `unsupported_move_bundle_target_shape` on `phase=before_return`,
  `destination_kind=function_return_abi`, `destination_storage=register`,
  `reason=return_stack_to_register`, with `source_home_kind=stack_slot` and
  `destination_home_kind=stack_slot`. This remains missing prepared return
  destination-home authority, not RV64 consumer breadth.
- The current backend bucket has `12` `unsupported_stack_frame` rows:
  `src/20021113-1.c`, `src/20190820-1.c`, `src/920721-2.c`,
  `src/20020314-1.c`, `src/pr36321.c`, `src/pr43220.c`, `src/alloca-1.c`,
  `src/20040223-1.c`, `src/strcpy-2.c`, `src/vla-dealloc-1.c`,
  `src/920929-1.c`, and `src/20040811-1.c`.
- Focused prepared dumps for those frame rows show the active rejecting
  functions are dynamic/fixed-frame rows with `has_dynamic_stack=yes` or
  unsupported FPR/dynamic-frame save shapes. Their callee-saved rows name
  registers such as `s1`, `s2`, `fs1`, or `fs2`, but the dynamic-frame save
  rows do not publish concrete `slot_placement` offsets/sizes. These rows stay
  under idea 626 or later FPR/frame work; deriving save slots in RV64 object
  emission would be producer-authority inference.
- Excluded producer-gap rows remain out of scope: `src/20000808-1.c` is idea
  624 outgoing-stack destination offsets, and `src/20020529-1.c` is idea 625
  stack-slot preservation source publication.

Step 2 positives were preserved in the refresh probes:

- `src/20000603-1.c` remains past `unsupported_call_abi`, now stopping at
  `unsupported_terminator_fragment`.
- `src/20021219-1.c` remains past `unsupported_call_abi`, now stopping at
  `malformed_prepared_join_transfer_carrier`.
- `src/pr77767.c` still compiles through the focused RV64 object probe.

Conclusion: after excluding ideas 624, 625, and 626, there is no remaining
complete-authority Step 3 stack-frame or prepared-return consumer family
suitable for RV64 implementation in idea 613.

## Suggested Next

Advance idea 613 out of Step 3. The next coherent packet is a Step 4 residual
refresh for adjacent ABI consumer authority, or Step 5 close-readiness
classification if the supervisor prefers to split remaining non-consumer
owners before more ABI work. Do not select a Step 3 implementation packet until
prepared return destination-home authority or dynamic-frame save-slot placement
facts are published upstream.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
- Keep dynamic/fixed frame callee-saved save-slot placement production under
  idea 626; do not infer save slots in RV64 object emission.
- Do not infer return destination home authority for `src/20001130-2.c` or
  `src/20080719-1.c`; their before-return rows remain producer-authority gaps.
- Do not expand the next frame packet into FPR save/restore, generic
  move-bundle authority production, local/global producer repair,
  runtime/library/variadic policy, or named-case-only frame handling.
- Preserve Step 2 positive guards: `src/20000603-1.c`, `src/20021219-1.c`,
  and `src/pr77767.c`.
- `src/20020314-1.c` remains a compatibility watchout with FPR-heavy
  frame/call details; do not fold FPR dynamic-frame handling into the first GPR
  dynamic-frame packet.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` reports
`100% tests passed, 0 tests failed out of 346` and is the preserved proof log.
