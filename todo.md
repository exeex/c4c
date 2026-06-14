Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Agreement and Rejection Proof

# Current Packet

## Just Finished

Completed plan Step 4, `Focused Agreement and Rejection Proof`, using the
supported joined-branch compare-join `EdgeStoreSlot` selected-`LoadLocal` route
from idea 261.

Existing focused rows prove the positive Route 3/prepared agreement path and
the naturally reachable fail-closed rows for missing source-memory authority,
join-carrier-only drift, and incomplete prepared source-memory publication.
Prepared-only, stale-publication, byte-offset drift, and cross-publication
mismatch rows remain unsupported by this surface because they require synthetic
or stale prepared state.

## Suggested Next

Execute plan Step 5, `Compatibility Sweep and Closure Readiness`.

Re-check that prepared lookup/status behavior, helper/oracle names, fallback
names, route-debug output, x86 output spelling, and RISC-V behavior were
preserved, then hand back to plan-owner for closure if no blocker remains.

## Watchouts

- Do not count legacy no-publication fallback as positive Route 3/prepared
  agreement proof.
- Do not hand-build `PreparedEdgePublication` rows, stale prepared render
  contract state, or named-case shortcuts.
- Preserve prepared lookup/status behavior, helper/oracle names, fallback
  names, route-debug output, and x86 output spelling.

## Proof

Ran the supervisor-delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

`test_after.log` shows 2/2 default prepared tests passed and 2/2 x86-enabled
tests passed.
