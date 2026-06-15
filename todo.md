Status: Active
Source Idea Path: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run Acceptance Proof

# Current Packet

## Just Finished

Completed Step 4 of `plan.md` by running the acceptance proof for the
byte-offset coverage gate.

The delegated build plus targeted
`^backend_riscv_prepared_edge_publication$` CTest subset passed 1/1 and
preserved the fail-closed byte-offset mismatch proof plus the compatible exact
`lw a1, 12(s2)` output path.

## Suggested Next

Supervisor should decide the next lifecycle action for the active plan.

## Watchouts

Do not claim exhaustive byte-offset coverage from this one row. This acceptance
proof did not weaken fallback, status/debug, route-debug, helper/oracle,
prepared-printer, wrapper, exact-output, or baseline contracts.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'; } > test_after.log 2>&1
```

Result: passed. Build was up to date, and targeted CTest passed 1/1.

Proof log: `test_after.log`.
