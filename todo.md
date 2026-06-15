Status: Active
Source Idea Path: ideas/open/276_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Acceptance Proof

# Current Packet

## Just Finished

Step 4 - Run Acceptance Proof is complete. The supervisor-selected acceptance
proof passed for the stale-publication fail-closed behavior and the compatible
dynamic stack-source `LoadLocal` positive path.

## Suggested Next

Return to the supervisor for lifecycle decision and commit handling.

## Watchouts

- Keep cross-publication, byte-offset coverage, x86 parity, edge-publication
  families, metadata, liveness, aggregate retirement, and draft 155 work out
  of this runbook.
- The stale proof is intentionally fixture-backed and uses no synthetic
  post-construction mutation, named-case shortcut, or expectation downgrade.
- Route 3 / Route 5 authority must stay on the current offset `12` row; moving
  it to the stale public offset `16` only proves a weaker mismatch case.
- No fallback, status, debug, prepared-printer, wrapper, exact-output, helper,
  oracle, route-debug, or baseline contract was weakened.

## Proof

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` records the successful build step and targeted
`backend_riscv_prepared_edge_publication` CTest run, with 1/1 tests passing.
