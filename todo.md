Status: Active
Source Idea Path: ideas/open/158_sema_validate_string_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Validate String Tables

# Current Packet

## Just Finished

Step 1: Inventory Validate String Tables completed for
`src/frontend/sema/validate.cpp`. The Validator state now has an in-file
inventory classifying the named string-keyed tables as semantic authority with
structured mirrors, compatibility mirrors, local syntax-name helpers, or legacy
template fallbacks, with owner/limitation/removal-condition notes for retained
rendered-name bridges.

## Suggested Next

Start the next Step 2 packet by converting one covered lookup path to prefer
structured keys/TextIds and to fail closed when complete metadata is present.

## Watchouts

- Do not treat rendered `std::string` lookup as authority when complete
  structured metadata is present.
- Do not weaken tests or mark supported paths unsupported.
- The Step 1 change is comment-only; any behavior change should be in a
  separate packet with focused tests for stale rendered names.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_') > test_after.log 2>&1`

Proof log: `test_after.log` reports 34/34 `positive_sema_` tests passed.
