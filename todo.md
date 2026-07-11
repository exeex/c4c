# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Implement memory-backed source/home authority

## Just Finished

- Step 6 bound memory-backed routed operands to a complete prepared source/home
  contract at the generic owner query. Memory-backed stack authority now
  requires a concrete prepared producer origin whose result is the published
  source, and stack homes require complete nonzero slot layout in addition to
  matching source id, name, and home kind. Register-backed named authority
  retains its established producer-optional contract, while any producer that
  is present must still agree with the published source.
- Extended the focused memory probe to reject attachment-only, home-only,
  missing-origin, incomplete-stack-home, and mismatched-home cases. The prior
  direct-root and composed-dependency probes remain green; their stable-key
  owner boundary continues to fail closed on duplicate, multiple, and parallel
  applicable-fact disagreement.

## Suggested Next

- Execute the supervisor-selected Step 7 short-circuit dependency-composition
  packet after accepting the Step 6 source/home contract.

## Watchouts

- `src/backend/prealloc/publication_plans.cpp` contains an unrelated
  pre-existing dirty candidate hunk around join-source semantic-origin
  preparation; Step 6 preserved it exactly and does not claim it.
- The stack home remains supporting evidence only: attachment or storage
  identity without the matching prepared source and producer origin grants no
  routed-operand authority.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_(current_block_(direct_root_classification|composed_dependency_authority|memory_source_authority)_probe|prealloc_current_block_routed_operand_authority)$'
  > test_after.log 2>&1` exactly as delegated. Build passed and all three
  focused probes plus the established routed-operand authority contract passed
  (4/4). Proof log: `test_after.log`.
