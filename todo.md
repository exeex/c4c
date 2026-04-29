Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extend Structured Identity Across Blocks And Types

# Current Packet

## Just Finished

Step 4 completed the BIR verifier block-label membership slice.

`bir_validate.cpp` now collects structured `BlockLabelId` definitions alongside
legacy raw block labels. Phi incoming labels, branch targets, and conditional
branch targets prefer structured membership whenever the reference carries a
valid id; raw-string membership remains the compatibility path only when the
reference id is absent. The verifier also rejects duplicate or unknown block
definition ids.

Added focused backend verifier coverage proving stale raw label text cannot
override structured ids for phi incoming labels, plain branch targets, and
conditional branch targets, while raw-only BIR remains valid when ids are
absent.

## Suggested Next

Continue Step 4 by extending the same structured-id preference to the next
block/type validation boundary that already carries ids. Keep raw fallback
limited to carriers whose structured id is absent.

## Watchouts

This packet intentionally did not change BIR printing, lowering, prepared BIR,
or LIR producers. Block references still require non-empty raw label strings
even when ids are present, matching the existing BIR surface contract.

## Proof

Passed delegated Step 4 proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: build completed; `ctest` reported 100% tests passed, 0 failed out of
108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
