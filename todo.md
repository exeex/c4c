# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement direct-root classification

## Just Finished

- Step 4 proved that the existing generic stable-key owner query already
  implements the required direct-root classification, so no code or test
  change was manufactured. `query_prepared_current_block_join_routing_consumption`
  selects facts by the complete `(routed_value_id, routed_value_name, role)`
  key: a direct root prepared as `Source` is available under that role and
  mismatched as `IncomingExpression`.
- The same query independently preserves a composed dependency prepared under
  its own `IncomingExpression` id/name. Its all-applicable-fact checks continue
  to reject negative or incomplete facts, invalid destination/source fields,
  unknown semantic origin, duplicate semantic edges, and parallel
  destination/source/origin disagreement.

## Suggested Next

- Execute Step 5 by proving and, only if missing, implementing composed
  dependency preservation from complete prepared composition while keeping
  direct-root rejection green.

## Watchouts

- Step 4 was already present at the owner boundary; the unrelated pre-existing
  dirty hunk later in `publication_plans.cpp` was inspected and preserved
  exactly.
- Step 5 must retain the direct publication's destination/source/origin in
  composed facts while changing only the routed dependency key; membership,
  names, numeric fixture identities, or storage alone grant no authority.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_current_block_(direct_root_classification|composed_dependency_authority)_probe$'
  > test_after.log 2>&1` exactly as delegated. Build passed and both focused
  probes passed (2/2). Proof log: `test_after.log`.
