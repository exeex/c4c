# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace the Route 4 common adapter and adapt AArch64 compatibility

## Just Finished

- Step 3.2 compile-consumer follow-up migrated the remaining block-entry
  identity sections in the frame-stack and prepared-lookup helper tests to
  consume `PreparedCurrentBlockEntryPublication` directly.
- No `BirBlockEntryPublicationIdentityRequest` or
  `BirBlockEntryPublicationStatus` consumer remains in either owned test.
- The frame-stack target compiles, and the helper target's direct compile now
  reports only unrelated pre-existing errors outside the owned section.

## Suggested Next

- Resolve or explicitly waive the pre-existing frame-stack runtime failure
  before accepting the Step 3.2 compile-consumer follow-up.

## Watchouts

- `backend_prepare_frame_stack_call_contract_test` fails before reaching the
  edited block-entry section at line 4603 with `call_plans no longer publish
  the direct call`; this is outside the owned block-entry consumer section.
- `backend_prepared_lookup_helper_test` remains blocked by pre-existing Route
  5, Route 1, and `string_view::clear` compile errors at lines 5334, 8821,
  8880, 9371, 9568, and 11027; none references the retired Step 3.2 symbols.

## Proof

- `backend_prepare_frame_stack_call_contract_test` built successfully, but
  `ctest --test-dir build --output-on-failure -R
  '^backend_prepare_frame_stack_call_contract$' > test_after.log 2>&1` failed
  0/1 at the unrelated earlier direct-call contract; `test_after.log` records
  the failure.
- Direct build of `backend_prepared_lookup_helper_test` confirmed zero errors
  for `BirBlockEntryPublicationIdentityRequest` or
  `BirBlockEntryPublicationStatus`; unrelated pre-existing compile errors
  remain outside the owned section.
