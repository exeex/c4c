Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Repair Return ABI And Select-Publication Authority

# Current Packet

## Just Finished

Step 5, "Repair Return ABI And Select-Publication Authority," completed as a
focused proof-and-reroute packet. Initial `git status --short` was clean.
The three target rows were inspected against Step 1 artifacts, current case
logs, prepared dumps, prior Step 2-4 changes, return ABI move publication,
select-publication intent construction, `prepared_lookups.cpp`,
`publication_plans.*`, `prepared_object_traversal.*`, `regalloc.cpp`, and
`regalloc/value_homes.cpp`.

No implementation file changed. The return rows already publish explicit ABI
register destinations on their moves, including
`destination_kind=function_return_abi`, `destination_storage=register`,
`placement=gpr:call_result#0/w1`, and `reg=a0`; the remaining failure is RV64
pointer stack-source return lowering, not missing prepared destination-home
authority. The select row already has concrete source and destination stack
homes for the select publication (`%t10` slot `#3` offset `2` size `2`,
`%t11` slot `#4` offset `4` size `2`); the remaining failure is RV64 I16
select stack-publication policy, not missing prepared source-home authority.

Changed files:

- `docs/rv64_gcc_torture_post_contract/prepared_authority_return_select_step5.md`
- `todo.md`

Derived artifacts:

- `build/agent_state/552_step5_return_select_authority.allowlist`
- `build/agent_state/552_step5_return_select_authority/row_status.tsv`

Focused Step 5 row counts:

| Classification | Rows |
| --- | ---: |
| Rerouted to RV64 return pointer stack-source lowering | 2 |
| Rerouted to RV64 I16 select stack-publication lowering | 1 |
| Still prepared return ABI/select source-home authority gap | 0 |

Rows rerouted: `src/20001130-2.c`, `src/20080719-1.c`, and `src/pr58726.c`.

## Suggested Next

Executor should run Step 6, "Reconcile The Prepared Authority Queue," and
recompute the 43-row queue after Steps 2-5 to decide whether any prepared
authority rows remain or whether the residuals have moved to later RV64 or
semantic-producer owners.

## Watchouts

- The focused Step 5 allowlist still fails `0/3` by pass count, but all three
  rows now have explicit prepared evidence and should not be treated as missing
  prepared return ABI/select source-home authority.
- Do not repair the two return rows by inventing a second prepared destination
  home for the same value; the ABI register destination is already the move
  destination authority.
- Do not repair `src/pr58726.c` by widening prepared source-home inference; it
  already has concrete 2-byte stack source/destination homes.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step5_return_select_authority.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 5 proof passed `0/3`, with `0` rows still blocked by prepared
  return ABI/select-publication source-home authority and all three rows
  rerouted with row-level evidence.

Proof output is preserved in `test_after.log`.
