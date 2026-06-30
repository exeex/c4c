Status: Active
Source Idea Path: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Stack-Slot Pointer Dependency Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 453. The contract rationale is under
`build/agent_state/453_step2_stack_slot_pointer_dependency_contract/`.

Accepted shape:

- authoritative `select_materialization` predecessor-edge publication;
- incoming source is a prepared compare producer to rematerialize into the
  destination, not a successor-block compare register to copy;
- non-stack dependency operands have GPR register homes or accepted immediate
  homes;
- the pointer dependency has explicit value-home and stack-object facts with
  coherent slot id, stack offset, size, alignment, pointer width, and target
  bank;
- the producer also proves edge placement, freshness, clobber-safety, and one
  explicit materialization policy: `load_from_stack_slot` or
  `rematerialize_cast_from_source`;
- destination selected value has an available register block-entry publication.

Rejected shapes:

- copying `%t18` or another successor/join-block compare result on a
  predecessor edge;
- raw-BIR inference from `inttoptr`, compare, stack-slot spelling, block order,
  testcase names, or one prepared dump;
- stack-slot homes without explicit freshness/clobber-safety and a load/cast
  policy;
- stack object metadata that is not tied to the dependency value id and edge
  materialization operation;
- address-exposed, stale, ambiguous, non-pointer-compatible, missing-size, or
  missing-alignment slots;
- missing/unavailable edge publication, missing/non-GPR destination, generic
  stack-home branch operand work, pointer-value provenance, or instruction-side
  lowering.

Step 2 conclusion: the first owner is producer/home metadata. Current `%t17`
facts are candidate evidence, but they still lack a durable edge dependency
authority and a load-vs-cast materialization policy.

## Suggested Next

Step 3: `Add Stack-Slot Pointer Dependency Authority Coverage`.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` if a helper test
  is clearer
- `todo.md`
- `test_after.log`
- `build/agent_state/453_step3_stack_slot_pointer_dependency_authority/*`

Add a producer/prepared predicate or contract record for stack-slot pointer
select-edge dependencies. If current producers cannot express
freshness/clobber-safety or an explicit load/cast policy, record that
producer/home metadata blocker instead of editing RV64 lowering.

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

Step 2 contract proof:

```sh
git diff --check
```

Result: passed.
