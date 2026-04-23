Status: Active
Source Idea Path: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Call Boundary Authority Completion
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed another Step 3 "Call Boundary Authority Completion" packet for idea
88 by publishing literal call-argument authority in `PreparedCallPlan`,
printing it in prepared dumps, and tightening backend/prealloc tests so
downstream consumers no longer need to recover immediate call operands from raw
BIR when the prepared call plan already knows them.

Current packet result:
- `PreparedCallArgumentPlan` now publishes a richer `source_encoding` plus
  literal, symbol, and computed-address source detail instead of collapsing
  non-register/non-stack argument sources to `none`.
- `populate_call_plans` now snapshots immediate arguments directly from BIR and
  preserves richer prepared-home detail for rematerializable and pointer-base
  argument sources at call-plan construction time.
- Prepared dumps now expose immediate call-argument authority in both the
  summary callsite view and the `prepared-call-plans` detail section.
- Backend/prealloc tests now prove immediate argument publication for direct,
  indirect, and memory-return call contracts.

## Suggested Next

Continue Step 3 by proving the newly published non-register argument source
shapes beyond immediates, with the best next packet likely focused on direct
symbol-address or computed-address call operands if any target consumer still
needs to fall back to raw BIR names or pointer arithmetic.

## Watchouts

- Keep scalar frame/stack/call authority separate from grouped-register work in
  idea 89.
- Keep call-boundary authority at the prepared contract boundary; do not turn
  this packet into target-specific call instruction recovery.
- If later aggregate returns use non-frame-slot or multi-lane storage, publish
  that shape in `PreparedCallPlan` instead of sending backends back to raw BIR
  or storage-plan side channels.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_dump_prepared_bir_exposes_contract_sections)$'`.
Result: pass.
Proof log: `test_after.log`.
