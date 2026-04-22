# Post-Link Byval-Param Pointer-Argument Runtime Correctness

Status: Active
Source Idea: ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md
Activated from: ideas/closed/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md

## Purpose

Move the active backend runtime leaf downstream now that the `%lv.s`
address-exposed `local_slot` seam is cleared and `00204.c`'s first bad fact
lives in fixed-arity helper pointer-argument materialization for
address-exposed `byval_param` values.

## Goal

Repair one post-link byval-param pointer-argument runtime seam at a time so
`00204.c` advances beyond the current `fa_s*` `Arguments:` mismatch toward
correct execution or a later, more precise runtime leaf.

## Core Rule

Treat this as a truthful pointer-materialization correctness problem for
address-exposed `byval_param` permanent homes, not as a reason to reopen the
already-cleared `%lv.s` local-slot seam or to add testcase-shaped helper
special cases.

## Read First

- `ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md`
- `ideas/closed/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- the generated `build/c_testsuite_x86_backend/src/00204.c.s` around `fa_s1`
  through `fa_s17` and matching prepared metadata for `%p.a` / `%t2`

## Scope

- x86 backend cases that already pass prepared-module matching, assemble, and
  link, but still fail at runtime because emitted pointer arguments for
  address-exposed `byval_param` values come from the wrong home
- truthful ownership for authoritative byval home selection and pointer
  materialization in fixed-arity helpers once codegen already succeeds
- runtime-only proof that distinguishes byval-param pointer correctness from
  earlier aggregate transport, local-slot home placement, and later variadic
  traversal work

## Non-Goals

- reopening idea 72's aggregate-by-value transport seam without fresh proof
- reopening idea 73's `%lv.s` / `myprintf` local-slot permanent-home route
- forcing this pointer-materialization defect back into idea 68's pre-codegen
  prepared local-slot handoff family
- silently absorbing genuine later `llvm.va_start` / `va_list` traversal bugs
  that should return to idea 71
- changing tests or expectations merely to mask the runtime defect

## Working Model

- keep one runtime seam per packet
- use `c_testsuite_x86_backend_src_00204_c` as the anchor proof case
- prefer semantic byval-home and pointer-materialization repairs over helper
  name or testcase name shortcuts
- route the case onward immediately if the first bad fact moves out of the
  fixed-arity byval-param pointer-argument family

## Execution Rules

- prefer `build -> focused runtime proof` for each accepted code slice
- keep runtime proof on the owned `00204.c` path and add nearby focused
  coverage only when it protects the changed pointer-materialization contract
- record in `todo.md` whenever the case advances beyond the current `fa_s*`
  seam but still has a later wrong-result or downstream runtime blocker
- reject helper-shaped aliases that only special-case `fa_s1`, `fa_sN`, or
  `00204.c`

## Step 1: Confirm The Byval-Param Pointer-Argument Crash Surface

Goal: restate the current post-link failure in fixed-arity byval-param
pointer-materialization terms and identify the smallest owned seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- emitted `fa_s1` through nearby `fa_sN` helpers
- prepared metadata for `%p.a` and the pointer operands forwarded to `printf`

Actions:

- rerun or inspect the focused proof for the current `00204.c`
  `[RUNTIME_MISMATCH]`
- confirm idea 73's `%lv.s` local-home family remains cleared
- identify the authoritative owner for address-exposed `byval_param`
  permanent-home metadata and the emitted pointer argument home used by helper
  calls
- choose the narrowest protective proof that still reflects the owned runtime
  contract

Completion check:

- the next executor packet is narrowed to one idea-74-owned byval-param
  pointer-materialization seam with a clear boundary against ideas 68, 71, 72,
  and 73

## Step 2: Repair The Authoritative Byval-Home Pointer Materialization Seam

Goal: make emitted fixed-arity helper code derive by-reference pointer
arguments from the same authoritative home that owns the address-exposed
`byval_param` value.

Primary targets:

- the prepared/home-selection surfaces that resolve authoritative stack homes
  for address-exposed `byval_param` values
- pointer-argument emission paths that currently forward transient register
  homes such as `%t2` / `%r12`

Actions:

- repair the semantic owner that decides how pointer arguments are materialized
  when `source_kind=byval_param`, `address_exposed=yes`, and
  `permanent_home_slot=yes`
- keep the repair at the shared ownership layer instead of adding helper-name
  exceptions for `fa_s*` or `printf`
- prove the owned case advances beyond the current fixed-arity
  pointer/home mismatch without reopening cleared local-slot or aggregate
  transport routes

Completion check:

- the targeted owned case no longer fails at the current `fa_s*`
  pointer-materialization mismatch and instead executes further

## Step 3: Prove Runtime-Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-74 family and preserves
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

- accepted slices show real shrinkage of the idea-74 runtime family and keep
  downstream routing explicit

## Step 4: Continue The Loop Until Idea 74 Is Exhausted

Goal: repeat the Step 1 -> 3 loop until the remaining failures no longer
belong to this post-link byval-param pointer-argument runtime family.

Actions:

- keep idea 74 active only while owned cases still fail in this runtime family
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-74
  seam, or lifecycle state is ready to hand off because idea 74 no longer owns
  the remaining failures
