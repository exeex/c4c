Status: Active
Source Idea Path: ideas/open/193_route3_prepared_policy_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add Positive And Negative Proof

# Current Packet

## Just Finished

Completed Step 3 by adding AArch64 consumer-level proof for the indirect-callee
stored-value-source boundary selected in Step 2.

Changed `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`:

- Added a selected stored indirect-callee fixture that stores a selected global
  pointer source, reloads it as the indirect callee, and requires prepared
  direct-global/select-chain policy before materializing `csel ...; blr`.
- Added a positive exact same-range Route 3 identity case where BIR store/load
  range facts match while prepared stored-source range facts are intentionally
  mismatched, so prepared fallback cannot authorize the identity.
- Added a negative/fallback case where BIR memory ranges are erased so Route 3
  lacks range authority, while prepared addressing remains exact and preserves
  the selected-callee policy.

## Suggested Next

Execute Step 4 by summarizing the accepted boundary and retained prepared
oracles/readiness notes, then run the supervisor-selected broader validation if
required.

## Watchouts

- The positive test deliberately mismatches only prepared store-slot identity
  while leaving printable prepared memory facts intact; that keeps the proof at
  the AArch64 consumer boundary without weakening supported-path expectations.
- The negative test deliberately erases only BIR memory range authority and
  relies on prepared addressing/source-producer policy for the stored-source
  fallback.
- No implementation files were changed for this Step 3 slice.

## Proof

`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log"` passed.

Result: build succeeded; 180/180 `backend_` tests passed. Proof log:
`test_after.log`.
