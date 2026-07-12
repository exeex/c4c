# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate direct dispatch-producer consumers

## Just Finished

- Plan Step 2.2 removed direct same-block select-producer discovery and the
  generated edge-publication producer-lookup fallback from
  `dispatch_producers.cpp`. Select adaptation and publication queries now
  require the owned, attached prepared lookups and fail closed when the common
  producer record is absent or inconsistent.

## Suggested Next

- Execute Plan Step 2.3 against the select/comparison value-home lookup
  rebuilding family.

## Watchouts

- The scalability route has attached prepared function lookups; no missing
  attachment was observed before timeout. The same case also exceeds 8 seconds
  from parent commit `1e5c28889`, before the Step 2.2 dispatch-producer change,
  so its runtime is not isolated to this owned consumer slice. The pre-existing
  selected-global-load fused-branch stale-stack-home failure also remains.

## Proof

- The delegated proof stopped at
  `backend_codegen_route_aarch64_prepared_call_boundary_scalability`, which
  deterministically exceeded its 5-second case timeout. Because the exact
  command short-circuits there, its focused four-test `tee test_after.log`
  stage did not run; the prior 3/4 baseline-comparable focused log remains in
  `test_after.log`.
