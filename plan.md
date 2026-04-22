# Post-Link Aggregate Call Runtime Correctness

Status: Active
Source Idea: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Activated from: ideas/open/71_post_link_variadic_runtime_correctness_for_x86_backend.md

## Purpose

Move the active backend leaf downstream now that idea 70's post-assembly
closure family is exhausted and `00204.c` links successfully.

## Goal

Repair one post-link fixed-arity aggregate-call runtime seam at a time so
`00204.c` progresses from its current helper crash toward correct execution or
a later, more precise runtime leaf.

## Core Rule

Do not treat link closure as runtime correctness. Progress here must come from
truthful aggregate-call ABI/runtime semantics and execution behavior, not from
testcase-shaped helper shims or expectation rewrites.

## Read First

- `ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md`
- `ideas/closed/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md`
- `tests/c/external/c-testsuite/src/00204.c`
- the fixed-arity helper and aggregate-call surfaces exposed by the current
  `00204.c` crash path
- nearby x86 call-lane, aggregate-home, or helper-emission surfaces that own
  the first crashing `fa_s*` route

## Scope

- x86 backend cases that already pass prepared-module matching, assemble, and
  link, but still fail afterward in emitted fixed-arity aggregate call/runtime
  behavior
- truthful ownership for aggregate-by-value call setup, argument home
  selection, helper forwarding, and nearby fixed-arity helper semantics
- runtime-only proof that distinguishes aggregate-call execution correctness
  from earlier link-closure work

## Non-Goals

- reopening idea 70's post-assembly same-module or direct variadic-runtime
  link-closure seams
- silently absorbing genuine later variadic-runtime defects that should still
  belong to idea 71
- changing boundary tests merely to mask runtime defects
- widening into unrelated runtime debugging that is not on the first aggregate
  helper crash route
- claiming success before the owned case executes without the current crash

## Working Model

- keep one runtime seam per packet
- use `c_testsuite_x86_backend_src_00204_c` as the anchor proof case
- prefer semantic aggregate-call/runtime repairs over testcase-specific runtime
  hacks
- route the case onward immediately if the crash is replaced by a later,
  better-scoped runtime correctness blocker

## Execution Rules

- prefer `build -> focused runtime proof` for each accepted code slice
- keep runtime proof on the owned `00204.c` path and add nearby focused
  coverage only when it protects the changed runtime contract
- record in `todo.md` whenever the case stops crashing but still has a later
  wrong-result or downstream runtime blocker
- reject helper-name or testcase-named shortcuts that only special-case
  `00204.c`

## Step 1: Confirm The Aggregate-Call Crash Surface

Goal: restate the current post-link failure in backend-owned aggregate-call
runtime terms and identify the smallest owned seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- the first crashing fixed-arity helper/runtime path now reached after link
  succeeds

Actions:

- rerun or inspect the focused proof for the current `00204.c` runtime crash
- confirm idea 70's link-closure family remains cleared
- identify whether the next packet is primarily aggregate argument home
  selection, fixed-arity call setup, helper forwarding, or another tighter
  aggregate-call seam
- choose the narrowest protective proof that still reflects the owned runtime
  contract

Completion check:

- the next executor packet is narrowed to one idea-72-owned runtime seam with
  named proof targets and a clear boundary against ideas 70 and 71

## Step 2.1: Repair The Helper Pointer-Home Runtime Seam

Goal: restore the aggregate/helper pointer-home facts needed for the first
post-link helper copy or aggregate-return handoff to execute correctly.

Primary targets:

- the bounded same-module helper renderer and prepared local-slot surfaces
  that own `%ret.sret` and other pointer-param homes
- fixed-arity helper forwarding paths such as `fa_s*` / `fr_s*` only at the
  layer that materializes their incoming ABI homes

Actions:

- materialize pointer-kind stack-slot homes from the incoming ABI register
  where the prepared helper layer owns that fact
- include `%ret.sret` in the same pointer-home refresh path instead of
  treating it as a special excluded case
- prove the owned case advances beyond the bogus helper destination-pointer
  dereference without reopening earlier link-closure seams

Completion check:

- the targeted owned case no longer fails at the helper destination-pointer
  or aggregate-return pointer-home seam and instead executes further

## Step 2.2: Repair The Zero-Stack Call-Alignment Runtime Seam

Goal: restore the outgoing-call stack-parity shim on the prepared same-module
paths that become visible after the helper pointer-home seam is fixed.

Primary targets:

- the bounded same-module helper-call fast path
- the general same-module defined-call prepared path
- any direct-extern prepared zero-frame path only if the same parity rule is
  already shared there

Actions:

- emit the required 8-byte outgoing-call pad around zero-stack calls when the
  prepared frame parity requires it
- keep the repair in the x86 prepared-call ownership layer instead of adding
  testcase-shaped call-site exceptions
- prove the owned case advances beyond the `ret()` / `printf` alignment crash
  into either later execution or a narrower downstream runtime seam

Completion check:

- the targeted owned case no longer fails at the misaligned zero-stack call
  seam and instead executes further into a later runtime-only blocker

## Step 2.3: Classify And Repair The Newly Exposed Post-`ret()` Runtime Seam

Goal: decide whether the current `stdarg()` crash is still owned by idea 72
and, if so, repair the smallest remaining aggregate-call/runtime handoff seam.

Primary targets:

- the prepared variadic or aggregate-home tracking surfaces reached
  immediately after `ret()` returns
- the first invalid home/pointer/accounting load on the current `stdarg()`
  crash path
- idea-boundary evidence showing whether the seam remains aggregate-call
  runtime ownership or has graduated into idea 71's genuine variadic traversal

Actions:

- inspect the live `stdarg()` crash path and identify the first bad home,
  pointer, or accounting fact that feeds the invalid load
- keep the packet in idea 72 only if the missing fact still belongs to
  aggregate-call/runtime handoff semantics rather than later variadic
  traversal semantics
- repair that owned seam at its semantic owner, or explicitly route the case
  onward if the first failing seam is now a genuine idea-71 variadic leaf

Completion check:

- the next accepted packet either advances `00204.c` beyond the current
  `stdarg()` crash through an idea-72-owned repair or records an explicit
  runtime rehoming boundary out of idea 72

## Step 3: Prove Runtime-Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-72 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned case plus the nearest runtime
  coverage that protects the changed contract
- record in `todo.md` when the case graduates into a later wrong-result or
  other runtime-only leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-72 runtime family and keep
  downstream routing explicit

## Step 4: Continue The Loop Until Idea 72 Is Exhausted

Goal: repeat the Step 1 -> 3 loop until the remaining failures no longer
belong to this post-link aggregate-call runtime-correctness family.

Actions:

- keep idea 72 active only while owned cases still fail in this runtime family
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-72
  seam, or lifecycle state is ready to hand off because idea 72 no longer owns
  the remaining failures
