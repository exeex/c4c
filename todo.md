Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 ("Re-Baseline And Choose The First Family") by
refreshing the current backend fail surface and selecting a c-testsuite-credible
first lane: dynamic indexed `gep` local-memory on integer arrays. The bounded
proving cluster is `00143` plus `00176`, with
`backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`
and `_store_...` kept as backend-only probes for the same mechanism. Idea `56`
still stays parallel: the current evidence points at semantic `lir_to_bir`
`gep` gaps rather than prepared-module renderer de-headerization as the
immediate blocker for this packet.

## Suggested Next

Execute `plan.md` Step 2 by tightening backend notes and x86 handoff
expectations around the dynamic indexed `gep` local-memory lane, using
`00143` and `00176` as the external proving cluster and the local dynamic
member-array backend cases as the bounded backend boundary for that family.

## Watchouts

- Do not reopen idea `55`; the memory ownership split is closed.
- Treat idea `56` as a separate initiative; the current Step 1 baseline still
  does not show renderer de-headerization as the immediate blocker.
- Do not weaken `x86_backend` expectations or add testcase-shaped shortcuts.
- `00143` and `00176` both fail in `gep local-memory` and are a coherent first
  external lane; `00181` also fails in the same family but adds recursive
  multi-function control flow and should stay as a nearby follow-on check, not
  part of the first proving cluster.
- `00154` is still blocked by the minimal x86 prepared-module/control-flow
  handoff rather than the newly selected local-memory lane.
- `00195` and `00204` are still blocked earlier by global-data/bootstrap
  lowering, so they must not be used to justify or reject the first local-
  memory `gep` packet.

## Proof

Step 1 investigation used targeted `ctest` probes for `00143`, `00176`,
`00181`, `00154`, `00195`, and `00204` to separate the dynamic indexed `gep`
local-memory lane from unrelated global-data and minimal-handoff blockers.

Delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_" |& tee test_after.log'`

Result: the refreshed backend subset still fails only in the dynamic local-
memory surface (`variadic_double_bytes`, `variadic_pair_second`,
`local_dynamic_member_array`, and `local_dynamic_member_array_store`), and
`test_after.log` is the canonical proof log for this packet.
