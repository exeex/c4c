Status: Active
Source Idea Path: ideas/open/455_dependency_operand_authority_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Population And Printing Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 455. Contract artifact:
`build/agent_state/455_step2_population_printing_contract/contract.md`.

Accepted population shape for the first packet:

- source edge is a prepared select-materialization edge transfer with
  predecessor-terminator placement and available register destination
  publication;
- source producer is a prepared binary compare for the incoming edge source;
- dependency operand role, value id/name/type, and producer operand identity
  match exactly;
- dependency operand is a pointer value with an explicit prepared
  `IntToPtr` cast producer;
- cast result value id/name/type matches the dependency operand;
- cast source value has a register or rematerializable immediate GPR home;
- pointer width and integer source width are explicit and supported by the
  dependency-operand authority planner;
- populated record prints policy `rematerialize_cast_from_source`, status,
  edge identity, producer identity, operand role, cast result, cast source, and
  source home.

Rejected population shapes:

| Shape | Reason |
| --- | --- |
| Raw `inttoptr` plus immediate without a populated dependency record | Raw BIR is not authority |
| Stack home/object metadata alone | Missing explicit policy |
| `load_from_stack_slot` without freshness and clobber-safety | Unsound slot load at predecessor edge |
| Successor/join-block `%t18` result copy | Source is not available at predecessor edge |
| Missing or mismatched edge/source-producer/operand/cast identity | Cannot bind authority to the selected dependency operand |
| Unsupported cast kind or width, or non-consumable cast source home | Outside first population packet |
| Unavailable or non-register destination publication | No valid edge placement target |

Printer evidence requirement: prepared output must expose a dependency-operand
authority section or equivalent line for the populated record, including exact
fail-closed status for rejected candidates.

## Suggested Next

Step 3: `Implement Or Route First Population Packet`.

Implement the narrow producer/prepared population and printer exposure for
explicit `rematerialize_cast_from_source` dependency operands. The focused
representative is `%t17` on the `logic.rhs.end.13 -> logic.end.14` edge, but
the rule must be semantic: consume prepared edge/source-producer, operand,
cast/source, home, and destination-publication facts rather than testcase,
block-name, value-name, or raw dump shape matching.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- prepared printer file if the authority record needs new dump output
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp` if printer coverage is clearer
- `todo.md`
- `test_after.log`
- `build/agent_state/455_step3_dependency_operand_population/*`

Proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 target lowering from metadata type existence alone.
- Do not infer `rematerialize_cast_from_source` from raw `inttoptr`; the
  implementation must populate an explicit prepared record.
- Do not infer `load_from_stack_slot` from stack home/object metadata without
  freshness and clobber-safety.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 contract-only proof:

```sh
git diff --check
```

Result: passed.
