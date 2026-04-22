# Long-Double Aggregate Asm Emission

Status: Active
Source Idea: ideas/open/69_long_double_aggregate_asm_emission_for_x86_backend.md
Activated from: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md

## Purpose

Switch the active backend leaf to the downstream long-double aggregate
asm-emission seam now that `c_testsuite_x86_backend_src_00204_c` has advanced
past idea 61's bounded multi-function prepared-module family.

## Goal

Repair one long-double aggregate asm-emission seam at a time so owned cases
assemble successfully without adding testcase-shaped x87 or aggregate render
shortcuts.

## Core Rule

Do not claim progress through one more named-case mnemonic rewrite when the
missing ownership is still a generic long-double load/store or aggregate
rendering plan that x86 should emit canonically.

## Read First

- `ideas/open/69_long_double_aggregate_asm_emission_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that reach final x86 assembly and then fail because the
  emitted long-double aggregate path uses assembler-invalid instruction forms
- ownership between prepared value/home facts and the final x87 or long-double
  render lane when the route already clears prepared-module and local-slot
  handoff
- durable rehoming of cases that advance from asm-emission failure into later
  runtime-correctness ownership

## Non-Goals

- reopening prepared-module, call-bundle, or local-slot ownership that is
  already cleared for the current route
- rewriting long-double handling through one more testcase-named x86 fast path
- treating stale route-debug or trace expectations as the main work item
- claiming runtime correctness progress before assembly succeeds

## Working Model

- keep one long-double aggregate asm-emission seam per packet
- use the nearest owned c-testsuite case plus the nearest backend coverage
  that protects the same render lane without collapsing the packet into one
  testcase
- prefer canonical render-plan fixes over mnemonic-string patching when the
  prepared facts already identify the operation
- route cases out immediately once they assemble and the next real blocker
  belongs to runtime correctness or another downstream leaf

## Execution Rules

- prefer one assembler-rejected long-double render seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on the owned asm-emission failure plus the nearest backend
  coverage that protects the changed long-double render surface
- reject testcase-shaped instruction spelling branches that only unblock
  `00204.c`

## Step 1: Confirm Downstream Ownership And Narrow The Long-Double Render Seam

Goal: confirm that `c_testsuite_x86_backend_src_00204_c` is now blocked by an
idea-69-owned long-double aggregate asm-emission seam and isolate the exact
render surface.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- x86 codegen files that emit long-double load/store or aggregate instructions
- backend coverage nearest the current long-double render route

Actions:

- rerun or inspect the focused proof that reproduces the assembler rejection
- confirm the old idea-61 module/call family now matches and no longer owns
  the top-level failure
- identify the exact long-double aggregate render path that emits invalid
  `fldt` / `fstpt` forms and note whether the missing meaning is render-plan
  normalization or plain instruction spelling
- choose the nearest protective coverage that can prove this seam without
  relying only on `00204.c`

Completion check:

- the next executor packet is narrowed to one idea-69-owned asm-emission seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Long-Double Aggregate Asm-Emission Seam

Goal: implement the smallest durable render-path repair that advances the
selected owned case beyond the current assembler rejection.

Primary targets:

- x86 long-double or x87 render helpers that own the current invalid emission
- shared prepared helpers only if the render lane lacks a needed generic fact

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer canonical render-plan fixes over ad hoc testcase mnemonic branches
- keep the repair generic across nearby long-double aggregate routes
- confirm the targeted case now assembles or graduates cleanly into a later
  downstream leaf

Completion check:

- the targeted owned case no longer fails for the selected idea-69
  asm-emission seam and instead reaches the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-69 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned case plus the nearest backend
  coverage that protects the changed long-double render path
- record in `todo.md` when advanced cases now belong in idea 63 or another
  downstream leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-69 family and preserve clear
  routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 69 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 69.

Actions:

- keep idea 69 active only while cases still fail because emitted long-double
  aggregate asm is rejected before execution starts
- use `todo.md` to preserve which cases graduate into later leaves
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-69
  seam, or lifecycle state is ready to hand off because idea 69 no longer owns
  the remaining failures
