# Current Packet

Status: Active
Source Idea Path: ideas/open/717_current_block_routed_value_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Prepare and attach owner facts on every supported entry path

## Just Finished

- Step 6.1 inventoried the production traversal, join-routing fixture, dispatch
  fixture, and explicit policy-attachment fixture entry paths. Production
  already attached the shared prealloc `PreparedFunctionLookups` owner; the two
  general fixture helpers now retain an equivalent shared owner instead of
  replacing its views with stack-local raw lookup pointers.
- The unchanged join-routing vectors now assert that every supported source
  positive carries an attached owner with upstream-produced routing facts.
  The explicit detached-owner and no-policy negatives remain fail closed in
  the fixture policy attachment contract. No AArch64 authority builder,
  testcase-shaped fact, or expected-vector change was added.

## Suggested Next

- Execute Step 6.2 by removing AArch64 current-block authority reconstruction
  and consuming only the owner-attached authoritative routing facts now proven
  on every supported positive entry path.

## Watchouts

- Owner attachment is now proven for the supported fixture paths, but Step 6.2
  must still preserve the existing Route 5 diagnostic/non-authority boundary
  and all supported vectors while deleting reconstruction.
- Do not reconstruct authority from a successor body, a target-local subset,
  Route 5, or rewritten source identity. Fixture setup may expose the production
  owner boundary but must not inject named-case facts or change policy meaning.
- The immediate routed-operand case deliberately authorizes the prepared
  destination value while retaining the prepared source object as its identity;
  that explicit distinction is not a general source-rewrite permission.
- The fixture helper owner copies deliberately preserve the complete upstream
  lookup family; do not replace them with a current-block-only fact injection.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: 320/320 backend tests passed, including the unchanged AArch64
  current-block routing vectors, fixture policy attachment negatives, dispatch
  coverage, all focused authority contracts, and lookup-attachment lifetime.
- Canonical proof log: `test_after.log`.
