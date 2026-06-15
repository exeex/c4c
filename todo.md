Status: Active
Source Idea Path: ideas/open/280_phase_f5_edge_publications_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Prepared-Only Fail-Closed Proof

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding the focused RISC-V prepared-only
`edge_publications` proof in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`. The proof
keeps the public `left -> join` prepared publication visible for destination
value id `2` / `%dst`, source value id `1` / `%src`, `LoadLocal` source memory
`%base + 12`, and destination register `a1`, then passes the real
`riscv::consume_edge_publication_move_intent(..., &route5_edge)` consumer a
same-edge `bir::Route5CfgEdgePublicationRecord` with status `NoSource`.

The consumer now proves fail-closed behavior for that prepared-only dynamic
`LoadLocal` row: the intent preserves the prepared publication pointer, records
Route 5 status `NoSource`, reports Route 5 source agreement false and Route 3
source-memory agreement false, emits no instruction, and records no source
memory byte offset. The compatible agreeing Route 5 `MemorySource` positive path
still emits exact `lw a1, 12(s2)`.

## Suggested Next

Execute Step 3: record the preserved compatible positive path for the dynamic
`LoadLocal` row, including the agreeing Route 5 `MemorySource` authority and
exact `lw a1, 12(s2)` output already asserted by Step 2.

## Watchouts

- The prepared-only proof uses the real RISC-V consumer path and a Route 5
  `NoSource` record, not helper-only lookup rejection or synthetic public
  lookup mutation.
- Use the dynamic `LoadLocal` row for this source idea; scalar register rows
  still preserve prepared fallback on non-agreeing Route 5 facts.
- Keep Step 3 scoped to documenting/preserving the compatible positive path; do
  not widen into additional Route 5 status rows.
- Do not weaken fallback, status/debug, route-debug, prepared-printer,
  helper/oracle, wrapper, exact-output, or baseline contracts.
- Keep this runbook to one bounded prepared-only `edge_publications` row.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

Result: passed. Targeted CTest passed 1/1, and `test_after.log` is preserved as
the proof log.
