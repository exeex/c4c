# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace Safe Include Sites

## Just Finished

Step 3: Replace Safe Include Sites completed with no include-site edits.

Checked whether any consumers can safely replace `bir.hpp` with
`bir_route_index.hpp` directly. No safe direct include-site replacement exists
yet because `bir_route_index.hpp` is currently an aggregator-included
declaration fragment, not a standalone public header:

- it is included from `bir.hpp` inside `namespace c4c::backend::bir`, after
  prerequisite BIR model, route1, route4, and route7 declarations are already
  available
- a direct compile probe of `#include "src/backend/bir/bir_route_index.hpp"`
  fails on missing `Function`, `Block`, `Value`, `BlockLabelId`,
  `Route1SourceValueIdentity`, `Route4PublicationAvailabilityIndex`,
  `Route7ComparisonConditionIndex`, route4/route7 status and record types, and
  standard library declarations
- the route-index implementation body `src/backend/bir/bir_route_facade.cpp`
  also dereferences `Block` fields and calls `route1_source_value_identity`, so
  replacing its broad include would still require the earlier BIR route/model
  surface
- route-index consumers found under MIR, prealloc, and focused BIR tests also
  use `bir::Value`, `bir::Block`, `bir::Function`, route4/route7 records, or
  other broad BIR route surfaces, so replacing their broad include would not
  reduce dependency pressure without adding include churn to rebuild the old
  surface

No implementation bodies, route declarations, `bir.hpp`, or
`bir_route_index.hpp` were edited.

## Suggested Next

Step 4 should perform the header split review checkpoint and decide whether to
close the source idea as a narrow compatibility-preserving split or route a
separate mapped packet for making `bir_route_index.hpp` standalone by first
splitting prerequisite route/model declarations.

## Watchouts

- The current `bir_route_index.hpp` placement is namespace- and
  declaration-order-dependent; direct top-level include sites are not viable
  until the header owns its namespace and prerequisite includes/forward
  declarations.
- Making it standalone is not Step 3 include hygiene; it would require a new
  mapped header-boundary packet because route4/route7 records, route1 identity,
  `BlockLabelId`, and core BIR model declarations are still upstream in
  `bir.hpp`.
- Avoid replacing broad `bir.hpp` includes in MIR, prealloc, or route tests with
  multiple route headers just to recover the previous surface.

## Proof

No build/tests were required for this no-code Step 3 decision, per the
delegated proof contract. `test_after.log` was not created or updated by this
packet.

Evidence commands run:

```bash
rg -n '#include "(src/backend/bir/)?bir\.hpp"|#include <(src/backend/bir/)?bir\.hpp>' src/backend/bir src/backend/mir src/backend/prealloc tests/backend/bir
rg -n "bir_route_index|RouteIndex|route_index_|route4_validate_|route7_validate_" src/backend/bir src/backend/mir src/backend/prealloc tests/backend/bir
c4c-clang-tool function-signatures src/backend/bir/bir_route_index.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/backend/bir -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
printf '#include "src/backend/bir/bir_route_index.hpp"\n' | c++ -x c++ -std=c++17 -I/workspaces/c4c -I/workspaces/c4c/src -fsyntax-only -
```

The clang-backed header query and direct compile probe both confirmed that
`bir_route_index.hpp` depends on declarations supplied by `bir.hpp` before its
aggregator include point.
