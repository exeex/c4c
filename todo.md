Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Ordinary ABI Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Ordinary ABI Consumer", residual scan after
splitting the byval/outgoing-stack producer gap to idea 624.

Current live gcc_torture backend case logs still contain `51`
`unsupported_call_abi` rows; `50` were parseable through focused
`--dump-prepared-bir` sampling. `src/20000603-1.c` is no longer in that owner
set and remains past call ABI at `unsupported_terminator_fragment`.

Focused prepared-call classification:

- `11` rows are basic same-module register/void call/result consumers without
  outgoing stack, missing frame-slot publication, fpr, or stack-copy markers:
  `src/20020529-1.c`, `src/20021219-1.c`, `src/931004-1.c`,
  `src/931004-11.c`, `src/931004-13.c`, `src/931004-3.c`,
  `src/931004-5.c`, `src/931004-7.c`, `src/931004-9.c`,
  `src/931031-1.c`, `src/pr77767.c`.
- `7` rows have explicit same-module memory-return/sret frame-slot facts:
  `src/20000917-1.c`, `src/20020206-1.c`, `src/20020810-1.c`,
  `src/20020920-1.c`, `src/20030613-1.c`, `src/990525-2.c`,
  `src/bf64-1.c`.
- `18` rows expose `missing_frame_slot_arg_publication`; keep them out of the
  next RV64 consumer packet because they need prepared publication authority,
  not target-local inference.
- `13` rows expose outgoing stack argument facts, and `7` rows expose
  aggregate `StackCopy` transport. `src/20000808-1.c` stays in this group and
  remains split to idea 624 for missing prepared outgoing-stack destination
  offsets.
- `7` sampled rows involve FPR argument/result lanes and should remain a
  separate Step 2 packet from the basic GPR/void group.
- Guard families remain out of Step 2: stack-frame rows, return
  `return_stack_to_register` rows, variadic/library/runtime rows, local/global
  producers, and downstream terminator or move-bundle owners.

## Suggested Next

Continue Step 2 with a basic same-module GPR/void call/result consumer packet.
Positive rows should come from the `11` register/void group, with
`src/931004-1.c` and `src/pr77767.c` as representative positives because they
exercise multiple scalar argument bindings without requiring outgoing stack,
missing frame-slot publication, FPR lanes, or aggregate stack-copy transport.

Negative guards for that packet should include:

- `src/20000917-1.c` as memory-return/sret breadth, not part of the first
  basic register/void packet.
- `src/20000808-1.c` as the idea-624 outgoing-stack destination-offset gap.
- `src/20001017-2.c` or `src/pr20466-1.c` for missing frame-slot argument
  publication.
- `src/20080529-1.c` or `src/ieee/unsafe-fp-assoc.c` for FPR lanes.
- Existing stack-frame, return stack-to-register, local/global producer,
  variadic, library, and runtime guard rows.

## Watchouts

- Keep variadic, library, runtime, missing prepared-authority, local/global
  producer, expectation, unsupported-marker, allowlist, timeout, and accounting
  work out of this idea.
- Do not infer prepared call/return/frame facts from final assembly or
  testcase names.
- `src/20000603-1.c` now stops at terminator lowering; do not claim it as fully
  backend-supported from this packet.
- `src/20000808-1.c` is still ordinary same-module call ABI, but its
  aggregate-address stack-copy plus outgoing stack-slot argument shape lacks
  prepared destination stack offsets. Treat that as split producer work under
  idea 624, not as immediate 613 consumer scope.
- Do not fold memory-return/sret, FPR, frame-slot publication, or outgoing
  stack transport into the next packet unless the supervisor deliberately
  changes the packet boundary.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` reports
`100% tests passed, 0 tests failed out of 346`.
