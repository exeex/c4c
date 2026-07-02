Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select extraction boundary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: selected the body-only extraction boundary for
route2 select-chain implementation based on the committed Step 1 audit.

Selected destination:

- Move route2 select-chain implementation bodies to
  `src/backend/bir/bir_route2.cpp`.
- Preserve public route2 declarations, enums, query/index/value record types,
  and `Route2SelectChainDirectGlobalDependencyRecord` in `bir.hpp`.
- Do not add route2 public surface area and do not split the public route2
  declarations out of `bir.hpp` in this plan.

Included body/helper boundary for Step 3:

- Include public route2 bodies now implemented in `bir.cpp`:
  `route2_select_chain_producer_kind`,
  `route2_select_chain_producer_record`,
  `route2_select_chain_value_record`,
  `route2_build_select_chain_value_index`, and
  `route2_find_select_chain_value_record`.
- Include the route2-owned anonymous helper
  `route2_find_direct_global_dependency` in `bir_route2.cpp`'s anonymous
  namespace.
- Keep the helper private to the route2 TU. Step 1 found no route6 or external
  direct caller, so no `bir_private.hpp` declaration is needed for this helper.

Excluded symbols and behavior:

- Exclude public route2 declarations, records, and enums from the move; they
  stay in `bir.hpp`.
- Exclude route6 bodies and route6 publication helpers from the move.
- Exclude comparison-only, prepared lookup, MIR query, printer, and unrelated
  route helpers from the new route2 TU.
- Preserve the existing direct-global dependency classification, including
  recursive same-block scalar producer handling, local-load availability, direct
  `LoadGlobalInst` recording, `root_is_select`, and root instruction index
  publication.
- Preserve route6 consumption through the public
  `route2_select_chain_value_record` and public
  `Route2SelectChainDirectGlobalDependencyRecord` shape.

Rejected unsafe alternatives:

- Reject preserving all route2 bodies in `bir.cpp`; Step 1 found a cohesive
  body-only route2 set with a route2-owned private helper and no blocking
  private cross-route dependency.
- Reject moving route2 declarations, route2 records, or route2 enums out of
  `bir.hpp`; route6, MIR query code, and prepared lookup tests are legitimate
  public consumers.
- Reject exposing `route2_find_direct_global_dependency` through
  `bir_private.hpp`; no current consumer needs that boundary, and exposing it
  would invite route6/private implementation coupling.
- Reject making route6 call route2-private helpers or otherwise changing route6
  direct-global publication policy.
- Reject changing tests, expectations, call ABI behavior, or direct-global
  semantics as part of the extraction.

Build metadata implication:

- Step 3 should add the new route2 TU to the backend build path and to any
  direct-source BIR test target that currently compiles `bir.cpp` directly and
  needs the moved route2 definitions.

## Suggested Next

Execute Step 3: create `src/backend/bir/bir_route2.cpp`, move only the selected
route2 bodies and the route2-owned anonymous direct-global helper, preserve the
public route2 surface in `bir.hpp`, and update build metadata for the new TU and
any direct-source BIR test target that needs it.

## Watchouts

- Step 3 should not add a `bir_private.hpp` declaration for
  `route2_find_direct_global_dependency`; move it as a private anonymous helper
  with route2 unless the code move exposes a new fact not present in Step 1.
- Keep route6 on the public `route2_select_chain_value_record` call path.
- Do not change direct-global dependency classification, route6 publication
  policy, call ABI behavior, tests, or expectations.
- Do not move comparison-only helpers, route6 helpers, MIR query code, prepared
  lookup logic, or printer logic into the route2 TU.
- Build metadata must account for direct-source BIR tests, not just the
  `c4c_backend` library path.

## Proof

No build or ctest proof was required for this decision-only packet. Ran
`git diff --check -- todo.md`.
