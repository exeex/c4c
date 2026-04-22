# Post-Link Return-Value And HFA Runtime Correctness

Status: Active
Source Idea: ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md
Activated from: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md

## Purpose

Keep the next `00204.c` runtime route grounded in the downstream seam that now
remains after idea 76's publication/layout repair. The owned problem is no
longer helper byval-home publication; it is later post-link return-value / HFA
runtime correctness once argument publication is already truthful.

## Goal

Reduce idea 77's owned runtime failure surface by isolating one generic
return-value / HFA carrier seam at a time and proving the result on the owned
family without reopening upstream publication/layout work.

## Core Rule

Treat this as a downstream runtime leaf, not a license to keep poking
`00204.c` from every direction. Reject fixes whose main value is making one
probe print a little more output by reintroducing publication/layout special
cases, helper-specific hacks, or expectation churn instead of repairing a real
return-value / HFA runtime contract.

## Read First

- `ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md`
- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/open/71_post_link_variadic_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/memory.cpp`
- `tests/backend/backend_x86_route_debug_test.cpp`

## Scope

- x86 backend failures whose first remaining bad fact is a post-link aggregate
  return-value or HFA runtime mismatch after helper argument publication is
  already truthful
- authoritative prepared/x86 return-lane and copyout surfaces that decide how
  floating or aggregate results are carried at runtime
- proof that distinguishes return-value / HFA runtime ownership from earlier
  byval publication defects and later variadic traversal work

## Non-Goals

- reopening idea 76 publication/layout work unless a new first bad fact proves
  helper payload homes overlap again
- reopening idea 75 call-lane consumer clobber unless the first remaining bad
  fact moves back before the helper call
- treating true `va_start` / `va_list` traversal failures as part of this leaf
- changing tests or expectations merely to hide a downstream runtime mismatch

## Working Model

- start from the current `00204.c` runtime mismatch, then map the first wrong
  fact specifically into return values or HFA output instead of calling the
  whole late runtime surface one bucket
- keep argument publication repair treated as already proven context, not as
  search space to reopen in the next packet
- if the owned family still has only one confirmed probe, explicitly isolate
  the generic carrier seam before editing code
- once the first bad fact moves into true variadic traversal, graduate the work
  immediately instead of stretching this leaf

## Execution Rules

- prefer `build -> focused runtime proof -> broader backend spot check`
- keep the proving set tied to the downstream return-value / HFA seam, not just
  to one big testcase name
- add committed focused runtime probes when they make the owned seam easier to
  prove without depending only on `00204.c`
- record in `todo.md` whenever the owned family grows, shrinks, or graduates

## Step 1: Re-Establish The Owned Runtime Failure Family

Goal: restate exactly what idea 77 owns now that idea 76's publication/layout
repair is in place.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- the latest focused runtime proof for `00204.c`
- `tests/c/external/c-testsuite/src/00204.c`
- `ideas/open/71_post_link_variadic_runtime_correctness_for_x86_backend.md`
- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`

Actions:

- confirm that the `Arguments:` portion still reflects the repaired upstream
  byval-home publication path
- identify the first wrong line in the later `Return values:` / `HFA` output
- write down whether the current owned family has one confirmed probe or any
  adjacent committed runtime probes already exist
- define the narrow proving set for the next packet in downstream-runtime terms

Completion check:

- `todo.md` names the owned runtime seam, the representative proof command, and
  why ideas 71, 75, and 76 do not own the first remaining bad fact

## Step 2: Isolate The Return/HFA Carrier Contract

Goal: trace the first wrong runtime fact back to one authoritative x86 return
or copyout carrier seam.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/memory.cpp`
- `tests/backend/backend_x86_route_debug_test.cpp`
- representative owned-family probes, including `00204.c`

Actions:

- trace the first wrong runtime line through the prepared/x86 return or copyout
  path that materializes that value
- determine whether the root defect is return ABI carrier selection, floating
  fragment copyout, sret-adjacent handoff, or another downstream runtime
  contract
- if the family still has only one committed probe, isolate why the seam is
  generic before editing code

Completion check:

- one concrete downstream carrier seam is isolated, and the next packet no
  longer needs a generic search across return ABI, helper arguments, and
  variadic traversal at the same time

## Step 3: Repair The Downstream Runtime Seam Generically

Goal: emit truthful post-link return-value / HFA runtime behavior from the
authoritative carrier facts isolated in Step 2.

Primary targets:

- the exact producer/consumer surface isolated in Step 2
- any shared helper or route-debug coverage needed to prove the seam
- the owned runtime proving set from Step 1

Actions:

- implement the smallest generic runtime repair that removes the current wrong
  return-value / HFA seam without naming one testcase or one helper symbol
- preserve the already-proved publication/layout and helper-argument behavior
  unless newly observed facts prove the blocker moved back upstream
- prove the owned runtime family advances beyond the current mismatch and does
  not regress protected backend boundaries

Completion check:

- the owned-family probes no longer fail at the current return-value / HFA
  seam, and any remaining blocker is explicitly reclassified

## Step 4: Reclassify The Remaining Runtime Route

Goal: turn the packet result into durable lifecycle progress instead of another
single-probe loop.

Primary targets:

- refreshed owned-family runtime proof output
- refreshed broader backend proof
- downstream boundary for idea 71

Actions:

- rerun the proving set and inspect the next remaining mismatch after the owned
  return/HFA seam is repaired
- compare the downstream runtime surface before and after the packet
- keep work in idea 77 only if the next bad fact is still return-value / HFA
  runtime ownership
- otherwise record explicit graduation or de-scoping in `todo.md`

Completion check:

- lifecycle state shows whether idea 77 still owns a downstream runtime family
  or whether the route has moved to a different leaf
