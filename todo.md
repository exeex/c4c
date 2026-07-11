# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract four focused classification probes

## Just Finished

- Step 2 added and registered four single-seam authority probes: direct-root
  role classification, composed-dependency stable-key authority, memory-backed
  source/home consistency, and short-circuit add/select dependency closure.
- Each probe states a positive contract and at least one fail-closed negative;
  all four pass independently without modifying the pre-existing dirty
  implementation or integration-test candidates.

## Suggested Next

- Execute Step 3 by mapping the four green probe contracts to their exact
  prepared owner/query inputs, outputs, and agreement invariants.

## Watchouts

- The focused probes deliberately use semantic roles and stable keys rather
  than integration fixture values 810/811.
- The short-circuit probe preserves distinct `%rhs.add` and `%short.selected`
  producers; its missing-add negative rejects both the orphaned add value and
  its transitive leaf.
- The delegated backend proof remains red only in the two pre-existing dirty
  integration contracts: `backend_aarch64_instruction_dispatch` and
  `backend_aarch64_current_block_join_routing`.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
  Build passed; all four new probes passed; 326/328 backend tests passed. The
  only failures remain `backend_aarch64_instruction_dispatch` and
  `backend_aarch64_current_block_join_routing`. Proof log: `test_after.log`.
