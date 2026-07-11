# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract and register the no-policy/zero-fact focused probe

## Just Finished

- Completed plan Step 4 by registering
  `backend_current_block_no_policy_zero_fact_probe` against the Step 3
  production policy query.
- The focused probe distinguishes `AbsentPolicy`, `AbsentOwner`,
  `AttachedOwnerWithoutApplicableFacts`, and `AuthoritativeFacts`, including
  fail-closed consumption for the first three states and validated consumption
  for an applicable authoritative fact.
- Added no routing semantics and preserved all existing supported expectations.

## Suggested Next

- Execute plan Step 5 by binding each of the four focused probes to one owned
  upstream backend capability and a precise semantic rule.

## Watchouts

- Keep `backend_aarch64_current_block_join_routing` integration-only and keep
  the new policy-state probe observational. `AuthoritativeFacts` records
  validated consumption; it does not authorize incoming-expression rewriting.
- The direct identity, transitive closure, and stable-key composition probes
  remain red at their intended capability gaps. Do not weaken their contracts
  during Step 5 ownership analysis.

## Proof

- Exact supervisor-selected command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- Build succeeded. The matching subset ran 324 tests: 321 passed, including the
  new no-policy/zero-fact probe, and the same three Step 2 focused probes failed
  at their intended capability gaps. No supported expectation changed. Full
  output is preserved in `test_after.log`; the exact command returned exit
  status 8.
