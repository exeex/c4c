# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Compose the focused authority contracts

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

- Execute Step 6 owner-only AArch64 consumption using the proven
  owner-attached stable-key query, preserving all supported integration vectors
  unchanged.

## Watchouts

- Keep Step 6 consumption owner-only: do not reconstruct authority from a
  successor body, a target-local subset, or rewritten source identity.
- The immediate routed-operand case deliberately authorizes the prepared
  destination value while retaining the prepared source object as its identity;
  that explicit distinction is not a general source-rewrite permission.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 320/320 backend tests passed, including all three focused authority
  contracts, incoming-expression authority, prepared-owner lookup-attachment
  lifetime, and unchanged AArch64 current-block routing integration.
- Canonical proof log: `test_after.log`.
