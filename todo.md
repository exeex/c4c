Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Ordinary ABI Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Ordinary ABI Consumer", residual refresh
after the producer gaps for `src/20000808-1.c` and `src/20020529-1.c` were
split to ideas 624 and 625.

Current focused `build/c4cll --codegen obj --target riscv64-linux-gnu`
probes confirm the accepted Step 2 positives remain moved:

- `src/20000603-1.c` now stops at downstream
  `unsupported_terminator_fragment`, not `unsupported_call_abi`.
- `src/20021219-1.c` now stops at downstream
  `malformed_prepared_join_transfer_carrier`, not `unsupported_call_abi`.
- `src/pr77767.c` still compiles through RV64 object codegen.

The refreshed ordinary same-module `unsupported_call_abi` candidates sampled
after those moves do not expose a remaining complete-authority Step 2 consumer
packet. The live residual classes are:

- Missing frame-slot call-argument publication:
  `src/20001101.c`, `src/20001017-2.c`, `src/20010118-1.c`,
  `src/20020404-1.c`, `src/20020406-1.c`, `src/20030715-1.c`, and
  `src/20041218-1.c` have prepared call rows that still publish
  `missing_frame_slot_arg_publication=yes` or equivalent incomplete
  frame-slot source authority before RV64 can consume them.
- Missing stack-preserve source register publication:
  `src/20020529-1.c` remains idea 625, and the same prepared-authority shape
  is also visible in rows such as `src/20010129-1.c`,
  `src/20020404-1.c`, `src/20020406-1.c`, and `src/20041218-1.c` through
  `preservation_source=register:value#...` with no concrete source register.
- Outgoing stack / aggregate transport:
  `src/20000808-1.c` remains idea 624; the same family includes
  `931004-*` and `src/931031-1.c`, where prepared facts are still not a
  simple complete GPR register call/result consumer packet.
- Memory-return / sret and FPR lanes:
  representative residuals such as `src/20000917-1.c` and
  `src/20020810-1.c` require stack/memory return or lane-specific ABI
  handling outside this first ordinary GPR Step 2 packet.

No implementation files, plan files, idea files, expectations, unsupported
markers, allowlists, or timeout/accounting files were touched.

## Suggested Next

Hand off idea 613 to Step 3, "Add Supported Frame Or Return Handling". The next
coherent packet should refresh supported stack-frame and prepared return
residuals, then select a frame or return consumer row only when explicit
upstream facts are already complete. Treat the remaining Step 2
`unsupported_call_abi` rows above as producer gaps or later distinct ABI
families, not as ordinary complete-authority GPR call/result work.

## Watchouts

- Keep `src/20000808-1.c` under idea 624 and `src/20020529-1.c` under idea
  625; do not reclassify missing prepared authority as RV64 consumer progress.
- Do not infer frame-slot argument sources, stack-preserve source registers,
  outgoing stack offsets, FPR lanes, memory-return slots, or aggregate payload
  transport from ABI index, final assembly shape, or testcase names.
- `931004-*` / `src/931031-1.c` have breadth, but the breadth is aggregate
  stack transport, not the already-complete ordinary GPR call/result consumer
  family selected for Step 2.
- Existing positive rows (`src/20000603-1.c`, `src/20021219-1.c`,
  `src/pr77767.c`) should remain guards for any Step 3 frame/return slice.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` is the preserved
proof log.
