# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce One Narrow Route Header

## Just Finished

Step 2: Introduce One Narrow Route Header completed.

Moved only the selected route-index facade/validation declaration cluster out
of `src/backend/bir/bir.hpp` into
`src/backend/bir/bir_route_index.hpp`:

- `RouteIndexRoute`, `RouteIndexOwnerScope`,
  `RouteIndexRecordCategory`, `RouteIndexRelationshipKind`, and
  `RouteIndexValidationStatus`
- `RouteIndexRecordReference`, `RouteIndexReferenceFacade`,
  `Route4IndexReferenceValidation`, and
  `Route7IndexReferenceValidation`
- `route4_validate_*`, `route7_validate_*`,
  `route_index_reference_facade`, and `route_index_validate_*`
  declarations

`bir.hpp` remains the broad public compatibility include and now includes
`bir_route_index.hpp` at the original route-index cluster location, after the
route-4 and route-7 record/status declarations are available. No implementation
bodies, route1-route8 record/index clusters, core model declarations, public
signatures, namespaces, or enum values were changed.

## Suggested Next

Step 3 should decide whether any narrow consumers can include
`src/backend/bir/bir_route_index.hpp` directly, or whether this header should
remain an aggregator-included declaration fragment until more route record/index
dependencies are split.

## Watchouts

- `bir_route_index.hpp` currently depends on earlier BIR model and route
  declarations supplied by `bir.hpp`; it is intentionally included at the
  original dependency point.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not move route1-route8 record/index clusters without a separate mapped
  boundary, because their complete-type/container dependencies remain dense.
- Do not replace broad `bir.hpp` includes at implementation sites unless the
  next packet explicitly owns the affected files and proves the narrower include
  path.

## Proof

Exact delegated proof passed on rerun:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```

`test_after.log` contains the passing result: 2/2 tests passed
(`backend_prepared_lookup_helper` and
`backend_aarch64_branch_control_lowering`).

The first run of the same command failed during the build because `cc1plus` was
killed while compiling an unrelated MIR test object; the rerun completed and
overwrote `test_after.log` with the passing proof.
