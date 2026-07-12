# Current Packet

Status: Active
Source Idea Path: ideas/open/707_prepared_stack_destination_authority_positive_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory the authority schema and select the first positive producer

## Just Finished

- Completed Plan Step 1's read-only authority inventory.  The named
  `PreparedStackDestinationAuthorityView` does not yet exist: its declaration,
  producer, MIR consumer, status enum, and registered contract are all explicit
  Step 2/3 gaps rather than hidden existing seams.
- Existing prepared owners for the required row are:
  - destination/source value ids and names, source/destination home kinds, and
    the selected move: `PreparedEdgePublication` plus
    `PreparedMoveResolution` (`publication_plans.hpp:1481`, `regalloc.hpp:208`);
  - complete home/slot geometry: `PreparedValueHome`, `PreparedFrameSlot`, and
    `PreparedStackObject` (`value_locations.hpp:329` and the prepared stack
    layout); no MIR reconstruction is needed or allowed;
  - selected freshness and its producer reference: the unique
    `PreparedValueFreshnessAuthority` selected by the prepared freshness query;
  - edge identity/publication: predecessor/successor labels and the indexed
    `PreparedEdgePublication`; instruction cursor identity is
    `PreparedMirInstructionCursor` (`prepared_view.hpp:45`);
  - aggregate evidence: `PreparedAggregateStackSourceAuthority`; it owns source
    slot/offset/size/alignment, copy width, destination placement, lane/layout,
    and scratch facts, but its producer currently stops at
    `MissingAggregateCopyAuthority` and never produces `Available`;
  - branch evidence: `PreparedBranchStackLoadAuthority`; it owns role,
    block/terminator cursor, value/home, slot, stack object, load policy,
    stack geometry, clobber safety, pointer proof, and selected branch freshness.
- Negative-state ownership is also prepared-side: missing values/homes/moves or
  publication belong to edge-publication/home/move production; missing,
  invalid, or ambiguous freshness belongs to the freshness selector;
  incomplete/mismatched slot-object-layout facts belong to stack-layout and
  aggregate/branch planners; duplicate identity belongs to the authority
  composer. `RouteOnly` remains a new composer status: route records, dumps,
  source order, and MIR-derived facts have no admissible producer mapping.
- Selected the smallest real first positive path: the collected branch
  stack-load `Lhs` row in `check_branch_stack_load_authority_contract`
  (`backend_prepare_stack_layout_test.cpp:7166`).  Production is
  `collect_prepared_branch_stack_load_authorities` ->
  `make_branch_stack_load_authority_record` ->
  `plan_prepared_branch_stack_load_authority`
  (`publication_plans.cpp:4537/4407/4189`); the current cursor-bound MIR
  consumer is `PreparedMirFunctionView::branch_stack_load_authority` ->
  `query_prepared_mir_branch_stack_load_authority`
  (`prepared_view.cpp:459/355`).  The same fixture's exact same-slot
  intervening write rejects only the clobbered LHS as
  `MissingStackClobberSafety` while the RHS stays `Available`, providing nearby
  semantic negative coverage rather than a testcase-shaped exception.
- The branch row already supplies real load/freshness/cursor/slot/object
  evidence.  Step 2 must compose it with producer-owned publication and selected
  move identity; those fields are gaps in the new aggregate view, not permission
  for MIR inference.  The aggregate route was not selected because its current
  producer has no positive `Available` path.

## Suggested Next

- Execute Plan Step 2 as one branch-first producer packet: declare the status-rich
  prepared authority row, compose the existing collected LHS branch row with
  uniquely matching producer-owned publication/move/home identity, and retain
  the same-slot-clobber case as a fail-closed neighbor.

## Watchouts

- Ideas 647 and 655 remain parked until the complete positive row and
  fail-closed proof exist; do not resume their implementation in Step 1.
- Route agreement, dumps, source order, final assembly, and MIR reconstruction
  are not acceptable authority or positive proof.
- Keep target materializer cleanup in ideas 708-710.
- Do not claim the existing aggregate helper as a positive producer: its only
  terminal aggregate state today is `MissingAggregateCopyAuthority`; lane,
  ABI-layout, and scratch ownership are not populated.
- The current `PreparedMirBranchStackLoadAuthorityView` is a useful consumer
  seam but is not the requested destination-authority row: it omits
  destination/source pairing, selected move, publication identity, and complete
  slot/object geometry.  Extending it by MIR-side lookup would violate the
  source idea; the new row must be prepared before MIR reads it.

## Proof

- Read-only inventory packet: no build or tests run, and no log was produced.
- Exact proposed Step 2 build command:
  `cmake --build build --target backend_prepare_stack_layout_test -j`.
- Exact registered narrow proof:
  `ctest --test-dir build -R '^backend_prepare_stack_layout$' --output-on-failure`.
  This target contains the real collected LHS positive, cursor-bound MIR view,
  and nearby exact same-slot-clobber negative contract.
