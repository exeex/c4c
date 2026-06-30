Status: Active
Source Idea Path: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Stack-Slot Dependency Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 453 as a routed blocker. No implementation or tests
were changed. The blocker artifact is
`build/agent_state/453_step3_stack_slot_pointer_dependency_authority/blocker.md`.

Inspected prepared authority surfaces:

- `PreparedEdgePublication` names the edge source `%t18`, its producer, homes,
  move bundle, and source-memory metadata.
- `PreparedEdgePublicationSourceProducer` can point to the `%t18 = ule`
  `BinaryInst` and other producer instruction kinds.
- `PreparedTypedStackSourcePublication` only covers direct same-width `i32`
  stack sources where the stack value is the edge source.
- `PreparedAggregateStackSourceAuthority` covers aggregate-width stack sources
  and remains intentionally fail-closed without copy authority.
- `PreparedFusedPointerBranchPublication` rejects stack-home pointer operands.

Exact blocker:

- The `%t18 -> %t22` edge source is `%t18`; the stack-slot value is a dependency
  operand `%t17`, not the edge source.
- Current prepared facts expose `%t17`'s stack-slot home and object metadata,
  and `%t16`'s immediate source, but they do not carry a dependency-operand
  materialization policy.
- There is no prepared fact saying to load `%t17` from `slot_id=2 offset=16`,
  no fact saying to rematerialize `%t17` from `%t16` through `inttoptr`, and no
  freshness/clobber-safety fact for using the slot at the predecessor-edge move
  site.

Missing owner: producer/home metadata. A predicate added over current facts
would only prove slot identity and size/alignment, not load-vs-cast policy,
freshness, clobber safety, or edge availability, so RV64 lowering must remain
fail-closed.

## Suggested Next

Step 4: `Residual Disposition And Close Readiness`.

Record that idea 453 is blocked before RV64 consumption on a producer/home
metadata surface for dependency-operand materialization. A future executable
packet should add:

- edge dependency-operand authority, not just edge source authority;
- an explicit `load_from_stack_slot` vs `rematerialize_cast_from_source`
  policy;
- slot freshness/clobber-safety for the predecessor-edge placement;
- value-home/object linkage for the dependency operand `%t17`.

If that scope is broader than idea 453, route a lifecycle split for the
producer/home metadata surface before returning to stack-slot pointer
select-edge materialization.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not copy `%t18` from its successor/join-block register home on the
  predecessor edge.
- Do not load or rematerialize `%t17` from a stack slot without explicit
  prepared authority for slot identity, width, freshness, clobber safety, edge
  placement, and target compatibility.
- Do not infer dependency materialization authority from raw `inttoptr`,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump layout.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 routed-blocker proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
