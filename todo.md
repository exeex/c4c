Status: Active
Source Idea Path: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First RV64 Consumer Packet

# Current Packet

## Just Finished

Completed Step 3 implementation for idea 465. RV64 object emission now
collects and consumes explicit `PreparedSelectCarrierAliasAuthorityRecords` for
the select-edge source-producer route, and it rematerializes only authorized
unsigned pointer `Ule` compares into the destination register. No raw
`.phi.sel` alias inference, plain successor-defined copy/no-op, stale stack
load, or generic move support was added.

Implementation table:

| Field | Result |
| --- | --- |
| Authority collection | `prepared_function_to_object_function` now collects `collect_prepared_select_carrier_alias_authorities(prepared)`. |
| Move-bundle consumer | `fragment_for_prepared_move_bundle` passes carrier-alias records to `fragment_for_prepared_select_edge_source_producer`. |
| Record matching | The selected-source route matches available records by function, predecessor/successor edge, source/destination ids and names, binary source-producer kind, and source-producer block/instruction. |
| Accepted rematerialization | Duplicate-carrier ULE source producers are accepted only with explicit carrier-alias authority and target-consumable operands. |
| Instruction-side handling | Authorized carrier alias select instructions and the owned source producer are skipped through record-backed authority instead of raw select shape. |
| Fail-closed coverage | Tests reject missing alias authority, non-`Ule` source producer, and missing/non-consumable operand homes. |
| Files changed | `src/backend/mir/riscv/codegen/object_emission.cpp`, `tests/backend/mir/backend_riscv_object_emission_test.cpp`. |

Artifact:
`build/agent_state/465_step3_ule_rematerialization_consumer/summary.md`.

## Suggested Next

Execute Step 4 from `plan.md`: residual disposition and close readiness.
Re-probe or inspect the representative route, classify whether `20010329-1`
advanced past the carrier-alias ULE rematerialization consumer, and route any
next first owner without broadening idea 465.

Suggested artifact directory:
`build/agent_state/465_step4_residual_disposition/`.

## Watchouts

- Do not broaden any follow-up into generic move-bundle lowering, plain
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

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
