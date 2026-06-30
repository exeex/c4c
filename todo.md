Status: Active
Source Idea Path: ideas/open/468_carrier_alias_identity_publication_api.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Shared Identity Publication Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 468.

Selected boundary: a mutable pre-consumer prepared publication stage, e.g.
`populate_select_carrier_alias_identity(PreparedBirModule&)`, called from
`BirPreAlloc::publish_contract_plans()` after control-flow, value-location,
and edge-publication inputs exist and before prepared dump/RV64 const
consumers observe the module.

Contract table:

| Area | Contract |
| --- | --- |
| Timing | Runs in the mutable prealloc contract-plan phase before backend handoff to const consumers. |
| Publication | Interns synthesized carrier-alias result names into canonical `prepared.names.value_names`. |
| Required facts | Available edge publication, `select_materialization`, binary source producer, matching join transfer, final select, successor-block candidate, named `i32` result, candidate distinct from destination, payload use of selected source, no condition use of selected source, and candidate feeds final select. |
| Value id/home | Optional; identity publication does not require alias `PreparedValueId` / home. |
| Authority boundary | Identity publication is not RV64 permission; available authority remains decided by later carrier-alias authority collection. |
| Source-use closure | Kept in the later authority collector/planner, not the identity publication stage. Extra source-use rows may get identity but must remain unavailable authority records. |

Rejected identity-publication shapes: missing publication/join/final carrier,
unsupported carrier/source-producer kind, missing binary source producer,
source-producer result mismatch, unnamed candidate, candidate equals
destination, wrong successor block, no payload use, source used as condition,
candidate not feeding final select, duplicate candidates, raw `.phi.sel`
spelling inference, testcase-name inference, dump-order inference, value-id
coincidence, and scratch-copy-only publication.

Artifact:
`build/agent_state/468_step2_shared_identity_publication_contract/contract.md`.

## Suggested Next

Execute Step 3: implement or route the shared identity API packet.

Target implementation/tests:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prealloc.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/468_step3_shared_identity_publication/*`

Optional only if evidence/printing assertions require them:

- `src/backend/prealloc/prepared_printer/select_chains.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not reintroduce `const_cast` or hidden mutation.
- Do not claim scratch-copy-only publication is sufficient for original-module
  consumers.
- Do not make RV64 lowering changes in this identity/API idea.
- Identity publication alone is not authority; RV64 must consume only
  available carrier-alias authority records.
- Preserve fail-closed handling for raw alias inference, unavailable evidence
  rows, wrong edge/source/final result, stale stack-loads, and generic move
  cases.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
