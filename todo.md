Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Surviving Boundary Leak

# Current Packet

## Just Finished

Step 5 - Closure Review: source-idea closure rejected after the block-entry
preservation republication checkpoint.

Decision: keep the source idea active with a regenerated checkpoint focused on
the next surviving AArch64 calls boundary leak.

Closure blockers:

- The source idea acceptance criteria cover the full AArch64 `calls*`
  consolidation, not only the completed block-entry republication route.
- Multiple `calls*.cpp` files and a broad `calls.hpp` helper surface remain.
- `calls_dispatch_bridge.cpp` still mixes call-emission bridge work with
  dispatch recovery, same-block scalar materialization, local aggregate address
  publication, indirect callee/result materialization, and prepared-call helper
  selection.
- Adjacent argument-source, byval, move, preservation, and printing helpers
  still need boundary review before the source idea can be called complete.
- Close-time regression guard was not accepted because source-idea completion
  is false; the canonical `test_after.log` referenced by the previous Step 4
  packet was also absent in this checkout.

## Suggested Next

Execute Step 1 - Select The Next Surviving Boundary Leak.

Start with `calls_dispatch_bridge.cpp` and `calls.hpp`. Pick one concrete
boundary leak, identify the correct prepared/emission/dispatch/printer owner,
and record the focused proof command before implementation.

## Watchouts

- Do not touch `ideas/open/03_dispatch_responsibility_reduction.md`.
- Keep any dispatch work limited to AArch64 call-emission bridge boundaries.
- Do not invent a new shared prepared-call API unless the selected boundary
  proves a required prepared fact is missing.
- Do not weaken unsupported or expected-output contracts.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Proof

Lifecycle-only review; no build or test command was run.

Prior executor note reported:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

with 100% tests passed, 0 tests failed out of 162, but `test_after.log` was not
present in this checkout during the closure review.
