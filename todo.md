Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Destination Map

# Current Packet

## Just Finished

Step 4 - Draft Destination Map completed
`docs/bir_core_cleanup/destination_map.md`.

The map uses the Step 1-3 artifacts without additional C++ queries and proposes
destinations for each inventoried family. It separates existing-file
redistribution candidates, possible new route/facade/memory files, retained
central public model surfaces, include/API risks, late or no-move regions, and
a sequencing signal for Step 5 follow-up ideas.

## Suggested Next

Delegate Step 5: produce staged behavior-preserving follow-up ideas from
`docs/bir_core_cleanup/destination_map.md`, with owned files, non-goals,
validation expectations, and reviewer reject signals for each proposed cleanup
slice.

## Watchouts

- This idea is analysis-only; do not move declarations or definitions.
- Continue using AST-backed structure queries before raw long-file reading.
- Keep idea 422 producer implementation work separate from cleanup planning.
- Do not change tests, expectations, unsupported diagnostics, or pass/fail
  accounting.
- Route3 memory access is directly used by route6 call publication source
  analysis; do not propose route3 extraction without recording that coupling.
- `Value`, `Inst`, `Block`, `Function`, and `Module` have broad fanout and
  should remain late/no-move candidates until lower-risk route families are
  classified.
- Route6 call publication is the highest-risk route implementation family
  because it composes route1, route2, route3, route4, route5, call ABI, and
  tail query helpers.
- `bir_printer.cpp` and `bir_validate.cpp` are already clear implementation
  owners for their private helpers; Step 4 should distinguish those existing
  owners from public API declarations.
- Treat `lir_to_bir/` as a private lowering subsystem; do not propose moving
  public BIR model records into that tree unless they are proven lowering-only.
- Step 5 should keep body-only route extraction ideas separate from later
  declaration/header extraction ideas; early body movement is lower-risk than
  public header splitting.
- Avoid proposing one giant `bir_routes.hpp`; the destination map favors
  per-route or small-family headers only after route dependency direction is
  proven.

## Proof

Proof command:

```sh
git diff --check && test -s docs/bir_core_cleanup/destination_map.md
```

Result: passed.

No implementation files were changed. No `test_after.log` was produced because
the delegated proof command is a docs/lifecycle sanity check and does not write
one.
