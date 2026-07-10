Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected AArch64 Rule

# Current Packet

## Just Finished

Step 3 repaired the selected AArch64 entry-formal publication boundary in
`src/backend/mir/aarch64/codegen/prologue.cpp`.

The code packet keeps row 322 untouched and changes only
`lower_entry_formal_publications`: AArch64 non-byval f128 stack-source
publications with supported `None`, `Register`, or `StackSlot` homes may now
reach `entry_formal_stack_source_publication_lines` when the generic prepared
formal publication plan lacks `incoming_stack_offset_bytes`. The AArch64 helper
still computes the incoming stack offset target-locally and still returns no
publication when BIR context, frame size, stack-source ownership, q scratch
selection, unsupported home kinds, unsupported views, or byval shapes are not
valid.

Before this packet, the delegated proof failed row 284 at `expected
stack-passed f128 HFA formals to seed local carriers before return`. After this
packet, row 284 passes, including the nearby mixed GPR/HFA stack-passed f128
guard. Row 322 remains the first failing row in the delegated subset and still
belongs to the later AArch64 call-publication packet.

## Suggested Next

Delegate the next Step 3 packet to the AArch64 call-publication path for row
322, likely around `src/backend/mir/aarch64/codegen/calls.cpp` and the
`materialize_missing_frame_slot_call_arguments` /
`find_prepared_frame_slot_call_argument_move` boundary identified in Step 2.
Keep the packet focused on the missing `arg index=8` frame-slot publication and
do not re-open entry-formal publication unless new evidence points back here.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 still misses
  `arg index=8 value_bank=vreg source_encoding=frame_slot source_value_id=2728
  source_slot=#3138 source_stack_offset=8224 source_bank=fpr dest_bank=none
  dest_stack_offset=0`; the observed dump still reports `source_value_id=2728`
  at `arg index=15 source_slot=#3145 source_stack_offset=8336`.
- Do not repair row 322 through CLI text formatting, expectation edits,
  unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, baseline accounting, or named-case shortcuts.

## Proof

Ran exact delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1
```

Result: build succeeded, row 284 `backend_aarch64_instruction_dispatch` passed,
and row 322
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
failed at the known missing prepared call-publication snippet. Overall delegated
subset result: 1 of 2 tests passed, with `test_after.log` preserved as the proof
log.
