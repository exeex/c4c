# Current Packet

Status: Active
Source Idea Path: ideas/open/533_bir_route_index_standalone_prerequisites.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Promote or Park Route-Index Standalone Use

## Just Finished

Step 3 re-ran the direct no-file top-level include probe for
`src/backend/bir/bir_route_index.hpp` after the prerequisite split:

```bash
printf '#include "src/backend/bir/bir_route_index.hpp"\n' | \
  c++ -x c++ -std=c++17 -I/workspaces/c4c -I/workspaces/c4c/src \
    -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser \
    -fsyntax-only -
```

The probe still fails, so `bir_route_index.hpp` cannot be promoted to direct
standalone use in this packet. The remaining blocker is the expected complete
type requirement:

- `RouteIndexRecordReference::value` stores `Route1SourceValueIdentity` by
  value, so the forward declaration in `bir_route_index_prereqs.hpp` is not
  sufficient for full standalone compilation.

No code/header/include edits were made. No consumer include replacement is safe
while the route-index header fails as a top-level include, because replacing
`bir.hpp` would require reconstructing broad model/route prerequisites instead
of proving a narrow dependency surface.

## Suggested Next

Step 4 lifecycle review should decide whether to close this prerequisite idea
as a proof-backed aggregator-only decision, or split a new source idea for the
separate work needed to make `Route1SourceValueIdentity` and its `Value` /
`TypeKind` dependencies available as a narrow standalone prerequisite.

## Watchouts

- `bir_route_index.hpp` should remain aggregator-only for now.
- Do not claim dependency reduction or replace consumer includes while the
  standalone probe fails.
- The remaining blocker is not a missing forward declaration; it is a by-value
  complete-type requirement for `Route1SourceValueIdentity`.
- Resolving that blocker would require an explicitly authorized packet that
  changes the route1 identity boundary or the `RouteIndexRecordReference`
  layout. This packet did neither.

## Proof

No build/tests were required because this packet made no code/header/include
edits and the supervisor delegated the direct no-file compile probe as the
readiness proof.

```bash
printf '#include "src/backend/bir/bir_route_index.hpp"\n' | \
  c++ -x c++ -std=c++17 -I/workspaces/c4c -I/workspaces/c4c/src \
    -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser \
    -fsyntax-only -
```

Result: failed as expected on:

```text
src/backend/bir/bir_route_index.hpp:65:29: error: field 'value' has incomplete type 'c4c::backend::bir::Route1SourceValueIdentity'
```

No `test_after.log` was created or updated by this packet.
