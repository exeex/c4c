Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Reactivated the backend inventory umbrella after closing focused ideas 366 and
370. Current accepted full-suite baseline is `3347` passed, `28` failed,
`3375` total, with no pending baseline review.

## Suggested Next

Execute Step 1: reconstruct or capture the current backend-regex residual
surface after the recent closures.

Suggested initial command if existing logs are insufficient:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, or proof evidence that contradicts their
closure boundary.

## Proof

Activation-only lifecycle change. No implementation or validation proof was
run by the plan owner.
