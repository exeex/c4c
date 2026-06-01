Status: Active
Source Idea Path: ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Edge-Copy Publication Inputs

# Current Packet

## Just Finished

Step 1: mapped edge-copy publication inputs in
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.

Relevant prepared-publication carriers:

- Lines 238-247 and 664-673: `emit_edge_value_publication_to_register_impl`
  carries `const prepare::PreparedEdgePublication* prepared_publication`
  through recursive source materialization.
- Lines 639-647: `emit_edge_value_publication_to_register` public wrapper
  requires a matching non-null `PreparedEdgePublication*` before delegating.
- Lines 489-518: `emit_edge_load_local_to_register` and its impl already
  accept `PreparedEdgePublication*` for load-local edge-source recovery.
- Lines 1166-1215: `lower_predecessor_join_source_publication` receives
  `const PreparedEdgePublication& publication`, resolves the destination GPR,
  and calls `emit_edge_value_publication_to_register(..., &publication)`.
- Lines 1244-1274 and 1289-1308:
  `should_emit_block_entry_edge_copy_move` obtains
  `PreparedEdgeCopySourceFacts` from `prepare_edge_copy_source_facts`; those
  facts include `source_facts.publication`.
- Lines 1360-1380: `lower_predecessor_select_parallel_copy_sources` obtains
  `PreparedEdgeCopySourceFacts` from
  `prepare_block_entry_parallel_copy_edge_source_facts`, verifies source and
  destination homes, then passes `*source_facts.publication` into
  `lower_predecessor_join_source_publication`.

Selected insertion point:

Add the typed stack-source preparation inside
`lower_predecessor_join_source_publication`, after destination-register parsing
and scratch selection and before the call to
`emit_edge_value_publication_to_register`. This is the narrow point where the
edge-copy route has the existing `PreparedEdgePublication&`, destination
register authority, source/destination homes, and the emission lines vector for
the actual materialization.

Implementation can proceed without changing `dispatch_publication.cpp`; that
file remains outside this route because the needed `PreparedEdgePublication*`
is already available in the edge-copy source-facts path.

No missing authority blocker found for starting implementation. The next slice
still needs to choose the exact AArch64 emission helper/record shape for an
available `PreparedTypedStackSourcePublication`, but no new prepared lookup or
`dispatch_publication.cpp` threading is required.

## Suggested Next

Execute Step 2: in `lower_predecessor_join_source_publication`, compute
`const auto typed_stack_source =
prepare::prepare_same_width_i32_stack_source_publication(&publication)` and,
when available, emit the same-width I32 stack-slot-to-destination-GPR load
before falling back to the existing recursive publication path.

## Watchouts

- Do not thread `PreparedEdgePublication` into `dispatch_publication.cpp` just
  to satisfy this route.
- Do not add named testcase shortcuts or weaken expected supported behavior.
- Keep AArch64 load/copy/register emission target-local.

## Proof

Not run; delegated proof was read-only inventory plus `todo.md` update only.
No build required and no proof logs created.
