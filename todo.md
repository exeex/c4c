# Execution State

Status: Active
Source Idea Path: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Establish The Owned Publication/Layout Seam
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Lifecycle repair completed after the idea 77 switch proved premature. Fresh
focused runtime proof for `c_testsuite_x86_backend_src_00204_c` still fails
first in `arg()` before `Return values:` begins, and regenerated
`build/c_testsuite_x86_backend/src/00204.c.s` shows the concrete overlap at
the `fa4(...)` call site: the byte home for `s1` at `[rsp + 364]` is
overwritten by `hfa14.d`. The active runbook therefore switches back to idea
76's upstream prepared byval-home publication/layout route.

## Suggested Next

Take idea 76 step `1` in upstream-layout terms:

- rerun the focused `00204.c` proof as the representative seam check
- record the concrete overlapping homes at the `fa4(...)` call site
- restate why ideas 75 and 77 do not own that first bad fact
- define the narrow proving set for the next packet around the upstream
  publication/layout seam

## Watchouts

- Do not treat downstream `Return values:` corruption as the owned seam until
  the first bad fact actually moves past `arg()`.
- Do not reopen idea 75 call-lane consumer work unless the published homes are
  already truthful at the current `fa4(...)` site.
- Do not treat true `va_start` / `va_list` traversal defects as part of this
  route.
- Keep the next packet focused on authoritative home publication/layout, not
  on speculative runtime or expectation rewrites.

## Proof

Lifecycle repair only. Fresh route-classification proof on 2026-04-22:

- `cmake --build --preset default`
- `ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$'`
- inspect `build/c_testsuite_x86_backend/src/00204.c.s` around the `fa4(...)`
  call site
