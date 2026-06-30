Status: Active
Source Idea Path: ideas/open/467_unsupported_carrier_alias_planner_rejection.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Carrier Alias Acceptance Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 467. The acceptance contract
allows synthesized carrier alias candidates only when identity is derived from
producer-owned join-transfer/source-producer facts, not raw `.phi.sel`
spelling.

Contract table:

| Area | Required condition |
| --- | --- | --- |
| Publication/join | Available edge publication, `select_materialization` carrier, binary source producer, matching join transfer, and final select for the join result. |
| Candidate shape | Candidate select is in the successor join block, named `i32`, distinct from destination, payload-uses selected source, does not condition-use it, and feeds the final select. |
| Alias identity | Candidate result has or receives durable `ValueNameId` through the producer-owned planner path after semantic candidate discovery. |
| Alias value id/home | Optional; preserve when present, but absence is not a rejection because carrier aliases are metadata carriers, not the moved edge value. |
| Source closure | Every selected-source use must be the producer result or an authorized carrier-alias payload use; extra binary/cast/phi/call/store/terminator uses remain rejected. |
| Authority publication | Only `status=available` rows with non-empty aliases and `source_use_closure_proven=yes` enter the RV64-facing available-authority collector. |

Fail-closed cases: unpublished/unpublishable alias identity, empty/duplicate
candidates, candidate equals destination, wrong block, wrong final result,
wrong source/edge, source used as condition, source not payload-used, extra
source uses, raw-name-only fixtures, and any attempt to consume unavailable
evidence rows as authority.

Artifact:
`build/agent_state/467_step2_carrier_alias_acceptance_contract/contract.md`.

## Suggested Next

Execute Step 3: implement or route a narrow prepared naming/planner repair.
Target files are `publication_plans.hpp/.cpp`,
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`, `todo.md`,
`test_after.log`, and
`build/agent_state/467_step3_carrier_alias_acceptance/*`; prepared-printer
files/tests are optional only if evidence assertions need them.

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not treat candidate discovery as authority; candidates still require
  prepared identity and closure proof.
- Intern/publish candidate names only through the producer-owned
  carrier-alias planner path after semantic candidate checks pass; do not
  intern or accept names merely because they match `.phi.sel` spelling.
- Alias `ValueNameId` is required; alias `PreparedValueId`/home is optional.
- Do not treat unavailable evidence rows as authority.
- Do not make RV64 ULE rematerialization changes until an available authority
  record exists or a later packet proves an RV64 matcher mismatch against one.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
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
