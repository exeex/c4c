Status: Active
Source Idea Path: ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Coverage

# Current Packet

## Just Finished

Step 4 - Add Focused Coverage completed focused classifier-contract coverage
for ambiguous non-parallel multi-source stack-destination prepared move
bundles.

Coverage now proves:

- A before-instruction, non-parallel `authority=none` bundle with two explicit
  register-source moves targeting the same stack destination value rejects with
  shared category
  `ambiguous_non_parallel_multi_source_stack_destination`.
- The adjacent same-destination case where the two moves target distinct
  prepared value ids backed by the same explicit stack-slot home also rejects
  with the same shared category, so the contract is not row-name or value-id
  dependent.
- The accepted/stable single register-source to stack-destination move remains
  `available` at the prepared-object consumer boundary.
- RV64 prepared object emission preserves the shared producer/classifier
  diagnostic for the same multi-source prepared move-bundle shape before
  fragment materialization.

## Suggested Next

Execute Step 5 validation/handoff or ask the plan owner to decide whether this
runbook is ready to close. No implementation follow-up is suggested from this
executor slice.

## Watchouts

Missing value-home lookup facts stay outside the new semantic classification
and continue to fall through to existing consumer/RV64 diagnostics. Unsupported
banks, unsupported widths, missing source or destination sizes, and incoherent
register identities remain intentionally RV64-local materialization rejects
unless a later producer contract publishes stronger authority.

## Proof

Proof completed for this coverage packet:

- `cmake --build --preset default` passed.
- Delegated proof command
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  345/345 backend tests, with final output captured in `test_after.log`.
- `git diff --check` passed for touched files.
