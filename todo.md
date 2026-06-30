Status: Active
Source Idea Path: ideas/open/464_select_carrier_alias_metadata.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Carrier-Alias Authority Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 464. A bounded
producer/prepared metadata packet is justified: publish durable carrier-alias
authority records for select-materialization join transfers, with use-closure
proof for the selected source producer.

Contract table:

| Field | Value |
| --- | --- |
| Accepted fact | One carrier-alias authority record per carrier value, keyed by function, predecessor/successor edge, join transfer, final destination value, selected source value, source-producer site, and carrier value/instruction |
| Use closure | All uses of the selected source-producer result must be the selected edge source or authorized carrier aliases for the same final join-transfer result |
| Positive case | Duplicate carrier selects such as `%t50.phi.sel0` / `%t50.phi.sel1` alias final `%t50` for the same `%t46 -> %t50` select-materialization edge |
| Negative cases | Wrong final result, wrong source value, wrong edge, raw-name-only `%*.phi.sel*` shape, extra non-carrier use, missing/ambiguous edge publication, missing/ambiguous source producer |
| Target files | `src/backend/prealloc/control_flow.hpp`, `src/backend/prealloc/prepared_object_traversal.*`, `src/backend/prealloc/select_chain_lookups.*`, `src/backend/prealloc/publication_plans.*`, focused `tests/backend/bir/` coverage |
| Consumer boundary | No RV64 ULE rematerialization or target consumer change in idea 464; later RV64 work may consume this fact only after it exists |
| Step 3 proof command | `{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check` |

Artifact:
`build/agent_state/464_step2_carrier_alias_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: implement or route the carrier-alias metadata
packet in the prepared control-flow/publication layer with focused positive
and fail-closed tests. Do not touch RV64 target lowering.

Suggested artifact directory:
`build/agent_state/464_step3_carrier_alias_metadata/`.

## Watchouts

- Do not make RV64 ULE rematerialization or target consumer changes before the
  metadata exists.
- Do not infer duplicate-carrier authority from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Preserve fail-closed behavior for extra non-carrier uses and mismatched
  source/destination/edge facts.
- Do not implement a plain `%t46 -> %t50` copy or same-register no-op.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
