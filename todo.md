Status: Active
Source Idea Path: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define ULE Rematerialization Consumer Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 465. A bounded RV64 consumer
packet is justified: collect and consume explicit
`PreparedSelectCarrierAliasAuthorityRecords` to authorize ULE rematerialization
for the coordinate-backed `%t46 -> %t50` predecessor-edge move, while keeping
raw alias inference, plain successor-defined copies, stale stack loads, and
generic move support rejected.

Contract table:

| Contract field | Required condition |
| --- | --- |
| Authority source | RV64 must consume `PreparedSelectCarrierAliasAuthorityRecords` collected from the prepared module; no raw `%*.phi.sel*` inference is accepted. |
| Event match | The move-bundle event must be the coordinate-backed `pre_terminator_copies` / `out_of_ssa_parallel_copy` route for `logic.rhs.end.40 -> logic.end.41` or the semantic equivalent: predecessor/successor labels match the authority record and edge publication. |
| Publication match | Edge publication intent must be available, `carrier_kind=select_materialization`, `source_producer_kind=binary`, `source_value_id` matches move source, and destination value/id matches move target and join-transfer result. |
| Source producer | Source producer must be an unsigned pointer compare rematerializable by the selected packet: first implementation target is `Ule` with i32 result and pointer operands. Other compare opcodes remain out of first packet unless explicitly covered. |
| Carrier aliases | An available carrier-alias authority record must match function, predecessor/successor edge, source value/id, destination value/id, source-producer block/instruction, and binary source-producer kind. |
| Operand consumability | Both operands must be target-consumable at the predecessor edge through existing register/immediate/cast-authority routes. Representative `%t42` is `s1`; `%t45` is `s2`. |
| Emission | Rematerialize the binary compare into the destination register at the coordinate-backed predecessor-edge event; do not copy the successor-defined `%t46`. |
| Fail-closed cases | Missing/wrong alias authority, wrong edge, wrong source producer, mismatched carrier/final result, non-consumable operands, stale stack-load authority, raw alias inference, plain copy/no-op, and generic move-bundle cases remain rejected. |

Target files/tests for Step 3:

| File | Role |
| --- | --- |
| `src/backend/mir/riscv/codegen/object_emission.cpp` | Collect/pass carrier-alias records and consume them in the select-edge source-producer path. |
| `src/backend/prealloc/publication_plans.hpp` / `.cpp` | Only if a small record-matching helper is needed; do not change producer semantics. |
| `tests/backend/mir/backend_riscv_object_emission_test.cpp` | Focused accepted/fail-closed RV64 consumer coverage. |
| `todo.md`, `test_after.log`, `build/agent_state/465_step3_ule_rematerialization_consumer/` | Step 3 execution state, proof, and artifacts. |

Artifact:
`build/agent_state/465_step2_ule_rematerialization_consumer_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: implement or route the first RV64 consumer
packet. The packet should wire explicit
`PreparedSelectCarrierAliasAuthorityRecords` into the select-edge
source-producer route and rematerialize only the authorized ULE binary compare
when both operands are target-consumable.

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not broaden Step 3 into generic move-bundle lowering, plain
  successor-defined copies, or same-register no-ops.
- Do not infer duplicate-carrier aliases from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Preserve record-based fail-closed behavior for missing/wrong alias authority,
  wrong edge, wrong source producer, mismatched carrier/final result, and
  non-consumable operands.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
