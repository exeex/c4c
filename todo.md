Status: Active
Source Idea Path: ideas/open/87_out_of_ssa_contract_and_parallel_copy_authority_for_prealloc.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Surface Audit
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 1 audit now closes the remaining slot-backed path: the prepared dump already made loop-carry
authority explicit as `carrier=edge_store_slot` plus `storage=` and per-edge `edge_transfer`
records, while `PreparedParallelCopyBundle` already made cycle-break publication explicit through
bundle `resolution=` and ordered `step[...]` rows. This packet tightens printer-focused coverage so
`backend_prepared_printer_test` now locks in a slot-backed loop-carry dump and the cycle-breaking
parallel-copy plan without needing source dives. Proof is green with
`cmake --build --preset default` and `ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Suggested Next

Step 2: use the completed Step 1 audit to decide whether any target-independent join-transfer or
parallel-copy fields still need strengthening, or whether downstream work can proceed with the
current published authority and focus on consumer-side reliance instead of dump visibility.

## Watchouts

- Keep phi elimination and parallel-copy authority in `out_of_ssa`; do not push semantics back into `legalize`.
- The slot-backed loop path is not implicit fallback state: it is already published as a per-edge
  edge-store-slot contract in both `join_transfer` and `parallel_copy` rows, so future work should
  extend semantics only if a real downstream authority gap remains.
- Treat grouped-register authority and prepared frame/stack/call work as separate ideas, not scope for this runbook.

## Proof

Passed: `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Proof log: `test_after.log`.
