Status: Active
Source Idea Path: ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove compatibility retention across x86 and riscv

# Current Packet

## Just Finished

Step 5 - Prepared the compatibility-retention proof across prepared lookup
helper, x86 route-debug, and riscv edge-publication tests.

The proof surface preserves the public prepared helper status names, x86
route-debug/`ConsumedPlans` compatibility rows, and riscv prepared-backed
wrapper-output assertions. Read-only inspection confirmed the retained riscv
wrapper output checks still cover `mv a1, a0` and `lw a1, 12(s2)`, while the
prepared helper surface still exposes the compatibility status names protected
by the earlier packets.

## Suggested Next

Supervisor close review for Step 6: decide whether idea 246 can close using the
matched before/after proof for prepared lookup helper, x86 route-debug, and
riscv edge-publication tests.

## Watchouts

No Step 5 blockers found. Close review should not claim adapter work, prepared
status demotion, aggregate retirement, or draft 155 readiness.

## Proof

Delegated proof to run exactly:

`cmake --build build-x86 --target backend_prepared_lookup_helper_test backend_x86_route_debug_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_x86_route_debug|backend_riscv_prepared_edge_publication)$' | tee test_after.log`

Result: passed 3/3:
`backend_prepared_lookup_helper`, `backend_x86_route_debug`, and
`backend_riscv_prepared_edge_publication`.

Proof log: `test_after.log`.
