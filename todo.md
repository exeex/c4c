# Current Packet

Status: Active
Source Idea Path: ideas/open/787_lir_parallel_cfg_edge_verifier_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish and admit typed parallel successor occurrences

## Just Finished

- Plan Step 1: established typed-LIR verifier admission coverage for duplicate
  conditional successor IDs and duplicate ordered switch case-successor IDs.
  Each repeated occurrence retains its matching destination-label mirror;
  existing malformed, foreign, ambiguous, and misleading-display rejections
  remain covered.

## Suggested Next

- Execute plan Step 2: map typed-LIR parallel successor occurrences through
  the handoff into the next lowering boundary without altering Raw-BIR WIP.

## Watchouts

- Keep duplicate occurrences ordered and distinct; retain all non-duplicate
  malformed-authority rejection. The typed verifier already validates each
  conditional and switch occurrence independently; indirect-branch duplicate
  rejection is intentionally outside this packet. Do not touch Raw-BIR or
  parent receiver WIP.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
  The focused typed-LIR verifier subset passed; proof log: `test_after.log`.
