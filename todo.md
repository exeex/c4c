Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Structure Snapshot

# Current Packet

## Just Finished

Step 1 - Establish Structure Snapshot completed the initial BIR core model
snapshot in `docs/bir_core_cleanup/structure_snapshot.md`.

The snapshot records line counts for `src/backend/bir/bir.hpp` and
`src/backend/bir/bir.cpp`, confirms `c4c-clang-tool` and
`c4c-clang-tool-ccdb` on `PATH`, records exact AST query commands, summarizes
symbol/function/type-reference results, notes the one non-blocking caller query
fallback, and starts initial declaration/implementation clusters for the
durable cleanup analysis.

## Suggested Next

Delegate Step 2: inventory declaration families in `src/backend/bir/bir.hpp`
into core model, route-local analysis, memory authority, call publication,
validation facade, and printer/render surfaces, then extend
`docs/bir_core_cleanup/structure_snapshot.md` or a sibling artifact with a
declaration-family table.

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

## Proof

Proof command:

```sh
git diff --check && test -s docs/bir_core_cleanup/structure_snapshot.md
```

Result: passed.

No implementation files were changed. No `test_after.log` was produced because
the delegated proof command was a docs/lifecycle sanity check and did not write
one.
