# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.28
Current Step Title: Publish direct scalar floating call-result authority

## Just Finished

- Completed Plan Step 7.28: the existing direct-call producer now allocates a
  fresh `LirValueId` for exactly a direct, fixed, nonvariadic, zero-argument
  `double` result call, and its ordinary `double` FAdd use preserves that ID.
- Added native-only verification of the module-owned direct callee ID, exact
  empty signature, matching double return refs, result authority, and floating
  use shape; display-only mutations remain compatible.

## Suggested Next

- Supervisor: select the next unclaimed bounded Step-7 producer/use route;
  do not expand this completed direct-double-call slice.

## Watchouts

- The completed route is intentionally limited to direct zero-argument fixed
  nonvariadic `double`; direct integer calls and other floating call forms stay
  unchanged. Native IDs and type refs, never rendered spelling, govern it.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
- Passed `git diff --check`; canonical focused-proof log: `test_after.log`.
