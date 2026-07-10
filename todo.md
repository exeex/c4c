Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected AArch64 Rule

# Current Packet

## Just Finished

Step 3 completed the bounded current-call aggregate-carrier classification
packet for row 322 `arg index=12`. Prepared call-plan construction now has a
same-call aggregate lane-group lookup that requires matching
`aggregate_source_value_name`, matching lane count, and a terminal lane index
inside the current call before it reuses a lane-run carrier identity.

Before this packet, row 322 `arg index=12` used the scalar lane source
`source_value_id=2725` / `%t58.0` while retaining `source_slot=#3142` and
`source_stack_offset=8288`; the expected snippet wanted `source_value_id=2732`.
After this packet, the same row resolves to the current-call `%t58` aggregate
group terminal source `source_value_id=2728` / `%t58.48` while preserving
`source_slot=#3142`, `source_stack_offset=8288`, and `dest_stack_offset=64`.
The requested `2732` remains outside current-call arg-12 authority in this
proof; it is the later `%t61.global.aggregate.load.16` value, so using it would
require the rejected later-call/later-store lookahead route.

## Suggested Next

Stop Step 3 implementation and request an explicit expectation-contract review
for row 322 before any more code packets. The current-call owner for
`arg index=12` is now classified as `%t58.48` / `2728`; the expected
`source_value_id=2732` is the later `%t61.global.aggregate.load.16` value and
cannot be used without reviving the rejected later-call/later-store lookahead.

Lifecycle decision: do not continue Step 3 by chasing `2732`. If review or
explicit user direction accepts `2728` as the correct current-call contract,
the supervisor can move to Step 4 regression/lifecycle readiness after the
expectation path is handled outside this packet. If review proves a
current-call fact that should name `2732`, delegate a narrow Step 3 packet
around that current-call source authority only. If neither condition holds,
split or reroute row 322 instead of expanding idea 665.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 is no longer blocked on the original `arg index=8` prepared source
  identity. The remaining first failure is now `arg index=12`, where the
  current-call aggregate group owner is `%t58.48` / `2728` and the expected
  owner is still the later value id `2732`.
- Lifecycle decision: row 322 is blocked on expectation-contract authority, not
  on another known AArch64 implementation repair. Do not move to Step 4 or
  delegate more Step 3 code until review resolves whether expected `2732` is a
  valid current-call contract or should be revised outside this packet.
- Reviewer reset: `review/row322_later_lane_review.md` rejected the
  uncommitted later-call/later-store lookahead route as testcase-overfit and
  temporal route drift. Do not record that route as progress or revive it as
  the authority for current-call prepared source identity.
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

Result: build completed, row 284 passed, row 322 still failed closed at the
current-call aggregate owner for `arg index=12`. Current first failure in
`test_after.log` is `[BACKEND_DUMP_SNIPPET_MISSING]` for `arg index=12`:
expected `source_value_id=2732 source_slot=#3142 source_stack_offset=8288`;
actual prepared output now has `source_value_id=2728` / `%t58.48` with the
same slot and stack offset.
