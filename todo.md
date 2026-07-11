# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Implement short-circuit dependency composition

## Just Finished

- Step 7 bound short-circuit stable-key facts to the validated producer
  dependency closure. The select, add, and leaf roles remain independently
  authoritative through the complete prepared publication origin, while a
  dependency merely visited by traversal can no longer regain authority after
  its producer is found missing, duplicate, or unsupported.
- Strengthened the focused short-circuit probe to query owner authority for
  both original select/add producer roles and the transitive leaf, and to reject
  missing add producers, mismatched publication destinations, and incomplete
  transfers. Prior focused probes and the established routed-operand contract
  remain green.

## Suggested Next

- Execute the supervisor-selected Step 8 unchanged integration and handback
  packet after accepting the Step 7 composition contract.

## Watchouts

- `src/backend/prealloc/publication_plans.cpp` contains an unrelated
  pre-existing dirty candidate hunk around join-source semantic-origin
  preparation; Step 6 preserved it exactly and does not claim it.
- Stable-key duplicate, multiple, and parallel disagreement remains owned by
  the all-applicable-fact query; dependency traversal cannot bypass it because
  only validated closure members now receive composed routing facts.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_(current_block_(direct_root_classification|composed_dependency_authority|memory_source_authority|short_circuit_dependency)_probe|prealloc_current_block_routed_operand_authority)$'
  > test_after.log 2>&1` exactly as delegated. Build passed and all four
  focused probes plus the established routed-operand authority contract passed
  (5/5). Proof log: `test_after.log`.
