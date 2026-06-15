Status: Active
Source Idea Path: ideas/open/278_phase_f5_memory_accesses_byte_offset_coverage_gate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve Compatible Exact Output

# Current Packet

## Just Finished

Completed Step 3 of `plan.md` as a status-only preservation record.

The committed Step 2 proof preserves the compatible dynamic stack-source
`LoadLocal` path: the non-duplicated fixture still asserts exact
`lw a1, 12(s2)`, keeps selected source-memory facts at offset `12`, and keeps
Route 3 / Route 5 authority agreeing on the selected offset `12` memory-source
row.

## Suggested Next

Step 4 should run the acceptance proof for the byte-offset coverage gate using
the supervisor-selected build plus targeted
`^backend_riscv_prepared_edge_publication$` CTest subset.

## Watchouts

Do not claim exhaustive byte-offset coverage from this one row. Step 4 should
preserve the exact-output assertion, route agreement checks, fallback,
status/debug, route-debug, helper/oracle, prepared-printer, wrapper, and
baseline contracts without weakening them.

## Proof

Todo-only packet. No build or tests were run, and no logs were touched.
