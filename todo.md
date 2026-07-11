Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Define Stack View And Destination Authority Handoff

# Current Packet

## Just Finished

Completed Step 8: Define Stack View And Destination Authority Handoff by writing
`docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md`.
The document assigns BIR semantic view, prealloc/prepared producer, and MIR
consumer responsibilities; proposes first-cut frame layout, value home, move
bundle, and destination authority view shapes; explains Route 4, Route 5, and
Route 7 feed boundaries; records positive producer evidence required before
ideas 647 or 655 can resume; and states fail-closed MIR behavior for stack
destination fan-in without explicit prepared authority.

## Suggested Next

Proceed to Step 9 by assembling `docs/bir_route_index_retirement_research/index.md`
and verifying the research package. The next packet should link all eight
answer files, summarize the route-retirement strategy without replacing the
answer files, and check the package has exactly the required index plus eight
numbered documents.

## Watchouts

- Step 9 should not collapse the answer files into the index; the index should
  summarize and link them.
- Preserve the Step 8 boundary in the package summary: Route 4, Route 5, and
  Route 7 may feed prepared producers through named BIR views, but MIR must
  consume explicit prepared stack authority and fail closed on route-only
  evidence.
- Do not edit `plan.md`, `ideas/open/*`, `ideas/closed/*`, implementation
  files, tests, expectations, allowlists, runtime behavior, or root-level log
  files in the Step 9 assembly packet.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md && rg -n "BIR semantic view|prealloc|prepared|MIR|frame layout|value home|move bundle|destination authority|freshness|aggregate stack|fail closed|Route 4|Route 5|Route 7|647|655|positive producer evidence" docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
