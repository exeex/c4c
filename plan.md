# Post-Asm Global-Function-Pointer And Variadic-Runtime Link Closure

Status: Active
Source Idea: ideas/open/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md
Activated from: ideas/closed/69_long_double_aggregate_asm_emission_for_x86_backend.md

## Purpose

Switch the active backend leaf to the first downstream family that remains once
`00204.c` clears both prepared-module rejection and assembler-invalid
long-double aggregate emission.

## Goal

Repair one post-assembly global-function-pointer / indirect variadic-runtime
closure seam at a time so owned cases progress from assembly into successful
link or an even later runtime-only leaf.

## Core Rule

Do not claim progress through testcase-shaped symbol or helper exceptions when
the missing ownership is still a generic post-assembly closure rule for
same-module symbols, variadic-runtime references, or their truthful boundary
contract.

## Read First

- `ideas/open/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md`
- `ideas/closed/69_long_double_aggregate_asm_emission_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_rejection_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

## Scope

- x86 backend failures that already assemble but still fail on the
  global-function-pointer / indirect variadic-runtime family afterward
- truthful ownership between the accepted prepared-module route and later
  same-module symbol or variadic-runtime closure obligations
- explicit routing for cases that advance past link closure into later runtime
  correctness work

## Non-Goals

- reopening prepared-module rejection ownership that idea 61 already cleared
- reopening assembler-invalid long-double aggregate emission from idea 69
- downgrading boundary coverage to match stale rejection wording
- claiming runtime correctness progress before the owned cases link

## Working Model

- keep one post-assembly closure seam per packet
- use `c_testsuite_x86_backend_src_00204_c` plus the nearest boundary coverage
  that protects the same post-assembly contract
- prefer canonical ownership for same-module symbols and variadic-runtime
  references over testcase-named link hacks
- route cases out immediately once they link and the next real blocker belongs
  to a later runtime-only leaf

## Execution Rules

- prefer one post-assembly closure seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on the owned `00204.c` route plus the nearest boundary coverage
  for the changed contract
- reject testcase-shaped symbol-definition or helper-name shortcuts that only
  unblock `00204.c`

## Step 1: Confirm Downstream Ownership And Narrow The Post-Asm Closure Seam

Goal: confirm the first remaining `00204.c` blocker belongs to this
post-assembly family and isolate the exact closure seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- `backend_x86_handoff_boundary`
- x86 emit or lowering surfaces that still own same-module symbol or
  variadic-runtime closure after assembly succeeds

Actions:

- rerun or inspect the focused proof that reproduces the current post-assembly
  failures
- confirm the old idea-69 asm-emission seam stays cleared and no longer owns
  the top-level failure
- identify whether the next packet is primarily a truthful boundary-contract
  update, same-module symbol-definition closure, or variadic-runtime reference
  closure
- choose the nearest protective coverage that proves this seam without relying
  only on `00204.c`

Completion check:

- the next executor packet is narrowed to one idea-70-owned post-assembly seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Post-Asm Closure Seam

Goal: implement the smallest durable repair that advances the selected owned
case beyond the current post-assembly blocker.

Primary targets:

- the boundary or codegen surfaces that own the selected post-assembly seam
- shared helpers only if the current layer lacks a needed generic fact

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer canonical closure or contract fixes over ad hoc testcase or
  symbol-name branches
- keep the repair generic across nearby same-module global-function-pointer and
  variadic-runtime routes
- confirm the targeted case now links or graduates cleanly into a later
  runtime-only leaf

Completion check:

- the targeted owned case no longer fails for the selected idea-70
  post-assembly seam and instead reaches the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-70 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned case plus the nearest boundary
  coverage that protects the changed post-assembly contract
- record in `todo.md` when advanced cases now belong in a later runtime leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-70 family and preserve clear
  routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 70 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 70.

Actions:

- keep idea 70 active only while cases still fail in this post-assembly
  global-function-pointer / indirect variadic-runtime closure family
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-70
  seam, or lifecycle state is ready to hand off because idea 70 no longer owns
  the remaining failures
