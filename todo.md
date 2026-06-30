Status: Active
Source Idea Path: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 453. The disposition artifact is
`build/agent_state/453_step4_residual_disposition/disposition.md`.

Residual classification:

| Residual | Classification | First owner |
| --- | --- | --- |
| `%t18` successor/join-block compare result on `logic.rhs.end.13 -> logic.end.14` | Must not be copied on the predecessor edge. | Already fail-closed by the select-edge rematerialization contract. |
| `%t17` stack-slot pointer dependency | Stack home `slot_id=2 offset=16` and object metadata exist, but are candidate evidence only. | Dependency-operand authority producer. |
| Load `%t17` from stack slot | Not authorized by current prepared facts. | Needs explicit `load_from_stack_slot` policy plus freshness/clobber-safety. |
| Rematerialize `%t17` from `%t16` through `inttoptr` | Not authorized by current prepared facts. | Needs explicit `rematerialize_cast_from_source` policy and cast dependency record. |
| RV64 consumer route | Not selectable in this plan state. | Must wait for explicit prepared dependency-operand authority. |

Lifecycle recommendation: close or retire this runbook by split. Idea 453
classified the evidence and established the exact missing authority, but no
sound in-idea implementation packet remains before adding producer/home
metadata for dependency-operand materialization.

## Suggested Next

Plan-owner should close/split idea 453 into a producer/home metadata initiative
before any RV64 consumer work. The follow-up should add:

- edge dependency-operand authority, not just edge source authority;
- an explicit `load_from_stack_slot` vs `rematerialize_cast_from_source`
  policy;
- slot freshness/clobber-safety for the predecessor-edge placement;
- value-home/object linkage for the dependency operand `%t17`;
- focused positive/fail-closed producer/prepared tests.

After that authority exists, a later plan can decide whether to return to RV64
stack-slot pointer select-edge materialization. Do not route directly to
target lowering from current facts.

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

Step 4 residual-disposition proof:

```sh
git diff --check
```

Result: passed.
