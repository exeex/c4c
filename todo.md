Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 3 is complete at the expectation-contract boundary. The bounded
current-call aggregate-carrier classification packet for row 322
`arg index=12` moved prepared call-plan construction to a same-call aggregate
lane-group lookup that requires matching `aggregate_source_value_name`,
matching lane count, and a terminal lane index inside the current call before
it reuses a lane-run carrier identity.

Before this packet, row 322 `arg index=12` used the scalar lane source
`source_value_id=2725` / `%t58.0` while retaining `source_slot=#3142` and
`source_stack_offset=8288`; the expected snippet wanted `source_value_id=2732`.
After this packet, the same row resolves to the current-call `%t58` aggregate
group terminal source `source_value_id=2728` / `%t58.48` while preserving
`source_slot=#3142`, `source_stack_offset=8288`, and `dest_stack_offset=64`.
The expectation-contract review in
`review/row322_expectation_contract_review.md` confirmed that `2732` remains
outside current-call arg-12 authority. It is the later
`%t61.global.aggregate.load.16` value, so using it would require the rejected
later-call/later-store lookahead route. Do not continue Step 3 implementation
by chasing `2732` under idea 665.

## Suggested Next

Proceed with Step 4 regression and lifecycle readiness. The supervisor should
choose the focused AArch64 proof plus any broader backend subset needed for
acceptance, then decide whether idea 665 can close or must remain active with a
new narrow packet.

Expectation path status: `%t58.48` / `2728` is the valid current-call owner for
row 322 `arg index=12`; `2732` is not a valid current-call source identity on
the current evidence. If row 322 still needs an expectation adjustment, handle
that outside Step 3 implementation for idea 665. Create or switch to a separate
idea only if the supervisor decides the expectation contract itself is a
durable initiative; do not expand idea 665 to revive later-call/later-store
lookahead.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 is no longer blocked on the original `arg index=8` prepared source
  identity. The remaining first failure is now `arg index=12`, where the
  current-call aggregate group owner is `%t58.48` / `2728` and the expected
  owner is still the later value id `2732`.
- Lifecycle decision: the row 322 expectation-contract authority is resolved
  for Step 3. `2732` is not a valid current-call owner; no further Step 3 code
  packet should chase it unless new evidence proves a current-call fact that
  directly ties call inst 460 arg 12 to `2732` without later lookahead.
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

Lifecycle review:

```text
review/row322_expectation_contract_review.md
```

Result: `2732` is not a valid current-call source identity for row 322
`arg index=12`; the current-call authority is `%t58.48` / `2728`. Another
Step 3 implementation packet is not justified unless new evidence proves a
current-call fact that directly ties call inst 460 arg 12 to `2732` without
later-call/later-store lookahead.
