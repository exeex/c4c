# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Remaining Route Header Candidates

## Just Finished

Step 1 of `plan.md` mapped the remaining BIR route-header candidates after
the route-index prerequisite closure.

Candidate map:

- `bir_route_index.hpp` remains aggregator-only. A direct no-file include probe
  for `#include "src/backend/bir/bir_route_index.hpp"` still fails because
  `RouteIndexRecordReference::value` stores `Route1SourceValueIdentity` by
  value while the prerequisite header only forward-declares it.
- Route 1 is not a candidate under this idea. `Route1SourceValueIdentity`
  owns the complete identity shape for route records and directly depends on
  `Value`, `Value::Kind`, `TypeKind`, `ValueNameId`, and `LinkNameId`; moving it
  would violate the packet constraints and reopen the prerequisite idea.
- Route 3 is not a candidate. Its memory-access records are part of the later
  memory-provenance route and are referenced by route 5 and route 6 records.
- Route 4 and route 7 declarations already feed the existing route-index
  facade boundary. Their status/role prerequisites are split, but their full
  records/indexes still store `Route1SourceValueIdentity` and core BIR model
  pointers/references, so moving more would create another aggregator-only
  fragment rather than a narrower standalone header.
- Route 5 is not a safe boundary. Its records store route 1 identities,
  `Value::Kind`, `Block`/`Inst`/`PhiInst` pointers, and a by-value
  `Route3MemoryAccessRecord`, so it pulls both route1 identity and memory
  provenance surfaces.
- Route 6 is not a safe boundary. Its records compose route1 producer and
  materialization records, route2 direct-global records, route3 memory records,
  route4 publication records, route5 edge records, and `CallInst`/`Value` core
  model types.
- Route 8 is not a safe boundary. It is smaller, but its key and records still
  store `Route1SourceValueIdentity` by value and its signatures use `Function`,
  `Block`, and `Value`. Moving it without route1 identity ownership would only
  reproduce the route-index aggregator-only pattern.
- No direct include replacement is supported. Include/user scans still show
  broad `bir.hpp` consumers across BIR, MIR, prealloc, and tests, while the only
  direct route-index include is the aggregator include inside `bir.hpp`.

Selected next action: recommend closure/parking for closure with no more header
edits under idea 530. The remaining work that could make narrower headers real
requires a separately owned route1 identity or core-model dependency split, or
later memory-provenance/local-array work, all outside this idea.

## Suggested Next

Delegate Step 2 as the closure path: do not edit headers; record that idea 530
has no remaining safe route declaration movement that reduces dependencies.
Then ask the plan owner for the lifecycle checkpoint/close decision.

## Watchouts

- Do not edit implementation or header files during Step 1.
- Do not force `bir_route_index.hpp` standalone by moving route1 identity
  ownership or changing `RouteIndexRecordReference` layout.
- Keep memory-provenance and local-array/semantic-GEP header work for later
  ordered ideas.
- Treat any new route-only fragment that still depends on by-value
  `Route1SourceValueIdentity` as route drift unless the supervisor explicitly
  approves an aggregator-only cleanup.
- If the supervisor wants more dependency reduction, the next source idea
  should own route1 identity/core-model prerequisite splitting or a record layout
  redesign; that is not a Step 2 implementation packet for idea 530.

## Proof

Mapping-only packet; no build/tests required and `test_after.log` was not
created or updated.

Evidence gathered:

- `c4c-clang-tool list-symbols src/backend/bir/bir.hpp -- ...`
- `c4c-clang-tool list-symbols src/backend/bir/bir_route_index.hpp -- ...`
  returned route-index declarations but failed standalone on incomplete
  `Route1SourceValueIdentity`.
- Direct no-file compile probe for
  `#include "src/backend/bir/bir_route_index.hpp"` failed on the same by-value
  `Route1SourceValueIdentity` field.
- `c4c-clang-tool type-refs src/backend/bir/bir.hpp Route1SourceValueIdentity`
  showed by-value route references across route3/4/5/6/7/8 records and route8
  return values.
- `c4c-clang-tool type-refs src/backend/bir/bir.hpp Route3MemoryAccessRecord`
  showed route5 and route6 by-value memory-record dependencies.
- Include/user `rg` scans showed `bir.hpp` remains the broad consumer surface in
  BIR, MIR, prealloc, and tests; `bir_route_index.hpp` has no safe direct
  include-site replacement yet.

Suggested close proof for the supervisor/plan owner, if they accept the
closure recommendation:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```
