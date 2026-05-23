Status: Active
Source Idea Path: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Extract Shared Publication Helpers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/385_aarch64_dispatch_publication_mechanical_split.md`.

## Suggested Next

Delegate Step 1: extract shared publication helpers into
`dispatch_publication_common.cpp/hpp`, using `c4c-clang-tools` inspection before
moving the cluster and the plan's narrow proof command after the move.

## Watchouts

- Keep the split mechanical and behavior-preserving.
- Do not fold code into long-term feature homes such as `calls.cpp`,
  `memory.cpp`, or `comparison.cpp`.
- Record any helper intentionally left in `dispatch_publication.cpp` with the
  dependency reason instead of forcing a broad rewrite.

## Proof

Not run; lifecycle activation only.
