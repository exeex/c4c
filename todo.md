Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Reactivated the backend inventory umbrella after closing focused idea 375.
Current canonical close proof for idea 375 was lifecycle-only from the
inventory perspective: matching `^backend_` before/after logs reported 144/144
backend tests passing, and no implementation should happen under idea 295.

## Suggested Next

Execute Step 1: reconstruct or capture the current backend-regex residual
surface after idea 375. Existing `test_after.log` covers only `^backend_`, so
a fresh backend-regex capture is likely needed before classification.

Suggested initial command if the supervisor approves the capture:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. Reclassify the parked buckets after
removing the resolved `00218` fact from the current surface.

## Proof

Activation-only lifecycle change. No implementation or validation proof was
run by the plan owner.
