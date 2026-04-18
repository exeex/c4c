# X86 Backend Next Capability-Family Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

Repair the active runbook after the delegated proof rerun showed that the
chosen `00210` lane is real, but the earlier direct-call framing was wrong:
its honest boundary crosses from the x86 prepared-module consumer into
prepared-BIR pointer-arg provenance plus bounded indirect-callee identity for
same-module function-pointer calls.

## Goal

Land one bounded slice for the `00210`-anchored same-module
function-pointer indirect-call multi-function prepared-module lane, with the
required string/global-address provenance made explicit, without expectation
weakening or testcase overfit.

## Core Rule

Keep progress explained by shared backend capability growth. Do not weaken
`x86_backend` expectations, do not reintroduce fallback LLVM IR acceptance,
and do not add testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/globals.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/globals.cpp)
- [tests/c/external/c-testsuite/src/00045.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00045.c)
- [tests/c/external/c-testsuite/src/00051.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00051.c)
- [tests/c/external/c-testsuite/src/00189.c](/workspaces/c4c/tests/c/external/c-testsuite/src/00189.c)

## Current Targets

- Start from the blocker notes recorded in `todo.md`, not the earlier
  `00189`-anchored assumption that the next lane was x86-handoff-only work.
- Treat `c_testsuite_x86_backend_src_00210_c` as the current proving anchor
  for a bounded same-module function-pointer indirect-call multi-function
  prepared-module lane.
- Include the prepared-BIR or lowering surface required to preserve
  string/global-address provenance for external call pointer args plus the
  bounded callee identity needed when same-module function-pointer calls
  arrive indirect in prepared BIR.
- Keep adjacent families explicit when they are not required by this lane:
  `00189` remains the broader
  indirect-call/global-function-pointer/variadic-runtime neighbor, while
  `00057` and `00124` remain emitter/control-flow and scalar-control-flow
  neighbors.

## Non-Goals

- Reopening the completed pointer-backed same-module global lane as routine
  follow-up.
- Hiding a route change inside an implementation packet.
- Treating `00210` as a one-line `functions.size() != 1` relaxation inside the
  x86 handoff.
- Mixing prepared-BIR provenance work, indirect/runtime plumbing, bootstrap
  scalar globals, and multi-block control flow into one packet without the plan
  explicitly justifying that boundary.
- Accepting fallback LLVM IR or weakening supported-path expectations.
- Adding testcase-shaped helpers or recognizers keyed to rendered output.

## Working Model

- The current source idea remains open, but the prior runbook is exhausted.
- Step 1-2 route selection is complete enough to name one honest current lane:
  `00210`'s bounded same-module function-pointer indirect-call
  multi-function prepared-module path.
- The blocked executor packets showed that this lane is not satisfied by x86
  handoff work alone because the prepared module still needs both
  string/global-address provenance at the external `printf` call sites and a
  bounded same-module indirect-callee representation for the
  `function_pointer -> actual_function` calls.
- The next coherent packet therefore spans prepared-BIR provenance or call
  classification plus the canonical x86 prepared-module consumer, while
  keeping broader indirect-call, global-function-pointer, and
  variadic-runtime plumbing out of scope.
- If the next honest family expands further than bounded same-module indirect
  calls plus pointer-address provenance, stop and record that lifecycle
  blocker instead of silently broadening again.

## Execution Rules

- Step 1 must name one family, one proving cluster, and the nearby out-of-scope
  shapes before implementation starts.
- Keep packet churn in `todo.md`; use `plan.md` only for genuine route
  checkpoint changes.
- Validation ladder per packet: build, narrow backend proof, selected
  same-family `c_testsuite_x86_backend_*` probes, then `x86_backend`
  checkpoint once a coherent slice exists.
- If Step 1 shows a distinct initiative that should live outside this source
  idea, stop and record that lifecycle blocker instead of mutating the plan ad
  hoc.
- Do not delegate a packet that claims `00210` as x86-only handoff work unless
  it also names how pointer-address provenance and bounded same-module
  indirect-callee identity are preserved into the prepared module.

## Step 1. Re-Baseline The Remaining Frontier

Goal: Group the post-pointer-backed failures into credible next-family
candidates before choosing new code work.

Primary targets:
- current `x86_backend` checkpoint evidence
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- inspect the current failure surface after the completed pointer-backed lane
- cluster remaining failures by one dominant capability blocker rather than by
  testcase name
- separate neighboring families such as bootstrap scalar globals,
  multi-function prepared-module routes, and multi-block control flow instead
  of merging them prematurely
- stop and record a route note if the best next family no longer fits the
  active source-idea path

Completion check:
- one or two credible next-family candidates are named with clear evidence and
  without testcase-by-testcase repair framing

## Step 2. Name The Next Bounded Family

Goal: Choose exactly one next capability family and its proving cluster.

Actions:
- select the smallest family whose nearby probes should move together if one
  shared capability is widened honestly
- record the intended proving cluster and the nearby out-of-scope neighbors
- make explicit whether the chosen route stays at the prepared x86
  handoff/emitter boundary or needs broader semantic work
- current checkpoint: the chosen family is the `00210`-anchored bounded
  same-module function-pointer indirect-call multi-function prepared-module
  lane, and it needs prepared-BIR string/global-address provenance plus
  bounded indirect-callee identity in addition to x86 handoff work

Completion check:
- one bounded family and one proving cluster are named clearly enough for a
  later implementation packet

## Step 3. Extend The Chosen Family Honestly

Goal: Implement the smallest shared capability widening required by the chosen
family across prepared-BIR provenance or call classification plus the x86
prepared-module consumer.

Primary targets:
- implementation files selected by Step 2's family choice, including the
  prepared-BIR or lowering provenance surface and the canonical x86
  prepared-module handoff

Actions:
- preserve enough address-origin information for pointer args that denote
  string/global symbols at external call sites
- preserve or recover the bounded same-module indirect-callee identity needed
  when `00210`-style function-pointer calls still arrive indirect in prepared
  BIR
- change shared lowering or backend logic only where the chosen family
  requires it
- reuse shared indirect-call/direct-call plus symbol/data emission paths
  instead of adding testcase-shaped prepared-module matchers
- keep adjacent families unsupported when they are not part of the named lane,
  especially broader `00189`-style
  indirect/global-function-pointer/variadic-runtime plumbing
- do not use expectation downgrades, testcase-named shortcuts, or rendered-text
  matching as proof of progress

Completion check:
- the chosen family is admitted through shared provenance plus bounded
  same-module indirect-callee support and shared codegen logic rather than
  testcase recognition

## Step 4. Keep The Boundary Truthful

Goal: Describe the newly supported family and the nearby unsupported boundary
explicitly.

Primary targets:
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- revise backend notes and handoff coverage to describe the chosen family by
  capability
- keep nearby unsupported neighbors explicit when they remain outside the lane,
  especially the `00189` indirect/global-function-pointer/variadic-runtime
  family
- ensure the tests still prove honest prepared-module or lowering boundaries
  rather than adapter growth

Completion check:
- backend notes and handoff tests describe the supported family and the nearby
  unsupported boundary truthfully

## Step 5. Prove Nearby Same-Family Cases

Goal: Show the slice improved a capability family instead of one probe.

Actions:
- run the narrow backend tests for the touched boundary
- run the chosen same-family `c_testsuite_x86_backend_*` probes without
  re-pairing `00189` unless the packet explicitly widens into that adjacent
  family
- once a coherent slice exists, run the `x86_backend` checkpoint to measure the
  truthful pass-count effect

Completion check:
- the proving cluster moves together or the packet stops with an explicit
  family blocker instead of pretending success
