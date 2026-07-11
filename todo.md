# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define the upstream policy-state boundary

## Just Finished

- Partially completed plan Step 2 by registering three valid focused C++ probes
  under `tests/backend/case/`: direct publication identity, production
  transitive dependency closure, and stable-key owner-fact composition.
- The closure probe now calls
  `prepare_current_block_join_parallel_copy_source_facts` over a real producer
  graph. Its positive two-edge traversal and cyclic termination checks pass;
  it exits 3 because a dependency with no producer is still included from its
  value-home record instead of failing closed. A conflicting-producer check is
  also encoded after that first failing assertion.
- The direct probe still exits 3 because rewriting the value while retaining
  publication IDs is accepted. The composition probe still exits 1 because an
  owner-attached stable-key fact cannot represent a routed dependency distinct
  from the preserved source identity.
- The fourth probe is blocked: no upstream production structure or query API
  represents the required policy axis. `PreparedFunctionLookups` contains only
  a routing-fact vector, so absent policy and attached-owner-with-zero-facts are
  the same empty-vector state. Registering a test that varies owner attachment
  would test a target fixture, while using a distinct routed value would merely
  duplicate the composition failure. The invalid substitute probe was removed.
- Kept `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
  and all `src/` implementation files unchanged. Probe registration is route
  correction only and does not claim backend capability progress.

## Suggested Next

- Execute plan Step 3 by adding the narrowest target-independent prepared-state
  representation/query that distinguishes absent policy, absent owner,
  attached owner with zero applicable facts, and authoritative facts without
  granting routing authority. Then execute Step 4 to register the fourth probe
  before implementing zero-policy routing semantics.

## Watchouts

- Keep `backend_aarch64_current_block_join_routing` integration-only. Do not
  change supported vectors, rewrite `%source` identity to `%operand`, treat an
  attached owner as authority, or restore Route 5/target-local reconstruction.
- Step 3 may add only the state boundary needed to make the four states
  observable; it must not implement their routing behavior. Step 4 owns probe
  extraction before semantic implementation resumes.
- Do not make the three registered probes green by weakening them. Their
  failures are extracted capability gaps for later implementation steps.

## Proof

- Exact supervisor-selected command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- Build succeeded. The matching subset ran 323 tests: 320 passed and the three
  registered focused probes failed at their intended initial capability gaps.
  Full output is preserved in `test_after.log`; the failed exact command
  returned exit status 8.
