# Current Packet

Status: Active
Source Idea Path: ideas/open/533_bir_route_index_standalone_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split One Proven Prerequisite Boundary

## Just Finished

Step 2 split the first mapped route-index prerequisite boundary without moving
implementation bodies. `src/backend/bir/bir_route_index_prereqs.hpp` now owns
the route-index prerequisite includes, forward declarations, and the complete
route4/route7 status and role enums required by value/default initializers in
`bir_route_index.hpp`.

`src/backend/bir/bir_route_index.hpp` now includes the prerequisite header and
declares its API inside `namespace c4c::backend::bir`. `bir.hpp` remains the
compatibility aggregator: it includes the prerequisite header near the top,
keeps the broad model, route1 identity, and route4/route7 record/index
definitions in place, and includes `bir_route_index.hpp` from a top-level
close/reopen namespace boundary.

The direct no-file top-level include probe:

```bash
printf '#include "src/backend/bir/bir_route_index.hpp"\n' | \
  c++ -x c++ -std=c++17 -I/workspaces/c4c -I/workspaces/c4c/src \
    -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser \
    -fsyntax-only -
```

still fails, but the missing declaration list shrank to the expected remaining
complete-type blocker:

- `RouteIndexRecordReference::value` stores `Route1SourceValueIdentity` by
  value, so the forward declaration in `bir_route_index_prereqs.hpp` is not
  sufficient for full standalone compilation.

The previous namespace, standard include, shared ID, route4 status, route7
status/role, route record, and route index declaration blockers are now supplied
by the prerequisite boundary.

## Suggested Next

Step 3 should map and split the next narrow prerequisite needed for standalone
route-index compilation: either reduce `RouteIndexRecordReference` so it no
longer needs a by-value `Route1SourceValueIdentity`, or split a small route1
identity prerequisite boundary with its exact `Value`/`TypeKind` dependencies
proven first. Do not move full route4/route7 records or consumer include sites
yet.

## Watchouts

- Full `Value` and full `Route1SourceValueIdentity` remain in `bir.hpp`; that is
  intentional for this packet.
- Direct consumer include replacement is still unsafe because
  `bir_route_index.hpp` does not yet compile as a top-level include.
- The route4/route7 record and index structs remain broad and should not be
  moved as part of the next packet unless that is explicitly selected and
  proven as the one boundary.

## Proof

Exact delegated proof passed on rerun:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```

Result: 2/2 tests passed. Proof log path: `test_after.log`.

The first exact proof attempt failed before tests because `cc1plus` was killed
while compiling unrelated `backend_aarch64_instruction_dispatch_test.cpp`; the
rerun overwrote `test_after.log` with the passing proof.

Additional compile probes:

- `#include "src/backend/bir/bir.hpp"` as a no-file top-level compile passed.
- `#include "src/backend/bir/bir_route_index.hpp"` as a no-file top-level
  compile still fails only on incomplete by-value `Route1SourceValueIdentity`,
  showing the missing declaration list shrank.
