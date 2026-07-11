Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current Route Ownership

# Current Packet

## Just Finished

Completed Step 1: Inventory Current Route Ownership by writing
`docs/bir_route_index_retirement_research/01_current_route_inventory.md`.
The inventory tables every `bir_route1` through `bir_route8` implementation
file, the `bir_route_facade` and `bir_route_index` public record families, and
the observed direct consumers in prealloc, prepared-printer, and prepared MIR
lowering/view code.

## Suggested Next

Proceed to Step 2 by classifying which current `bir_route_index` consumers can
move to `prepare::PreparedFunctionLookups` or `mir::prepared::PreparedMirCoreView`
without changing runtime behavior.

## Watchouts

- `bir_route_index` is currently a narrow Route 4/Route 7 validation facade, not
  a registry for all `bir_routeN` files.
- Route 1, Route 2, Route 3, Route 5, Route 6, and Route 8 have independent raw
  route consumers that should not be treated as retired by facade cleanup alone.
- Keep subsequent work documentation-only until the supervisor delegates an
  implementation packet.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/01_current_route_inventory.md && rg -n "bir_route1|bir_route2|bir_route3|bir_route4|bir_route5|bir_route6|bir_route7|bir_route8|bir_route_facade|bir_route_index|prealloc|prepared_view" docs/bir_route_index_retirement_research/01_current_route_inventory.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not created because the packet marked this as docs-only and forbade touching
root-level `.log` files.
