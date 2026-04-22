# Post-Link Prepared/X86 Call-Lane Clobber Runtime Correctness

Status: Active
Source Idea: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Activated from: ideas/closed/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md

## Purpose

Move the active backend runtime leaf downstream now that the byval-param
pointer-owner publication seam is cleared and `00204.c`'s first bad fact lives
in prepared/x86 call-lane source clobber after semantic ownership is already
truthful.

## Goal

Repair one post-link prepared/x86 call-lane runtime seam at a time so
`00204.c` advances beyond the current overlapping argument-home corruption
toward correct execution or a later, more precise runtime leaf.

## Core Rule

Treat this as a prepared call-lane source-preservation and move-order
correctness problem, not as a reason to reopen idea 74's owner publication or
to add helper-shaped register shuffles. Reject fixes that special-case
`fa_s17`, `pll`, `printf`, or one `mov rsi, rdi` spelling; the active route
must preserve overlapping argument homes generically from authoritative
prepared call ownership.

## Read First

- `ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md`
- `ideas/closed/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/calls.cpp`
- `src/backend/mir/x86/codegen/shared_call_support.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- the generated `build/c_testsuite_x86_backend/src/00204.c.s` around `fa_s17`
  and `pll`, plus matching prepared bundle data for the affected `printf` calls

## Scope

- x86 backend cases that already pass prepared-module matching, assemble, and
  link, but still fail at runtime because emitted fixed-arity call lanes copy
  an argument from a register that an earlier lane already rewrote
- truthful consumption of overlapping call-argument homes, BeforeCall move
  obligations, and renderer sequencing once semantic owners are already
  authoritative
- runtime-only proof that distinguishes call-lane clobber from earlier
  byval-owner publication and later variadic traversal work

## Non-Goals

- reopening idea 74's byval-param pointer-owner publication seam
- reopening idea 61's pre-assembly prepared call-bundle rejection family
- forcing this call-lane defect back into idea 71's variadic runtime leaf
- changing tests or expectations merely to mask the runtime defect

## Working Model

- keep one runtime seam per packet
- use `c_testsuite_x86_backend_src_00204_c` as the anchor proof case
- prefer normalized prepared call-lane plans and source-preserving move
  application over helper-name or register-pair shortcuts
- route the case onward immediately if the first bad fact moves out of the
  overlapping fixed-arity call-lane family

## Execution Rules

- prefer `build -> focused runtime proof` for each accepted code slice
- keep runtime proof on the owned `00204.c` path and add nearby focused
  coverage only when it protects the changed call-lane contract
- record in `todo.md` whenever the case advances beyond the current call-lane
  clobber seam but still has a later wrong-result or downstream runtime blocker
- reject helper-shaped aliases that only special-case `fa_s17`, `pll`,
  `printf`, or one overlapping register tuple

## Step 1: Confirm The Prepared/X86 Call-Lane Clobber Surface

Goal: restate the current post-link failure in overlapping call-lane terms and
identify the smallest owned seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- emitted `fa_s17` and `pll` call lanes
- prepared argument homes and BeforeCall bundles for the affected `printf`
  calls

Actions:

- rerun or inspect the focused proof for the current `00204.c`
  `[RUNTIME_MISMATCH]`
- confirm idea 74's owner-publication family remains cleared in dumped BIR and
  prepared BIR
- identify the authoritative prepared argument homes, move obligations, and
  emitted move order at the first clobbered call lane
- choose the narrowest protective proof that still reflects the owned runtime
  contract

Completion check:

- the next executor packet is narrowed to one idea-75-owned overlapping
  call-lane seam with a clear boundary against ideas 61, 71, and 74

## Step 2: Repair The Overlapping Call-Lane Source-Preservation Seam

Goal: make emitted fixed-arity call lanes preserve source values when
authoritative prepared argument homes overlap ABI argument registers.

Primary targets:

- prepared call-lane consumers that plan or apply BeforeCall moves
- x86 call render paths that currently materialize arg0 into `rdi` before arg1
  still sources the old `rdi` home

Actions:

- repair the generic prepared/x86 call-lane planning or move-application layer
  so overlapping sources are preserved through truthful ordering or a shared
  temporary rule derived from prepared ownership
- keep the repair at the shared call-lane layer instead of adding helper,
  callee-name, or register-pair exceptions
- prove the owned case advances beyond the current `fa_s17` / `pll` clobber
  without reopening idea 74's owner-publication seam

Completion check:

- the targeted owned case no longer fails at the current overlapping
  call-lane clobber and instead executes further

## Step 3: Prove Runtime-Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-75 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned case plus the nearest runtime
  coverage that protects the changed contract
- record in `todo.md` when the case graduates into a later wrong-result,
  variadic traversal, or other runtime-only leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-75 runtime family and keep
  downstream routing explicit

## Step 4: Continue The Loop Until Idea 75 Is Exhausted

Goal: repeat the Step 1 -> 3 loop until the remaining failures no longer
belong to this post-link prepared/x86 call-lane clobber runtime family.

Actions:

- keep idea 75 active only while owned cases still fail in this runtime family
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-75
  seam, or lifecycle state is ready to hand off because idea 75 no longer owns
  the remaining failures
