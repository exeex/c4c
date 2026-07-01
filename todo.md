Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Proof Carrier

# Current Packet

## Just Finished

Completed Step 3 for idea 486 by implementing the independent dynamic
local-array index range proof/path-dominance metadata and status surface.

Implemented surface:

| Surface | Purpose |
| --- | --- |
| `LocalArrayRangeProofStatus` | status vocabulary for available and fail-closed proof outcomes |
| `LocalArrayRangeProofSourceKind` | explicit proof-source kind, currently `branch_condition` or `explicit_compare` |
| `LocalArrayRangeProofPredicate` | normalized comparison predicate roles accepted by the checker |
| `LocalArrayIndexRangeProofInputs` | explicit proof source, bounds, path/dominance, and no-clobber inputs |
| `LocalArrayIndexRangeProofRecord` | checker output keyed to the idea 485 local-array element path |
| `evaluate_local_array_index_range_proof` | planner/checker API for synthetic complete inputs and fail-closed statuses |

Focused coverage:

- synthetic complete proof inputs produce `available`;
- missing local-array path, dynamic index, proof source, lower bound, upper
  bound, path validity, dominance, consumer path coverage, and no-clobber facts
  fail closed;
- unsupported predicate roles, clobbered index values, and raw-shape-only
  evidence fail closed;
- real dynamic local-array rows remain `missing_index_range_proof` unless
  explicit proof inputs are supplied.

Artifact:

- `build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md`

## Suggested Next

Complete Step 4 residual disposition for idea 486: classify the status surface
as implemented, identify the remaining producer-population owner for real proof
source/path/no-clobber facts, and keep idea 484 packaging and scalar local-load
production blocked for dynamic rows until those facts exist.

## Watchouts

- Do not infer index range from loop shape, value names, testcase names, dump
  order, final homes, or RV64 target behavior.
- Do not mark dynamic GEP rows available without proof source,
  path/dominance, and no-clobber facts.
- Do not change idea 485 carrier records, idea 484 packaging, scalar local-load
  consumption, or RV64/MIR lowering in this proof-carrier packet.
- `MemoryDynamicArrayRangeVerdict::BoundedByElementCount` is not enough by
  itself for idea 486; it lacks proof-source, dominance/path, and no-clobber
  authority.
- `PreparedControlFlowFunction::branch_conditions` exposes compare fields but
  does not by itself prove that a branch condition covers a later dynamic GEP
  consumer.
- Synthetic positive coverage is acceptable for the first status surface; real
  dynamic rows remain unavailable until producer data supplies every required
  fact.
- Step 3 did not populate real proof records from control-flow; it only added
  the independent checker/status carrier.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; `test_after.log` contains the build and `^backend_` CTest
output.
