Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Publication And Authority Boundaries

# Current Packet

## Just Finished

Completed Step 4: Classify Publication And Authority Boundaries by writing
`docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`.
The document classifies Route 4, Route 5, and Route 7 interactions with
prepared publication, freshness, and residual stack destination authority;
separates codegen-authoritative prepared records from observational route proof
records; and records reviewer reject rules for authority claims sourced only
from route-numbered debug records.

## Suggested Next

Proceed to the next research packet by turning the publication/authority
boundary into a concrete retirement sequence for the highest-value compatibility
fields, starting with Route 4 block-entry attribution and Route 5 stored
join-source agreement.

## Watchouts

- Route 5 raw CFG edge publication can remain a named BIR semantic fact, but
  `route5_join_source_agrees` is still proof-only and must not substitute for
  selected freshness or destination authority.
- Route 4 block-entry attribution currently lives on prepared query output, but
  the document classifies its route status and instruction index as
  observational proof rather than executable publication authority.
- Route 7 should stay in comparison/control-value validation; it must not be
  used to justify publication, freshness, stack destination, or move execution
  authority.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md && rg -n "Route 4|Route 5|Route 7|publication|freshness|destination authority|codegen|observational|diagnostic|proof|reject|PreparedEdgePublicationLookups|PreparedMoveBundleLookups|PreparedMirCoreView" docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
