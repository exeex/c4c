Status: Active
Source Idea Path: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Cross-Publication Negative Proof

# Current Packet

## Just Finished

Completed Step 2 of `plan.md` by adding the cross-publication/block-label
mismatch proof to
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` in
`check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()`.

The new fixture publishes the selected normal `PreparedMemoryAccess` row for
`%src` / value id `1` at predecessor block `left`, instruction index `0`,
pointer base `%base`, offset `12`, then adds a second normal public row with
the same address facts and offset `12` but `block_label = join`. The real
`consume_edge_publication_move_intent(..., &route5_memory_edge)` RISC-V
consumer path rejects the duplicated public source rows as
`EdgePublicationMoveIntentStatus::UnsupportedSourceHome`, emits no
instruction, records no rendered source-memory offset, and still reports
agreeing Route 5 / Route 3 memory-source authority at offset `12`.

The compatible dynamic `LoadLocal` path remains unchanged and still emits
exactly `lw a1, 12(s2)` with `EdgePublicationMoveIntentStatus::Available`.

## Suggested Next

Execute Step 3 by proving the compatible path remains a positive control for
the same fixture: current Route 3 / Route 5 authority stays aligned at
`left:0`, offset `12`, and the RISC-V dynamic stack-source output remains
`lw a1, 12(s2)`.

## Watchouts

- The new negative proof is intentionally distinct from byte-offset drift and
  stale-offset publication: both public rows use offset `12`, and the mismatch
  is the public publication block label (`left` versus `join`).
- The fail-closed evidence depends on normal lookup construction preserving two
  public rows by source value id, not synthetic map mutation.
- Do not weaken fallback, status/debug, route-debug, prepared-printer, wrapper,
  helper/oracle, exact-output, or baseline contracts.
- Keep byte-offset, stale-publication, x86 parity, edge-publication, metadata,
  liveness, aggregate retirement, and draft 155 work out of this plan.

## Proof

Passed.

Proof command run exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

`test_after.log` records a successful build and
`backend_riscv_prepared_edge_publication` passing 1/1.
