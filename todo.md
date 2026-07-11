# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define the upstream policy-state boundary

## Just Finished

- Completed plan Step 3 by adding a target-independent prepared routing-policy
  boundary. Production lookup construction now records policy presence from the
  prepared BIR function, owner attachment from prepared value locations, and
  owner facts independently.
- Added a query that observes `AbsentPolicy`, `AbsentOwner`,
  `AttachedOwnerWithoutApplicableFacts`, and `AuthoritativeFacts`. It delegates
  fact validation to the existing all-edge consumption query and does not
  authorize or substitute an incoming expression operand.
- Preserved the existing routing-fact vector and all supported expectations;
  this packet changes prepared-state observability only.

## Suggested Next

- Execute plan Step 4 by registering the fourth focused policy-state probe
  against the new production boundary before implementing zero-policy routing
  semantics.

## Watchouts

- Keep `backend_aarch64_current_block_join_routing` integration-only. Do not
  change supported vectors, rewrite `%source` identity to `%operand`, treat an
  attached owner as authority, or restore Route 5/target-local reconstruction.
- The policy-state query is observational: even `AuthoritativeFacts` is a
  validated fact-consumption state, not permission to replace an incoming
  expression. Step 4 owns probe extraction before semantic implementation.
- Do not make the three registered probes green by weakening them. Their
  failures are extracted capability gaps for later implementation steps.

## Proof

- Exact supervisor-selected command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- Build succeeded. The matching subset ran 323 tests: 320 passed and the same
  three registered Step 2 focused probes failed at their intended initial
  capability gaps. No supported expectation changed. Full output is preserved
  in `test_after.log`; the exact command returned exit status 8.
