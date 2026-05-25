Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local-Frame Publication Decision

# Current Packet

## Just Finished

Step 2 removed the selected local-frame publication decision from
`src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`.

- `make_frame_slot_call_argument_address_source` now gates fallback local-frame
  address publication on
  `argument.allows_local_aggregate_address_publication`.
- `make_local_frame_address_call_argument_source` now gates register-homed
  local-frame address publication on the same prepared fact.
- Deleted the obsolete helper chain that re-read `CallInst::arg_types`,
  `CallInst::arg_abi`, and the callee spelling to reconstruct publication
  eligibility.
- Updated the focused manual AArch64 dispatch fixture so its expected
  publication path explicitly carries the prepared eligibility fact.

## Suggested Next

Supervisor should review the Step 2 slice for acceptance and choose whether the
active plan needs a next packet or lifecycle handling.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark a nearby publication case unsupported to claim
  progress.
- The focused fixture update is a contract alignment: direct helper tests that
  bypass prealloc must now set
  `allows_local_aggregate_address_publication` when expecting publication.
- `rg -n "arg_abi|arg_types|call_argument_allows_local_frame_address_publication|call_argument_is_pointer|call_argument_is_byval_copy|call_argument_call|allows_local_aggregate_address_publication" src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
  shows only the two prepared-fact gates in this file.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Proof log: `test_after.log` contains `100% tests passed, 0 tests failed out of 162`.
