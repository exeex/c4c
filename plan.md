# Prepared Byval-Home Publication/Layout Correctness

Status: Active
Source Idea: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Activated from: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md

## Purpose

Move the active route upstream now that idea 75 step `2.2` proved the bounded
same-module helper lane is consuming overlapping byval homes that were already
published before the helper call.

## Goal

Repair one prepared byval-home publication/layout seam at a time so
`c_testsuite_x86_backend_src_00204_c` stops publishing overlapping helper
payload homes and advances into truthful downstream helper/runtime execution or
another later, better-fitting leaf.

## Core Rule

Treat this as an upstream prepared home publication/layout problem, not as a
reason to widen the bounded helper renderer or relax
`backend_x86_handoff_boundary` without proof. Reject fixes that special-case
`fa1`, `fa2`, one aggregate width, or one emitted stack offset spelling; the
active route must publish truthful byval homes generically from authoritative
aggregate width and layout facts.

## Read First

- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- the generated `build/c_testsuite_x86_backend/src/00204.c.s` around `fa1` and
  `fa2`

## Scope

- x86 backend cases that already clear semantic lowering, prepared-module
  traversal, and bounded helper-lane matching, but still publish overlapping
  byval payload homes before helper call rendering
- authoritative `PreparedValueHome`, `PreparedStackLayout`, and related stack
  object/frame-slot publication for helper byval payloads
- focused proof that distinguishes upstream home publication/layout from later
  helper consumption or variadic runtime work

## Non-Goals

- reopening idea 61's prepared-module or prepared call-bundle rejection family
- reopening idea 68's local-slot handoff-consumption family
- forcing the current overlap back into idea 75's downstream helper consumer
- changing helper-boundary tests or expectations merely to mask overlapping
  home publication

## Working Model

- keep one publication/layout seam per packet
- use `c_testsuite_x86_backend_src_00204_c` as the anchor proof case
- prefer contract-first publication/layout repair over helper-lane fragment
  rewrites
- route the case onward immediately if the first bad fact moves out of the
  helper byval-home publication/layout family

## Execution Rules

- prefer `build -> focused proof` for each accepted code slice
- keep proof on the owned `00204.c` path and `backend_x86_handoff_boundary`
  because the current route depends on preserving the existing bounded helper
  consumer contract while fixing upstream home publication/layout
- record in `todo.md` whenever the first bad fact advances beyond the current
  aggregate-string / mixed-aggregate overlap
- reject helper-name, aggregate-width, or one-offset shortcuts that do not
  repair publication/layout generally

## Step 1: Confirm The Prepared Byval-Home Publication/Layout Surface

Goal: restate the current first bad fact in upstream publication/layout terms
and identify the narrowest owned seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- generated `build/c_testsuite_x86_backend/src/00204.c.s` around `fa1` / `fa2`
- the published helper byval homes later consumed by
  `prepared_local_slot_render.cpp`

Actions:

- rerun or inspect the focused proof for the current aggregate-string /
  mixed-aggregate `[RUNTIME_MISMATCH]`
- confirm the helper renderer is only consuming prepublished byval homes and
  that the first overlap is already visible before the helper call
- identify the authoritative prepared home and stack-layout facts attached to
  the first overlapping helper payloads
- choose the narrowest protective proof that still reflects the owned
  publication/layout contract

Completion check:

- the next executor packet is narrowed to one idea-76-owned prepared
  publication/layout seam with explicit boundaries against ideas 61, 68, 71,
  and 75

## Step 2: Trace The Exact Publication/Layout Owner For The Overlapping Homes

Goal: identify which producer lane publishes the overlapping helper byval homes
and why aggregate width/alignment collapses to 8-byte spacing.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/prealloc.hpp`
- any nearby producer/helper used to publish stack objects, frame slots, or
  byval-copy home records for the affected helper payloads

Actions:

- trace the first overlapping `fa1` / `fa2` payload homes from emitted offsets
  back through `PreparedValueHome` and `PreparedStackLayout` publication
- determine whether the root defect is object sizing, frame-slot spacing,
  byval-slice publication, or another upstream prepared home/layout contract
- record the exact producer surface that owns the wrong overlap before editing
  x86 helper rendering

Completion check:

- one concrete producer seam is identified, and the executor no longer needs a
  generic search across helper rendering and upstream layout at the same time

## Step 3: Repair Prepared Byval-Home Publication/Layout Generically

Goal: publish truthful non-overlapping helper byval homes from authoritative
aggregate width/layout facts.

Primary targets:

- the exact producer surface isolated in Step 2
- shared prepared contract/layout helpers touched by that producer
- `c_testsuite_x86_backend_src_00204_c` aggregate-string / mixed-aggregate lane

Actions:

- implement the smallest generic publication/layout repair that prevents the
  current helper payload overlap without naming one helper or aggregate case
- preserve the existing bounded helper consumer contract unless the newly
  published facts prove that a later downstream leaf is now exposed
- prove the owned case advances beyond the current overlap without regressing
  `backend_x86_handoff_boundary`

Completion check:

- the generated helper payload homes for the first bad `fa1` / `fa2` lane no
  longer overlap at the current seam, and `00204.c` executes further or
  graduates into a later leaf

## Step 4: Reclassify The Next Visible Seam After Publication/Layout Repair

Goal: decide whether the next blocker remains upstream publication/layout work
or has moved downstream into helper/runtime ownership.

Primary targets:

- refreshed focused proof output for `00204.c`
- generated `00204.c.s` after the publication/layout repair
- the downstream boundaries for ideas 75 and 71

Actions:

- rerun the focused proof and inspect the first remaining mismatch after the
  repaired overlap
- keep the case in idea 76 only if the next bad fact is still published home
  publication/layout
- otherwise record explicit graduation back into the appropriate downstream
  leaf in `todo.md`

Completion check:

- lifecycle state clearly identifies whether idea 76 still owns the next packet
  or the case has moved to a later leaf
