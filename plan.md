# Prepared Byval-Home Publication/Layout Correctness

Status: Active
Source Idea: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Activated from: ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md

## Purpose

Repair the still-active upstream prepared byval-home publication/layout seam in
`00204.c` without drifting back into testcase-shaped debugging. The route is
not a downstream return/HFA leaf yet because the first remaining bad fact is
still visible before `fa4(...)` executes.

## Goal

Reduce idea 76's owned publication/layout failure surface by isolating one
generic mixed aggregate/HFA home-overlap seam at a time and proving the result
on the owned family.

## Core Rule

Treat this as an upstream layout/publication runbook, not a `00204.c`
line-by-line chase. Reject fixes whose main value is making one mixed call
print slightly more output without repairing the authoritative home publisher
or stack layout contract generically.

## Read First

- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `build/c_testsuite_x86_backend/src/00204.c.s`

## Scope

- x86 backend failures whose first bad fact is upstream prepared byval-home
  publication/layout overlap before a mixed aggregate/HFA call executes
- authoritative `PreparedValueHome`, stack-layout, and fixed-home publication
  surfaces that decide where aggregate and HFA payload fragments live
- proof that distinguishes upstream home overlap from later call-lane,
  return-value, or variadic runtime ownership

## Non-Goals

- treating idea 77's downstream return/HFA leaf as active before the first bad
  fact really moves there
- reopening idea 75 call-lane consumer clobber unless newly published facts
  prove homes are already truthful
- treating true `va_start` / `va_list` traversal work as part of this route
- changing tests or expectations merely to hide overlapping published homes

## Working Model

- start from the current `fa4(...)` corruption inside `arg()`, then trace that
  overlap back to one authoritative publication/layout seam
- treat the current assembly overlap at `[rsp + 364]` as proof that ownership
  is still upstream
- if the owned family still has only one confirmed probe, explicitly justify
  why the seam is generic before editing code
- once the first bad fact moves downstream, graduate the work immediately

## Execution Rules

- prefer `build -> focused runtime proof -> targeted assembly inspection ->
  broader backend spot check`
- keep the proving set tied to the upstream publication/layout seam, not just
  to one testcase name
- record in `todo.md` whenever the owned family grows, shrinks, or graduates
- prefer shared layout/publication fixes over helper-specific or callsite-shaped
  rewrites

## Step 1: Re-Establish The Owned Publication/Layout Seam

Goal: restate exactly what idea 76 owns now that the idea 77 switch was
rejected by fresh proof.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- `tests/c/external/c-testsuite/src/00204.c`
- `build/c_testsuite_x86_backend/src/00204.c.s`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md`

Actions:

- confirm the first wrong fact occurs in `arg()` before `Return values:`
- record the concrete overlapping homes at the current `fa4(...)` call site
- restate why idea 75 and idea 77 do not own that first remaining bad fact
- define the narrow proving set for the next packet in upstream-layout terms

Completion check:

- `todo.md` names the current owned seam, the representative proof command, and
  why the route still belongs to idea 76

## Step 2: Isolate The Authoritative Home Publisher

Goal: trace the `fa4(...)` overlap back to one authoritative prepared-home or
  stack-layout producer seam.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- representative owned-family probes, including `00204.c`

Actions:

- trace how the mixed aggregate/HFA payload homes for `fa4(...)` are assigned
- determine whether the root defect is family grouping, offset publication,
  alignment normalization, or another upstream layout contract
- if the family still has only one committed probe, isolate why the seam is
  generic before editing code

Completion check:

- one concrete upstream publication/layout seam is isolated, and the next
  packet no longer needs a generic search across layout, call-lane, and
  downstream runtime work at the same time

## Step 3: Repair The Publication/Layout Producer Generically

Goal: publish truthful non-overlapping mixed aggregate/HFA homes from the
authoritative layout facts isolated in Step 2.

Primary targets:

- the exact producer/consumer surface isolated in Step 2
- any shared helper or coverage needed to prove the seam
- the owned proving set from Step 1

Actions:

- implement the smallest generic publication/layout repair that removes the
  current overlap without naming one testcase or one helper symbol
- preserve downstream call-lane and runtime ownership boundaries unless newly
  observed facts prove the blocker moved later
- prove the owned family advances beyond the current overlap and does not
  regress protected backend boundaries

Completion check:

- the owned-family probes no longer fail at the current overlap seam, and any
  remaining blocker is explicitly reclassified

## Step 4: Reclassify The Remaining Runtime Route

Goal: turn the packet result into durable lifecycle progress instead of another
single-probe loop.

Primary targets:

- refreshed owned-family proof output
- refreshed broader backend proof
- downstream boundaries for ideas 75 and 77

Actions:

- rerun the proving set and inspect the next remaining mismatch after the owned
  seam is repaired
- compare the route before and after the packet
- keep work in idea 76 only if the next bad fact is still publication/layout
- otherwise record explicit graduation or de-scoping in `todo.md`

Completion check:

- lifecycle state shows whether idea 76 still owns the active seam or whether
  the route has moved to a different leaf
