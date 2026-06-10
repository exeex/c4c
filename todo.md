Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch One Semantic Identity Consumer If Proven

# Current Packet

## Just Finished

Completed Step 5 for `plan.md`: switched one same-block call-boundary semantic
source read in `prepared_call_boundary_source_value` to ask
`mir::find_bir_current_block_publication_identity` first for the same BIR
block, prepared value spelling, and before-instruction index.

The helper now returns the BIR-produced value when that semantic identity is
available. The prepared current-block publication consumption query remains as
fallback/readiness-oracle behavior for cases where the BIR identity is absent
or the prepared lookup/context data is needed.

No test expectation rewrites were needed; the existing call-contract and
AArch64 dispatch subset already covers the relevant boundary.

## Suggested Next

Step 5 follow-up candidate: review whether any remaining call-boundary semantic
source reads still depend directly on prepared current-block publication
consumption. Keep any further switch narrow and preserve prepared ownership of
publication readiness, register/storage policy, and emission facts.

## Watchouts

- This switch intentionally treats BIR current-block publication as semantic
  source identity only; prepared current-block publication consumption still
  owns readiness/fallback behavior when BIR identity is missing.
- The helper now no longer requires `prepared_lookups` before trying BIR
  identity, but it still requires prepared names to resolve the value id to a
  BIR spelling.
- Do not use this slice as precedent for moving register parsing, destination
  storage decisions, scalar emission policy, or prepared hook/home facts into
  BIR identity queries.
- Escalate validation if a future packet touches dispatch publication, scalar
  publication planning, value homes, or emitted instruction behavior beyond
  this local source-value helper.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R 'backend_(prepare_frame_stack_call_contract|aarch64_instruction_dispatch)' > test_after.log 2>&1
```

`test_after.log`: 2/2 targeted tests passed.
