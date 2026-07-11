Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Define The Retirement Sequence

# Current Packet

## Just Finished

Completed Step 5: Define The Retirement Sequence by writing
`docs/bir_route_index_retirement_research/05_retirement_sequence.md`.
The document proposes behavior-preserving phased commits that introduce named
views before consumer moves, names Route 4 block-entry attribution as the first
low-risk migration, records rollback points and focused proof commands, and
defers Route 5, Route 7, and the broader Route 1/2/3/6/8 families until later
BIR/prealloc rebuild work has named authority surfaces.

## Suggested Next

Proceed to Step 6 by defining the post-retirement test and dump policy for
route/prepared fact checks, including which route-numbered dump labels should
be kept, gated, rewritten, or deleted after named views become the public
surface.

## Watchouts

- Keep implementation packets small: add named compatibility wrappers before
  moving consumers, then migrate Route 4 prepared lookup attribution before
  printer spelling or facade deletion.
- Route 5 stored join-source agreement should stay as compatibility state until
  prepared publication, freshness, move-bundle authority, printer policy, and
  target consumers have a named proof path.
- Route 7 should remain a comparison/control-value proof surface. Do not let it
  become publication, freshness, stack destination, move execution, or prepared
  MIR authority during retirement.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/05_retirement_sequence.md && rg -n "phase|rollback|proof command|ctest|Route 4|Route 5|Route 7|first low-risk|defer|behavior-preserving|compatibility|named view" docs/bir_route_index_retirement_research/05_retirement_sequence.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
