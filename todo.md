Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Recommend Follow-Up Ideas

# Current Packet

## Just Finished

Completed Step 7: Recommend Follow-Up Ideas by writing
`docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`.
The document dependency-orders follow-up implementation and umbrella ideas,
separates route facade cleanup, named view extraction, publication boundary
cleanup, dump/test vocabulary rewrite, and stack destination authority follow-up,
and records prerequisites for revisiting ideas 647 and 655 without opening or
editing actual idea files.

## Suggested Next

Proceed to Step 8 by writing the stack view and destination authority handoff.
The next packet should define the BIR/prepared/MIR responsibility split for
frame layout, value homes, move bundles, freshness, aggregate stack sources,
and destination authority, including fail-closed MIR behavior and the positive
producer evidence required before ideas 647 or 655 can resume.

## Watchouts

- Step 8 should keep stack destination authority out of Route 4, Route 5, Route
  7, facade status, and dump rows; named BIR views may feed prepared authority
  production but should not become direct MIR-side destination inference.
- Ideas 647 and 655 remain blocked until file 08 identifies explicit prepared
  producer evidence, MIR fail-closed checks, and proof above route dumps.
- Do not edit `plan.md`, `ideas/open/*`, `ideas/closed/*`, implementation
  files, tests, expectations, allowlists, runtime behavior, or root-level log
  files in the Step 8 docs packet.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md && rg -n "follow-up|dependency|Route 4|Route 5|Route 7|named view|facade|dump|publication|stack destination|647|655|prerequisite" docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
