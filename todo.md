Status: Active
Source Idea Path: ideas/open/455_dependency_operand_authority_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Authority Population Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 455. Audit artifact:
`build/agent_state/455_step1_authority_population_audit/audit.md`.

Fresh prepared probe:
`build/agent_state/455_step1_authority_population_audit/20010329-1.prepared.out`
with status in
`build/agent_state/455_step1_authority_population_audit/probe_status.txt`.

Bucket table:

| Candidate | Edge identity | Dependency operand | Value home/object linkage | Cast/source identity | Freshness/clobber facts | Current planner/printing | First missing population fact |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `%t17` RHS dependency for `%t18 -> %t22` | `logic.rhs.end.13 -> logic.end.14`, join result `%t22`, incoming `%t18`, predecessor-terminator parallel copy, destination register `t0` | `%t18 = bir.ule ptr %t15, %t17`, operand role RHS, `%t17` value_id 9 type `ptr` | home `stack_slot slot_id=2 offset=16`; storage `frame_slot ... slot#2+stack16`; object #2 `name=%t17 source_kind=regalloc.spill_slot type=ptr size=8 align=8` | `%t17 = bir.inttoptr i32 %t16 to ptr`; `%t16` value_id 8 has rematerializable immediate `-2147483643` | No explicit slot freshness or clobber-safety fact for loading slot #2 at the edge | Fresh dump has no `dependency_operand`/authority record and no prepared printing for policy/status | Explicit population record tying edge, binary source producer, RHS operand `%t17`, cast producer, source `%t16`, and `rematerialize_cast_from_source` policy/status |
| `%t17` stack-load alternative | Same edge and RHS operand | `%t17` as stack-slot pointer dependency | Same stack home/object linkage | Not needed for pure load policy | Missing freshness and clobber-safety | No printed policy/status | Freshness/clobber producer facts before any `load_from_stack_slot` population |
| `%t15` adjacent LHS dependency | Same source producer `%t18` | LHS operand `%t15` | register home `%t15` value_id 7, `s1` | N/A | N/A | Existing register/immediate rematerialization class already handles this shape | No missing idea-455 population fact |
| `%t18` incoming source result | Edge source copied to `%t22` | `%t18` incoming value | register home `%t18` value_id 10, `t0` | `%t18` is successor-block compare result | N/A | Printed as edge incoming and block-entry move, but not copyable at predecessor edge | Remains rejected; do not populate successor-result copy authority |

First coherent next packet: define the population and printing contract for
explicit `rematerialize_cast_from_source` dependency-operand authority.
The representative has enough visible ingredients for a bounded cast policy
candidate, but Step 2 must specify the semantic source-producer relationship
and printer output before code. Stack-load authority is not first because
freshness and clobber-safety facts are absent.

## Suggested Next

Step 2: `Define Population And Printing Contract`.

Define the accepted population record for the cast-rematerialization candidate:
edge identity, source producer identity, dependency operand role/value/type,
cast producer identity, cast source home, supported width policy, destination
publication, explicit status, and prepared printing. Also define rejected
records for missing cast source, unsupported width, stack-load without
freshness/clobber-safety, successor-result copies, and unavailable edge
publication.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 target lowering from metadata type existence alone.
- Do not infer `rematerialize_cast_from_source` from raw `inttoptr`; the
  population packet needs an explicit prepared record.
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

Step 1 classification-only proof:

```sh
git diff --check
```

Result: passed.
