Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Propose Named BIR View Replacements

# Current Packet

## Just Finished

Completed Step 3: Propose Named BIR View Replacements by writing
`docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`.
The document proposes first-cut named C++ ownership boundaries for Producer,
Memory access, Publication, Control-flow value, Call boundary, and Return-chain
views; maps Route 1 through Route 8 records into those views; marks each view as
public, private, or compatibility-only; and cites the current
`PreparedFunctionLookups` and `PreparedMirCoreView` boundaries.

## Suggested Next

Proceed to Step 4 by classifying publication and authority boundaries, with
special attention to keeping Route 4, Route 5, and Route 7 agreement records as
diagnostic/proof surfaces unless a named prepared authority explicitly owns the
fact.

## Watchouts

- The Step 3 shape intentionally keeps `RouteIndexReferenceFacade`,
  `Route4IndexReferenceValidation`, and `Route7IndexReferenceValidation` in a
  compatibility/proof layer rather than making them the durable public model.
- Publication facts may be public BIR semantic facts, but prepared execution
  authority remains in `PreparedEdgePublicationLookups`,
  `PreparedMoveBundleLookups`, value homes, and `PreparedMirCoreView`.
- Step 4 should sharpen reject rules for authority claims sourced only from
  route-numbered debug or agreement records.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md && rg -n "Producer|Memory|Publication|Control|Call|Return|public|private|compatibility|Route1|Route2|Route3|Route4|Route5|Route6|Route7|Route8|PreparedFunctionLookups|PreparedMirCoreView" docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
