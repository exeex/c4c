Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Positive And Negative Coverage

# Current Packet

## Just Finished

Step 3 focused backend coverage completed for
`arg.source_selection=local_frame_address_materialization` without
implementation changes.

Added `tests/backend/case/riscv64_call_arg_local_frame_address_materialization.c`
and wired two RV64 backend CTests in `tests/backend/CMakeLists.txt`:

- `backend_dump_riscv64_call_arg_local_frame_address_materialization` observes
  `bir.call i32 read_local_address(ptr %lv.value)` with
  `arg.source_selection=local_frame_address_materialization`,
  `dest_reg=a0`, `selection_source_value=%lv.value`, and the matching
  `address_materialization block=entry` prepared fact.
- `backend_codegen_route_riscv64_call_arg_local_frame_address_materialization`
  observes text lowering that sets up the ABI GPR argument with
  `addi a0, sp, ...` and forbids the stale emitted `mv a0, s1` copy shape.

The prepared dump still contains the legacy move-bundle diagnostic line
`reason=call_arg_register_to_register` before the detailed prepared call
contract. The focused negative assertion is therefore on the emitted RV64
argument setup rather than that diagnostic artifact.

## Suggested Next

Proceed to Step 4 only if the supervisor wants an implementation or audit packet
for a remaining focused failure. The new text-route focused test is already
green with no implementation changes, so the next likely packet is a
representative Step 5 probe or split decision for the known
`src/20000722-1.c` object-route `unsupported_local_memory_access` blocker.

## Watchouts

- Do not reopen string-constant local-memory admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostics.
- Treat `reason=call_arg_register_to_register` in the focused prepared dump as
  a pre-contract move-bundle diagnostic artifact, not as the assertion target
  for this Step 3 route test.
- The fresh representative object route currently stops at
  `unsupported_local_memory_access` outside the call-argument consumer. Do not
  broaden Step 3/4 into local-memory admission just to get
  `src/20000722-1.c` to object disassembly.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- Preserve ordinary register-to-register GPR call-argument lowering when there
  is no explicit `LocalFrameAddressMaterialization` source selection.

## Proof

Ran the delegated proof exactly:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|codegen_route)_riscv64_call_arg_local_frame_address_materialization$"; } 2>&1 | tee test_after.log'`

Result: passed. Both
`backend_dump_riscv64_call_arg_local_frame_address_materialization` and
`backend_codegen_route_riscv64_call_arg_local_frame_address_materialization`
passed. Proof log: `test_after.log`.
