# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish pointer-select aggregate-copy baseline and probes

## Just Finished

Lifecycle reset from calls prepared-authority repair to dispatch value
materialization repair, based on `review/calls-step3-route-review.md`.

The reviewer judged the existing cursor and edge-copy fixes as semantic, not
testcase-overfit, but found the proposed next `%t49` pointer-select /
aggregate-copy materialization packet to be route drift under
`ideas/open/52_aarch64_calls_prepared_authority_repair.md`.

Current uncommitted implementation context:

- `src/backend/mir/aarch64/codegen/memory.cpp` updates the va_list field from
  the loaded cursor for the generic `load field -> add stride -> store same
  field` pattern.
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` publishes a
  prepared `LoadLocal` edge source from an authoritative register home before
  considering source memory reloads.
- These changes improved `c_testsuite_aarch64_backend_src_00204_c` from a
  segmentation fault to `[RUNTIME_MISMATCH]`, but the focused proof is still
  5/6, so the code slice is not acceptance-ready.

The remaining known failure is after `vaarg.join.39`: `%t49` pointer/select
aggregate-copy materialization still rebuilds pointer-derived scratch state via
32-bit moves such as `mov w9, w13; sxtw x9, w9`, and later `addr %t49+N`
byte-copy loads consume the bad pointer-derived value.

## Suggested Next

Delegate Step 1 under the dispatch value-materialization route:

- Re-run or reuse the focused subset that currently passes 5/6 and fails only
  `c_testsuite_aarch64_backend_src_00204_c`.
- Dump BIR, prepared BIR, and assembly around `vaarg.join.39`, `%t49`, and the
  `addr %t49+N` aggregate byte-copy loads.
- Add or identify semantic probes for pointer-valued select results feeding
  byte-copy/address-offset loads and for the variadic aggregate-copy path after
  the cursor fix.
- Record which prepared value-home, block-entry publication, edge-publication
  source, or select-chain fact should own the authoritative pointer source.

The next code-changing packet should not be accepted unless it has same-feature
semantic probe proof in addition to `00204`.

## Watchouts

Do not treat the current `memory.cpp` and `dispatch_edge_copies.cpp` changes as
committable merely because they removed the original cursor crash. Preserve the
fixed overflow-block shape while investigating `%t49`, but keep the slice
classified as incomplete until the pointer materialization mismatch is repaired
and proven.

Keep `ideas/open/52_aarch64_calls_prepared_authority_repair.md` open and parked.
This reset does not close the calls route or the earlier ALU route.

If Step 1 proves the remaining repair requires a broader shared contract across
calls, dispatch edge copies, and value materialization, stop for lifecycle split
instead of expanding the dispatch plan silently.

## Proof

No validation run in this lifecycle reset.

Most recent executor proof before reset:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: 5/6 passed. Passing tests were the two ALU prepared-authority probes,
`00164`, `00176`, and `00181`; failing test was
`c_testsuite_aarch64_backend_src_00204_c` with `[RUNTIME_MISMATCH]`.

Proof log: `test_after.log`.
