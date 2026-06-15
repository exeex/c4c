Status: Active
Source Idea Path: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run The Targeted Acceptance Proof

# Current Packet

## Just Finished

Completed Step 4 of `plan.md` by running the supervisor-selected targeted
acceptance proof for the cross-publication mismatch fail-closed behavior and
the compatible positive path.

The proof passed for `backend_riscv_prepared_edge_publication`: the mismatch
case remains fail-closed through the real RISC-V consumer path, and the
compatible dynamic stack-source `LoadLocal` positive path remains accepted with
the exact `lw a1, 12(s2)` output.

## Suggested Next

Return to the supervisor for lifecycle review or close-gate handling.

## Watchouts

- The new negative proof is intentionally distinct from byte-offset drift and
  stale-offset publication: both public rows use offset `12`, and the mismatch
  is the public publication block label (`left` versus `join`).
- The fail-closed evidence depends on normal lookup construction preserving two
  public rows by source value id, not synthetic map mutation.
- No fallback, status/debug, route-debug, prepared-printer, wrapper,
  helper/oracle, exact-output, or baseline contract was weakened.
- Keep byte-offset, stale-publication, x86 parity, edge-publication, metadata,
  liveness, aggregate retirement, and draft 155 work out of this plan.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

Result: passed. The build was up to date and targeted CTest passed 1/1.
Proof log path: `test_after.log`.
