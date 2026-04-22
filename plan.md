# Prepared Byval-Home Publication/Layout Correctness

Status: Active
Source Idea: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Activated from: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md

## Purpose

Keep idea 76 aligned to backend-suite progress instead of letting one probe case
drive the whole route. The owned problem is still upstream prepared byval-home
publication/layout correctness, but `00204.c` is only a probe for that family,
not the definition of success.

## Goal

Reduce the backend failure surface owned by idea 76 by repairing one generic
prepared byval-home publication/layout seam at a time and proving the result on
the owned family, with `00204.c` used only as one representative probe.

## Core Rule

Treat this as a failure-family runbook, not a single-testcase chase. Reject
fixes whose main value is making `c_testsuite_x86_backend_src_00204_c` run
slightly further without proving a broader prepared byval-home
publication/layout capability. Do not special-case helper names, one aggregate
width, or one emitted stack offset spelling.

## Read First

- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

## Scope

- x86 backend failures whose first bad fact is upstream prepared byval-home
  publication/layout overlap before helper call emission
- authoritative `PreparedValueHome`, `PreparedStackLayout`, and nearby producer
  surfaces that publish helper payload homes
- backend proof that distinguishes family-level progress from one-case churn

## Non-Goals

- treating `00204.c` as the only success metric for backend progress
- reopening idea 61 prepared-module traversal or call-bundle ownership
- reopening idea 68 local-slot handoff consumption
- reopening idea 75 helper consumer clobber work unless newly published facts
  prove the blocker moved downstream
- changing tests or expectations merely to hide overlapping published homes

## Working Model

- start from the backend failing set, then map owned failures into the idea 76
  family
- keep `00204.c` as a probe because it already exposes the family, but require
  at least one adjacent proof beyond that single probe before calling a packet
  progress
- prefer generic producer fixes over helper-renderer fragments
- when the first bad fact leaves publication/layout ownership, graduate the work
  immediately instead of continuing to mine `00204`

## Execution Rules

- prefer `build -> owned family proof -> broader backend spot check`
- keep the proving set tied to the idea 76 family, not just one testcase name
- record in `todo.md` whenever the owned family grows, shrinks, or graduates
- if the family currently has only one confirmed probe, say that explicitly and
  make the next packet about finding or ruling out adjacent owned cases before
  more code churn

## Step 1: Establish The Owned Failure-Family Baseline

Goal: define what part of `ctest -j8 -R backend` idea 76 actually owns, with
`00204.c` demoted to a probe.

Primary targets:

- `ctest -j8 -R backend` results or the freshest equivalent backend failure log
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `c_testsuite_x86_backend_src_00204_c`

Actions:

- inventory the current backend failing buckets instead of assuming `00204.c`
  is the route
- identify which failures actually match idea 76's upstream byval-home
  publication/layout ownership
- write down whether the owned family currently has multiple members or only
  one confirmed probe
- define the narrow proving set for the next packet in family terms

Completion check:

- `todo.md` names the current owned family, its representative proofs, and why
  `00204.c` is only a probe rather than the whole target

## Step 2: Isolate The Exact Producer Seam For The Owned Family

Goal: trace the common producer contract that makes the owned family publish
overlapping homes.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/prealloc.hpp`
- representative owned-family probes, including `00204.c` if it still fits

Actions:

- trace the first overlapping helper payload homes back through
  `PreparedValueHome` and `PreparedStackLayout` publication
- determine whether the root defect is value sizing, alignment normalization,
  byval-slice publication, or another producer contract
- if the family currently has only one confirmed member, explicitly prove why
  the seam is still generic before editing code

Completion check:

- one concrete producer seam is isolated, and the next packet no longer needs a
  generic search across helper rendering and upstream layout at the same time

## Step 3: Repair The Publication/Layout Producer Generically

Goal: publish truthful non-overlapping helper byval homes from authoritative
aggregate width/layout facts.

Primary targets:

- the exact producer surface isolated in Step 2
- shared prepared contract/layout helpers touched by that producer
- the owned-family proving set from Step 1

Actions:

- implement the smallest generic publication/layout repair that removes the
  current owned-family overlap without naming one helper or one aggregate case
- preserve existing downstream helper consumer contracts unless the newly
  published facts prove the blocker moved later
- prove the owned family advances beyond the current overlap and does not
  regress protected boundary coverage

Completion check:

- the owned-family probes no longer show the current overlapping-home seam, and
  any remaining blocker is explicitly reclassified

## Step 4: Reclassify Remaining Backend Failures After The Repair

Goal: convert the packet result into backend-suite progress instead of another
`00204` loop.

Primary targets:

- refreshed owned-family proof output
- refreshed backend failure snapshot
- downstream boundaries for ideas 71 and 75

Actions:

- rerun the proving set and inspect the next remaining mismatch after the owned
  seam is repaired
- compare the backend failing surface before and after the packet
- keep work in idea 76 only if the next bad fact is still publication/layout
- otherwise record explicit graduation or de-scoping in `todo.md`

Completion check:

- lifecycle state shows whether idea 76 still owns a backend failure family or
  whether the route has moved to a different leaf
