Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected AArch64 Rule

# Current Packet

## Just Finished

Step 3 completed the bounded prepared/prealloc publication packet for AArch64
variadic HFA aggregate-carrier lane metadata at the prepared call-plan
construction point. `CallArgumentPublicationSourceRouting`, the route6 source
records, and `PreparedCallArgumentPlan` construction now carry and consume
`aggregate_source_value_name` / lane index / lane count metadata instead of
leaving the prepared source identity to printer or regalloc interpretation.

Before this packet, row 322 failed on `arg index=8` with prepared
`source_value_id=2721` / `%t56.0` while retaining lane placement
`source_slot=#3138` and `source_stack_offset=8224`; the expected owner was
`source_value_id=2728` / `%t58.48`. After this packet, that row-322 snippet is
present: `arg index=8` now has `source_value_id=2728` while preserving
`source_slot=#3138`, `source_stack_offset=8224`, `dest_stack_offset=0`, and
the explicit lane `arg.source_selection` frame-slot facts.

## Suggested Next

Continue Step 3 with a packet for the new first row-322 prepared snippet
failure: `arg index=12` still reports `source_value_id=2725` / `%t58.0` with
`source_slot=#3142` and `source_stack_offset=8288`, while the expected snippet
wants `source_value_id=2732` with the same lane placement facts. Treat this as
the next prepared source-owner selection rule for later stack aggregate-carrier
lanes, not as a printer, expectation, or regalloc-index repair.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 is no longer blocked on the original `arg index=8` prepared source
  identity. The remaining first failure is now `arg index=12`, where the
  current prepared owner remains the lane value `%t58.0` while the expected
  owner has advanced to value id `2732`.
- Lifecycle decision: keep row 322 in Step 3 as an AArch64 BIR publication
  representation repair; do not move to Step 4 until the remaining prepared
  aggregate-carrier owner mismatch is resolved or proves a different owner.
- Do not repair row 322 by making `append_call_arg_move_resolution` reinterpret
  `arg_index` after BIR has already assigned the wrong value to that index.
- Do not repair row 322 through CLI text formatting, expectation edits,
  unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, baseline accounting, or named-case shortcuts.

## Proof

Ran the delegated proof and preserved `test_after.log`:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1
```

Result: build completed, row 284 passed, row 322 still failed after advancing
past the previous `arg index=8` mismatch. Current first failure in
`test_after.log` is `[BACKEND_DUMP_SNIPPET_MISSING]` for `arg index=12`:
expected `source_value_id=2732 source_slot=#3142 source_stack_offset=8288`;
actual prepared output has `source_value_id=2725` / `%t58.0` with the same slot
and stack offset.
