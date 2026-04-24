# Plan Review Metadata Sync Repair

Status: Closed
Last Updated: 2026-04-23

## Closure Note

This idea was created from a false premise during lifecycle repair work.

The temporary route assumed `todo.md` needed a constantly displayed
`Plan Review Counter` line. The actual intended contract is narrower:

- keep `Current Step ID` and `Current Step Title` near the top of `todo.md`
- write reminder lines into `todo.md` only when a hook-managed limit is hit
- do not read or write `todo.md` from `post-commit` while no reminder is
  pending

The underlying hook behavior has been repaired in `scripts/plan_review_state.py`,
but this did not represent a separate durable initiative from the parser
namespace work under idea 82. Close this file as a mistaken branch and resume
the active lifecycle state from idea 82.
