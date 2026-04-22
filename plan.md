# Post-Link Prepared Call-Lane Clobber Runtime Correctness

Status: Active
Source Idea: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Activated from: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md

## Purpose

Repair the downstream x86 consumer seam now exposed by `00204.c`: prepared
publishers are already truthful, but the final pre-call lowering still
collapses distinct homes into overlapping stack writes before `fa4(...)`
executes.

## Goal

Reduce idea 75's owned call-lane/address-lowering failure surface by isolating
one generic pre-call consumer clobber seam at a time and proving the result on
the owned family.

## Core Rule

Treat this as a downstream prepared-to-x86 consumer runbook, not a return/HFA
or upstream publisher chase. Reject fixes whose main value is making one call
site pass without explaining how truthful prepared homes are lowered into final
x86 addresses and copies generically.

## Read First

- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `build/c_testsuite_x86_backend/src/00204.c.s`

## Scope

- x86 backend failures whose first bad fact is a downstream pre-call consumer
  clobber after prepared publishers already emit truthful non-overlapping homes
- lowering paths that turn `PreparedValueHome`,
  `PreparedAddressingFunction`, and before-call move bundles into final x86
  addresses and copy order
- proof that distinguishes this consumer seam from upstream publication/layout
  and later return-value / HFA runtime ownership

## Non-Goals

- reopening idea 76 unless fresh proof shows the publishers themselves became
  contradictory again
- treating idea 77's return/HFA leaf as active before the first bad fact moves
  past the current pre-call overlap
- treating true `va_start` / `va_list` traversal work as part of this route
- changing tests or expectations merely to hide a consumer-side overlap

## Working Model

- start from the current `fa4(...)` corruption inside `arg()`, then trace where
  truthful prepared offsets collapse into the emitted `[rsp + 352..364]` writes
- treat the current overlap at `[rsp + 364]` as downstream of stack layout and
  regalloc publication, not proof that those publishers are wrong
- if the owned family still has only one confirmed probe, explicitly justify
  why the consumer seam is generic before editing code
- once the first bad fact moves into return/HFA runtime work, graduate the
  route immediately

## Execution Rules

- prefer `build -> focused runtime proof -> prepared-BIR/assembly inspection ->
  targeted x86 lowering inspection -> broader backend spot check`
- keep the proving set tied to the consumer seam, not just to one testcase
- record in `todo.md` whenever the owned family grows, shrinks, or graduates
- prefer shared x86 lowering fixes over callsite-shaped rewrites

## Step 1: Re-Establish The Downstream Consumer Ownership

Goal: restate exactly what idea 75 owns now that idea 76 proved the publishers
are truthful.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- `tests/c/external/c-testsuite/src/00204.c`
- `build/c_testsuite_x86_backend/src/00204.c.s`
- `ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md`
- `ideas/open/77_post_link_return_value_and_hfa_runtime_correctness_for_x86_backend.md`

Actions:

- confirm the first wrong fact still occurs in `arg()` before `Return values:`
- record the truthful prepared offsets already published for the `fa4(...)`
  setup and the final overlapping writes that replace them
- restate why idea 76 and idea 77 do not own that first remaining bad fact
- define the narrow proving set for the next packet in downstream-consumer
  terms

Completion check:

- `todo.md` names the active consumer seam, the representative proof command,
  and why the route belongs to idea 75

## Step 2: Isolate The Exact Lowering / Call-Lane Consumer

Goal: trace the emitted `fa4(...)` overlap to one concrete x86 lowering seam
that consumes already-truthful prepared homes.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- any adjacent x86 addressing helper touched by those consumers
- representative owned-family probes, including `00204.c`

Actions:

- trace how the `fa4(...)` byval copies and register ABI bindings are rendered
  after prepared publication is complete
- determine whether the root defect is address computation, frame-base choice,
  copy order, lane staging, or another downstream consumer contract
- if the family still has only one committed probe, isolate why the seam is
  generic before editing code

Completion check:

- one concrete downstream consumer seam is isolated, and the next packet no
  longer needs a generic search across publishers, x86 lowering, and later
  runtime work at the same time

## Step 3: Repair The Consumer Generically

Goal: lower the truthful prepared homes into non-overlapping final x86 copies
for the owned mixed aggregate/HFA call family.

Primary targets:

- the exact consumer surface isolated in Step 2
- any shared helper or coverage needed to prove the seam
- the owned proving set from Step 1

Actions:

- implement the smallest generic downstream repair that preserves truthful
  prepared-home ownership while removing the emitted overlap
- preserve upstream publication/layout boundaries and later return/HFA
  ownership unless newly observed facts prove the blocker moved
- prove the owned family advances beyond the current pre-call overlap and does
  not regress protected backend boundaries

Completion check:

- the owned-family probes no longer fail at the current overlap seam, and any
  remaining blocker is explicitly reclassified

## Step 4: Reclassify The Remaining Runtime Route

Goal: turn the packet result into durable lifecycle progress instead of another
single-probe loop.

Primary targets:

- refreshed owned-family proof output
- refreshed broader backend proof
- downstream boundaries for ideas 76 and 77

Actions:

- rerun the proving set and inspect the next remaining mismatch after the
  consumer seam is repaired
- compare the route before and after the packet
- keep work in idea 75 only if the next bad fact is still this downstream
  pre-call consumer clobber
- otherwise record explicit graduation or de-scoping in `todo.md`

Completion check:

- lifecycle state shows whether idea 75 still owns the active seam or whether
  the route has moved to a different leaf
