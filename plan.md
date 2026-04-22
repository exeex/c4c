# Post-Link Address-Exposed Local-Home Runtime Correctness

Status: Active
Source Idea: ideas/open/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md
Activated from: ideas/closed/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md

## Purpose

Move the active backend runtime leaf downstream now that the aggregate-call
handoff family is exhausted and `00204.c`'s first bad fact lives in
`myprintf`'s address-exposed local-home handling.

## Goal

Repair one post-link address-exposed local-home runtime seam at a time so
`00204.c` advances beyond the current `match(&s, ...)` crash toward correct
execution or a later, more precise runtime leaf.

## Core Rule

Treat this as a truthful same-module helper local-home correctness problem, not
as aggregate-call ABI work or a reason to add testcase-shaped stack-home
special cases.

## Read First

- `ideas/open/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md`
- `ideas/closed/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- the generated `build/c_testsuite_x86_backend/src/00204.c.s` `myprintf` body
  and matching prepared metadata for `%lv.s`

## Scope

- x86 backend cases that already pass prepared-module matching, assemble, and
  link, but still fail at runtime because an address-exposed local with a
  required permanent home slot disagrees with the emitted by-reference address
- truthful ownership for same-module helper local-home placement, refresh, and
  helper-call address agreement once codegen already succeeds
- runtime-only proof that distinguishes local-home correctness from earlier
  aggregate-call transport and later variadic traversal work

## Non-Goals

- reopening idea 72's aggregate-by-value argument or result handoff seams
- forcing this helper-local home mismatch back into idea 68's pre-codegen
  prepared local-slot handoff family
- silently absorbing genuine later `llvm.va_start` / `va_list` traversal bugs
  that should return to idea 71
- changing tests or expectations merely to mask the runtime defect
- widening into unrelated runtime debugging before the current `myprintf`
  permanent-home mismatch is understood

## Working Model

- keep one runtime seam per packet
- use `c_testsuite_x86_backend_src_00204_c` as the anchor proof case
- prefer semantic permanent-home placement and refresh repairs over helper-name
  or testcase-name shortcuts
- route the case onward immediately if the crash is replaced by a later,
  better-scoped runtime correctness blocker

## Execution Rules

- prefer `build -> focused runtime proof` for each accepted code slice
- keep runtime proof on the owned `00204.c` path and add nearby focused
  coverage only when it protects the changed local-home contract
- record in `todo.md` whenever the case stops crashing but still has a later
  wrong-result or downstream runtime blocker
- reject helper-shaped stack-slot aliases that only special-case `myprintf` or
  `00204.c`

## Step 1: Confirm The Address-Exposed Local-Home Crash Surface

Goal: restate the current post-link failure in same-module helper local-home
terms and identify the smallest owned seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- the emitted `myprintf` path around `%lv.s` and `match(&s, ...)`

Actions:

- rerun or inspect the focused proof for the current `00204.c` runtime crash
- confirm idea 72's aggregate-call family remains cleared
- identify the exact authoritative owner for `%lv.s`'s permanent home slot and
  the emitted address passed to helper calls
- choose the narrowest protective proof that still reflects the owned runtime
  contract

Completion check:

- the next executor packet is narrowed to one idea-73-owned local-home runtime
  seam with named proof targets and a clear boundary against ideas 68, 71, and
  72

## Step 2: Repair The Permanent-Home Agreement Runtime Seam

Goal: make emitted same-module helper code keep the live address-exposed local
value and every by-reference helper call on the same authoritative home.

Primary targets:

- the prepared local-home rendering surfaces that materialize and refresh
  address-exposed locals with permanent home slots
- same-module helper-call emission that passes addresses derived from those
  homes

Actions:

- repair the semantic owner that decides where `%lv.s` and similar locals live
  when `address_exposed=yes`, `requires_home_slot=yes`, and
  `permanent_home_slot=yes`
- keep the repair at the shared local-home ownership layer instead of adding
  helper-name exceptions for `myprintf` or `match`
- prove the owned case advances beyond the current corrupted `*s` handoff
  without reopening aggregate-call or pre-codegen ownership

Completion check:

- the targeted owned case no longer fails at the current `match(&s, ...)`
  local-home mismatch and instead executes further

## Step 3: Prove Runtime-Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-73 family and preserves
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

- accepted slices show real shrinkage of the idea-73 runtime family and keep
  downstream routing explicit

## Step 4: Continue The Loop Until Idea 73 Is Exhausted

Goal: repeat the Step 1 -> 3 loop until the remaining failures no longer
belong to this post-link address-exposed local-home runtime family.

Actions:

- keep idea 73 active only while owned cases still fail in this runtime family
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-73
  seam, or lifecycle state is ready to hand off because idea 73 no longer owns
  the remaining failures
