Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Dispatch Bridge Surface Into Calls Owner API

# Current Packet

## Just Finished

Step 3 - Fold Dispatch Bridge Surface Into Calls Owner API completed the
post-review stale wording cleanup packet:

- Updated stale `calls_dispatch_bridge` ownership wording in `dispatch.cpp`
  and `README.md` to refer to the current calls owner API.
- Replaced stale `calls_moves` / bridge section labels in `calls.hpp` with
  current calls owner API labels.
- Kept implementation semantics, tests, ABI/prepare behavior, and expectations
  unchanged.

## Suggested Next

Next packet: supervisor should hand back to plan-owner for closure or final
validation handling.

## Watchouts

This was comment/docs/header-label cleanup only. The reviewer report at
`review/aarch64_calls_foldback_review.md` remains a transient review artifact.

## Proof

Passed:

`cmake --build --preset default`
