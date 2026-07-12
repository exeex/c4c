# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.6
Current Step Title: Migrate remaining call consumers

## Just Finished

- Plan Step 2.6 removed the remaining target-local edge-publication source
  producer lookup rebuild from `calls.cpp`. Indirect-callee and call-boundary
  source materialization now require traversal-attached common lookup ownership
  and fail closed when that authority is absent, while preserving call ABI
  behavior.

## Suggested Next

- Execute Plan Step 2.7 as a bounded packet to remove remaining target-local
  lookup rebuilding from global and publication consumers.

## Watchouts

- Step 2.6 required no test changes or common contract changes. Preserve the
  owner/pointer identity check for executable consumers; a non-null borrowed raw
  lookup pointer is not sufficient authority.

## Proof

- Passed the exact supervisor-selected proof: `cmake --build --preset default`
  followed by `ctest --test-dir build -j --output-on-failure -R
  '^(backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_(byval_global_payload(_address)?|hfa_global_payload|f128_hfa_global_payload)_call_boundary)$'`
  (5/5). Canonical combined proof output is in `test_after.log`; the subset was
  sufficient for this bounded Step 2.6 call-consumer migration.
