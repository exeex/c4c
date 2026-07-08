Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broaden Within ABI Consumer Authority

# Current Packet

## Just Finished

Completed the Step 4 residual refresh for adjacent ABI consumer authority after
Step 2 and Step 3 exhaustion. No implementation files were touched.

Fresh focused probes ran
`build/c4cll --codegen obj --target riscv64-linux-gnu -o /tmp/...` over the
current ABI-adjacent case set selected from the RV64 torture backend logs. The
current focused classification is:

- `47` rows still stop at `unsupported_call_abi`.
- `12` rows still stop at `unsupported_stack_frame`: `src/20020314-1.c`,
  `src/20021113-1.c`, `src/20040223-1.c`, `src/20040811-1.c`,
  `src/20190820-1.c`, `src/920721-2.c`, `src/920929-1.c`,
  `src/alloca-1.c`, `src/pr36321.c`, `src/pr43220.c`, `src/strcpy-2.c`, and
  `src/vla-dealloc-1.c`.
- `36` probed move-bundle target rows are not Step 4 ABI consumer work. The
  two return-ABI rows, `src/20001130-2.c` and `src/20080719-1.c`, still stop at
  `reason=return_stack_to_register` with `destination_kind=function_return_abi`,
  `destination_storage=register`, `source_home_kind=stack_slot`, and
  `destination_home_kind=stack_slot`; these remain missing prepared return
  destination-home authority.
- `src/20021219-1.c` remains past `unsupported_call_abi` and now stops at
  `malformed_prepared_join_transfer_carrier`.
- `src/pr77767.c` still compiles through the focused RV64 object probe.

Prepared callsite dumps for the `unsupported_call_abi` rows show real Step 4
breadth with complete published consumer facts:

- Scalar GPR calls with prepared stack-slot argument sources and argument
  registers, for example `src/20001017-2.c` publishes
  `arg3 bank=gpr from=frame_slot:stack+24 to=a3`, `src/20001101.c` publishes
  `arg1 bank=gpr from=frame_slot:stack+40 to=a1` plus
  `result bank=gpr from=register:a0 to=register:t0`, and
  `src/20030715-1.c` publishes `arg1 bank=gpr from=frame_slot:stack+48 to=a1`
  plus a concrete stack-slot result destination.
- Scalar GPR result transport has breadth: the focused dump found `28`
  `result bank=gpr from=register:a0 to=register:t0` rows and multiple concrete
  `register:a0` to `stack_slot:stack+...` result destinations.
- Aggregate-address stack/frame call rows remain visible, including
  `931004-*`/`931031-1.c`, but those are not the first Step 4 packet because
  they overlap outgoing aggregate transport and producer gaps already split to
  idea 624.

Conclusion: Step 4 has adjacent ordinary ABI consumer breadth. The next
implementation packet should target complete-fact scalar GPR call argument and
result transport, not producer-authority or policy rows.

## Suggested Next

Implement the Step 4 scalar GPR same-module call/result consumer packet in RV64
object emission. The packet should consume only explicit prepared callsite
facts for:

- GPR argument transport from `register:*`, `frame_slot:stack+...`, and simple
  immediate sources to `a0`-`a7`.
- GPR result transport from `register:a0` to a prepared destination register or
  prepared destination stack slot.
- Concrete preservation rows already published by prepared callsite summaries.

Representative positives: `src/20001017-2.c`, `src/20001101.c`,
`src/20010118-1.c`, `src/20030715-1.c`, and `src/20040625-1.c`.
Representative guards: `src/20000808-1.c` for idea 624 outgoing-stack
destination offsets, `src/20020529-1.c` for idea 625 stack-slot preservation
source publication, `src/20040811-1.c`/`src/pr43220.c` for idea 626 frame
save-slot placement, `src/20001130-2.c`/`src/20080719-1.c` for prepared return
destination-home authority, and `src/20021219-1.c` for downstream
join-transfer carrier authority.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
- Keep dynamic/fixed frame callee-saved save-slot placement production under
  idea 626; do not infer save slots in RV64 object emission.
- Do not infer return destination home authority for `src/20001130-2.c` or
  `src/20080719-1.c`; their before-return rows remain producer-authority gaps.
- Do not fold aggregate-address outgoing stack transport, FPR call/result
  policy, generic move-bundle authority production, local/global producer
  repair, runtime/library/variadic policy, or named-case-only call handling
  into the scalar GPR Step 4 packet.
- Preserve Step 2 positive/downstream guards: `src/20000603-1.c`,
  `src/20021219-1.c`, and `src/pr77767.c`.
- `src/20020314-1.c` remains a compatibility watchout with FPR-heavy
  frame/call details; do not fold FPR dynamic-frame handling into the first GPR
  dynamic-frame packet.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` reports
`100% tests passed, 0 tests failed out of 346` and is the preserved proof log.
