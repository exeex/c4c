Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Separate Required BIR-To-Prealloc Inputs

# Current Packet

## Just Finished

Completed Step 2: Separate Required BIR-To-Prealloc Inputs by writing
`docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`.
The document separates minimum prepared/codegen inputs from route-index
proof/debug fields, identifies Route 4/Route 5/Route 7 agreement facts that
should be recomputed locally instead of stored in prepared state, and cites the
current `PreparedFunctionLookups` and `PreparedMirCoreView` boundaries.

## Suggested Next

Proceed to Step 3 by defining the named view replacement shape for the
route-index retirement path, using `PreparedFunctionLookups` and
`PreparedMirCoreView` as the prepared-side ownership boundary.

## Watchouts

- Route-index state is not a minimum codegen input; the durable prepared
  boundary is the semantic lookup data in `PreparedFunctionLookups` plus the
  BIR binding exposed by `PreparedMirCoreView`.
- Route 4 block-entry attribution, Route 5 join-source agreement, and Route 7
  comparison validation should remain local proof/debug recomputation surfaces
  until later implementation work replaces those diagnostics.
- Keep subsequent work documentation-only until the supervisor delegates an
  implementation packet.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md && rg -n "codegen input|debug|proof|diagnostic|recomput|prealloc|PreparedFunctionLookups|PreparedMirCoreView|Route4|Route5|route-index" docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
