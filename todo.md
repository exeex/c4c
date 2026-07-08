Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Readiness Summary

# Current Packet

## Just Finished

Step 5 from `plan.md`: completed the closure-readiness summary for idea 605 without implementation edits or lifecycle close/archive.

Implementation surface now supported by committed Step 3 and Step 4 evidence:
- Collision-safe local aggregate and scalar-array carrier slot names are in place for the repaired alloca-local path.
- Zero-size struct aggregate facts are present as part of the same aggregate/local carrier surface, with `src/zero-struct-1.c` moving beyond the original alloca local-memory producer stop to the explicit load owner.
- The route did not weaken expectations, unsupported markers, allowlists, diagnostics, or testcase contracts; progress is from producer repair and breadth classification, not expectation/accounting edits.

Closure evidence:
- All 11 current rows that originally stopped at the alloca local-memory producer moved beyond that producer boundary: `src/20180921-1.c`, `src/pr38151.c`, `src/20111208-1.c`, `src/931004-10.c`, `src/931004-12.c`, `src/931004-14.c`, `src/bswap-2.c`, `src/pr42833.c`, `src/pr82524.c`, `src/strct-stdarg-1.c`, and `src/zero-struct-1.c`.
- The 65-row Step 4 classification has zero current `alloca local-memory producer` stops.
- Adjacent owner boundaries stayed explicit: `load` 9, `GEP` 1, `store` 2, `vector` 1, `scalar-cast` 1, unordered compare 7, runtime 1, and success 21.

## Suggested Next

Recommended lifecycle action: close idea 605. The source idea acceptance criteria are satisfied for alloca-local repair breadth because multiple alloca rows, including all selected Step 3 representatives and eight additional same-family rows, progressed beyond the old BIR producer stop while load/store/GEP and other adjacent owners remained separate.

If not already captured elsewhere, open or route a follow-up for the 22 remaining scalar/local-memory producer rows. Those rows are adjacent follow-up scope, not a reason to keep this alloca-local repair runbook open.

## Watchouts

Remaining limitations and boundaries:
- 22 scalar/local-memory producer rows remain: `src/20020411-1.c`, `src/20041201-1.c`, `src/20070614-1.c`, `src/complex-2.c`, `src/complex-5.c`, `src/complex-6.c`, `src/ffs-2.c`, `src/20020227-1.c`, `src/20021118-2.c`, `src/921013-1.c`, `src/961223-1.c`, `src/builtin-bitops-1.c`, `src/ffs-1.c`, `src/ieee/acc1.c`, `src/ieee/acc2.c`, `src/pr42248.c`, `src/pr42691.c`, `src/pr47538.c`, `src/pr56837.c`, `src/pr61725.c`, `src/scal-to-vec1.c`, and `src/scal-to-vec2.c`.
- Downstream load, GEP, store, vector, scalar-cast, unordered-compare, and runtime boundaries remain adjacent-owner work.
- Do not reclassify the remaining scalar/local-memory producer rows as Step 3 alloca-local fallout unless a future source idea explicitly owns that scalar producer surface.

## Proof

No new tests were run for this closure-summary-only packet.

Committed proof evidence:
- Step 3 selected proof: `16/16` passed.
- Step 3 backend naming-sensitive proof: `9/9` passed, preserved in `build/agent_state/605_step3_backend_focused_after.txt`.
- Step 3 full backend proof: `346/346` passed, preserved in `build/agent_state/605_step3_backend_after.txt`.
- Step 4 broad same-family proof: build plus RV64 gcc_torture subset passed `65/65`, recorded in the previous canonical `todo.md` packet as `test_after.log`; the root `test_after.log` file is not present in this checkout, but the durable Step 4 classification artifacts are under `build/agent_state/605_step4_breadth/`.

Durable Step 4 artifacts:
- `build/agent_state/605_step4_breadth/current_rows.tsv`
- `build/agent_state/605_step4_breadth/current_counts.tsv`
- `build/agent_state/605_step4_breadth/moved_beyond_alloca.tsv`
- `build/agent_state/605_step4_breadth/remaining_in_scope_producers.tsv`
- `build/agent_state/605_step4_breadth/adjacent_downstream_rows.tsv`
- `build/agent_state/605_step4_breadth/summary.md`

Closure-readiness assessment: acceptance criteria are satisfied for alloca-local repair breadth, with the remaining scalar/local-memory producer rows and downstream load/GEP/store/vector/scalar-cast/unordered-compare/runtime rows correctly left for follow-up or adjacent owners.
