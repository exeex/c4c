# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement composed dependency preservation

## Just Finished

- Step 5 proved that the existing generic stable-key owner query already
  implements composed dependency preservation, so no code or test change was
  manufactured. A dependency is authoritative only under its complete
  `(routed_value_id, routed_value_name, IncomingExpression)` composition key;
  unrelated ids, names, or roles are not applicable.
- The query evaluates every applicable fact and fails closed for negative or
  incomplete facts, invalid destination/source fields, unknown semantic
  origin, duplicate semantic edges, and multiple/parallel
  destination/source/origin disagreement. The focused positive preserves the
  composed dependency independently while direct-root incoming-expression
  rejection remains green.

## Suggested Next

- Execute the next supervisor-selected packet from the active plan after
  accepting the Step 5 proof-only result.

## Watchouts

- Step 5 was already present at the owner boundary. Unrelated pre-existing
  dirty hunks in owned files were inspected and preserved exactly.
- The routed dependency key is independent of the preserved direct
  publication destination/source/origin; membership, names, numeric fixture
  identities, or storage alone grant no authority.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_current_block_(direct_root_classification|composed_dependency_authority)_probe$'
  > test_after.log 2>&1` exactly as delegated. Build passed and both focused
  probes passed (2/2). Proof log: `test_after.log`.
