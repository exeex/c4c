Status: Active
Source Idea Path: ideas/open/51_aarch64_alu_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Unpublished Load-Local Producer Authority

# Current Packet

## Just Finished

Step 3 regression fix completed.

Fixed `c_testsuite_aarch64_backend_src_00164_c` by keeping the shared
unpublished load-local source producer's BIR instruction index separate from
the prepared memory-access index. When the position-indexed memory access is
missing or belongs to a different result value, the shared helper now recovers
the load source through the result-value indexed prepared memory-access lookup.

Kept the ALU path consuming the shared prepared authority and clearing stale
emitted-register state when that authority supplies a memory source, so later
retargeting cannot replace the recovered load-local source with an obsolete
register value.

Adjusted focused store-source publication-plan coverage to exercise a valid
source-producer whose position-indexed memory access points at a different
load, plus the existing missing-index and intervening-store safety cases.

## Suggested Next

Proceed to supervisor review or the next supervisor-selected packet. This
regression fix is ready for broader validation if the supervisor wants to
roll the full-suite baseline forward.

## Watchouts

Prepared memory-access `inst_index` is not always the raw BIR instruction
index after preparation. Shared source-authority code must not treat a
position-indexed memory-access hit as authoritative unless it also matches the
source result value.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'set -o pipefail; cmake --build --preset default 2>&1 | tee test_after.log; ctest --test-dir build -j --output-on-failure -R "^(c_testsuite_aarch64_backend_src_00164_c|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_prepared_lookup_helper|backend_publication_plan_record|backend_store_source_publication_plan)$" 2>&1 | tee -a test_after.log'`

Result: build succeeded; all 9 delegated tests passed, including
`c_testsuite_aarch64_backend_src_00164_c`. Proof log:
`test_after.log`.
