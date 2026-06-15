Status: Active
Source Idea Path: ideas/open/280_phase_f5_edge_publications_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Real Consumer Fixture

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting the existing
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
RISC-V dynamic `LoadLocal` stack-source fixture as the real backend consumer
surface. The consumer path is
`riscv::consume_edge_publication_move_intent(&lookups, predecessor, successor,
2, &route5_edge)`, which preserves the prepared publication pointer and records
Route 5 agreement status while driving the RISC-V move-rendering path.

Selected prepared-only public row shape: the normal
`PreparedFunctionLookups::edge_publications` row produced from
`make_register_edge_publication_module()` plus
`make_load_local_dynamic_stack_source(..., load_local_source_access(ids))`,
keyed as predecessor `left`, successor `join`, destination value id `2`
(`%dst`). The row publishes source value id `1` (`%src`) with
`source_producer_kind = LoadLocal`, source home `StackSlot`, destination home
`Register`, selected source memory access at predecessor block `left`,
instruction index `0`, pointer base `%base`, byte offset `12`, size `4`, and
destination register `a1`.

Route 5 authority to leave absent or mismatched in Step 2: provide the real
consumer with a `bir::Route5CfgEdgePublicationRecord` whose status is
`NoSource` for the same requested public row, for example by querying a Route 5
index that has the `join` / `%dst` publication but not the `left -> join`
predecessor source. The public `edge_publications` row remains visible, but no
matching Route 5 semantic edge authority agrees with predecessor `left`,
successor `join`, destination `%dst`, source `%src`, and the LoadLocal memory
identity.

## Suggested Next

Execute Step 2: add the focused RISC-V proof in the selected fixture by keeping
the prepared public `edge_publications` row visible and passing the Route 5
`NoSource` edge record into the real consumer. Assert fail-closed behavior with
no instruction emitted, the publication pointer still preserved, Route 5 status
recorded as `NoSource`, and Route 5 / Route 3 agreement false.

## Watchouts

- Do not claim helper-only lookup rejection as backend consumer proof.
- Use the dynamic `LoadLocal` row, not the scalar register row: current scalar
  behavior still permits prepared fallback on non-agreeing Route 5 facts.
- Keep the compatible positive path with matching Route 5 `MemorySource`
  authority and exact `lw a1, 12(s2)` output for Step 3.
- Do not weaken fallback, status/debug, route-debug, prepared-printer,
  helper/oracle, wrapper, exact-output, or baseline contracts.
- Keep this runbook to one bounded prepared-only `edge_publications` row.

## Proof

Inspection-only packet. No build or tests were run, and `test_before.log` /
`test_after.log` were not touched.
