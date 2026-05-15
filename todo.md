Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: End-To-End Binary128 Route Proof

# Current Packet

## Just Finished

Step 6 completed the representative binary128 AArch64 route proof. The
instruction-dispatch test now builds one prepared F128 block that selects a
full-width frame-slot load, `__addtf3` helper boundary, full-width store-back,
and return, then prints the selected AArch64 machine nodes from structured
records.

## Suggested Next

Hand the completed Step 6 slice back to the supervisor for review/commit and
plan-owner lifecycle decision.

## Watchouts

- The end-to-end proof is still a prepared-backend route proof, not a source-C
  long-double frontend/BIR admission proof.
- Unsupported F128 sign-bit-like operations and unsupported helper families
  remain covered by fail-closed dispatch diagnostics in the same backend proof.
- F128 constants remain outside this runbook's authority until the separate
  full-width constant-carrier idea provides structured source facts.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Passed, 139/139 tests. Proof log: `test_after.log`.
