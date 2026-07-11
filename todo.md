# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Prepare and attach owner facts on every supported entry path

## Just Finished

- Step 5 composed the join-transfer destination-consistency, routed-operand
  publication-authority, and all-applicable-edge invariance contracts at the
  owner-attached query boundary; the existing lookup-attachment lifetime
  contract ran in the same backend proof.
- AST-backed inspection confirmed that the owner overload delegates directly
  to its owned routing-fact family, the complete family must agree before it
  becomes `Available`, and routed-operand authority preserves the prepared
  publication source identity. No testcase-shaped selection, hidden
  source-identity rewrite, implementation change, or expectation change was
  required.

## Suggested Next

- Execute Step 6.1 by inventorying the three supported owner-absent or
  owner-empty entry paths exposed by the rejected consumption attempt, then
  route them through the shared upstream preparation and attachment boundary
  while preserving their expected routing vectors unchanged.

## Watchouts

- Do not remove AArch64 reconstruction until every supported positive has been
  proven to reach consumption with an attached prepared owner and authoritative
  facts. A missing owner or absent fact must remain an explicit fail-closed
  negative, not a supported positive silently downgraded.
- Do not reconstruct authority from a successor body, a target-local subset,
  Route 5, or rewritten source identity. Fixture setup may expose the production
  owner boundary but must not inject named-case facts or change policy meaning.
- The immediate routed-operand case deliberately authorizes the prepared
  destination value while retaining the prepared source object as its identity;
  that explicit distinction is not a general source-rewrite permission.
- The rejected Step 6 attempt passed 317/320 backend tests and showed that
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_current_block_fixture_policy_attachment` still contain
  supported paths without the required owner facts. `test_after.log` retains
  that diagnostic failure; it is not acceptance proof.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 320/320 backend tests passed, including all three focused authority
  contracts, incoming-expression authority, prepared-owner lookup-attachment
  lifetime, and unchanged AArch64 current-block routing integration.
- Canonical proof log: `test_after.log`.
