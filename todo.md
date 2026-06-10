Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Review And Close Payload

# Current Packet

## Just Finished

Completed Step 6 for `plan.md`: recorded close-ready acceptance evidence for
the BIR block-entry publication identity route.

The accepted BIR publication vocabulary/API surface is semantic identity only:
`mir::BirCurrentBlockPublicationIdentityRequest`,
`mir::BirCurrentBlockPublicationIdentity`,
`mir::find_bir_current_block_publication_identity`,
`mir::BirBlockEntryPublicationIdentityRequest`,
`mir::BirBlockEntryPublicationIdentity`, and
`mir::find_bir_block_entry_publication_identity`. These queries cover block
label, value name, before-instruction index where applicable, source producer
identity, produced BIR value/name, producer instruction, producer instruction
index, source-producer kind, successor block identity, and PHI destination
value identity.

Step 4 proof established same-block semantic identity cases for available,
unavailable, wrong-value, wrong-block, and before-index paths, plus block-entry
PHI-destination identity cases for available, missing/unpublished destination,
wrong successor, and wrong destination paths. The boundary proof documented
that prepared-only move/home/storage/register readiness positives remain
prepared oracle/fallback behavior, and that BIR PHI-entry positives do not
imply prepared emission readiness.

Step 5 switched the narrow call-boundary semantic source consumer
`prepared_call_boundary_source_value` to query
`mir::find_bir_current_block_publication_identity` first for proven same-block
semantic identity. The prepared current-block publication consumption query
remains available as the readiness oracle and fallback when BIR identity is
absent or prepared lookup/context data is needed.

Rejected fields did not enter the BIR relationship: hook kind, destination
home, storage encoding, stack-source extension policy, register-view
conversion, immediate publication payloads, emitted-storage availability,
destination register spelling, scalar emission policy, and target emission
mechanics remain outside BIR publication identity.

## Suggested Next

Supervisor can hand this Step 6 close payload to lifecycle closure review for
`ideas/open/155_bir_block_entry_publication_identity.md`.

## Watchouts

- Treat `test_before.log` as the final matched proof log from the accepted
  Step 5 code-changing scope. This Step 6 packet is todo-only and intentionally
  did not create `test_after.log`.
- Do not close the source idea as a storage/register/emission migration; the
  accepted route is limited to semantic publication identity plus one proven
  semantic consumer switch.

## Proof

Docs-only/todo-only packet per supervisor instruction. No tests were run and
no `test_after.log` was created.

Final accepted Step 5 code-changing proof is available in `test_before.log`:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R 'backend_(prepare_frame_stack_call_contract|aarch64_instruction_dispatch)' > test_after.log 2>&1
```

`test_before.log`: 2/2 targeted tests passed for
`backend_aarch64_instruction_dispatch` and
`backend_prepare_frame_stack_call_contract`.
