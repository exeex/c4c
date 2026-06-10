# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Run the acceptance proof

## Just Finished

Step 6 recorded the final acceptance proof for idea 156's BIR CFG edge
publication identity route and left the packet close-ready for supervisor
lifecycle handling.

Completed work item:

- BIR CFG edge publication now exposes a query surface for edge-source
  identity instead of forcing consumers to inspect prepared ownership internals.
- Prepared-oracle equivalence proof covers normal edge, memory-source, and
  no-source paths, preserving behavior while changing the source of truth for
  semantic identity reads.
- Current-block join-source BIR query and proof are present for the join-source
  path.
- The single switched current-block join routing consumer is
  `build_current_block_join_prepared_query_routing`; it reads BIR identity when
  available and retains prepared facts as the availability guard and fallback.
- Prepared ownership remains authoritative for move execution, scheduling,
  storage, register, and home policy.

## Suggested Next

Close or otherwise resolve the active lifecycle state for
`ideas/open/156_bir_cfg_edge_publication_identity.md`; the executor acceptance
packet is complete.

## Watchouts

- This packet is evidence-only and intentionally did not modify source,
  tests, plans, ideas, review artifacts, or logs.
- Do not widen the BIR identity query into authority for register names, homes,
  stack-source routing, immediate move details, move bundles, prepared move
  records, or scheduling/storage policy.
- The final consumer switch remains intentionally narrow:
  `build_current_block_join_prepared_query_routing`.

## Proof

Did not run tests for this evidence-only packet, per supervisor delegation.

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|aarch64_instruction_dispatch)' > test_after.log 2>&1`

Supervisor-reported final acceptance proof logs:

- `test_before.log`
- `test_after.log`

Covered subsets:

- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Result: 2/2 passed. Supervisor verified
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passes with monotonic guard PASS.
