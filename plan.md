# X86 Backend Local-Memory Semantic Family Runbook

Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Activated from: ideas/open/54_x86_backend_c_testsuite_capability_families.md

## Purpose

The prepared-module frontier re-baseline runbook is exhausted. The bounded
`00131` / `00211` / `00210` lane remains admitted, and the adjacent
`00189` global-function-pointer plus indirect variadic-runtime path remains an
explicit unsupported boundary. Return the active runbook to the broader source
idea's next honest family: semantic local-memory lowering.

## Goal

Land one bounded local-memory capability lane through semantic `lir_to_bir`
and the prepared x86 handoff, then prove it across a small same-family external
C cluster without widening into indirect-call or unrelated control-flow work.

## Core Rule

Keep progress explained by shared `alloca` / `gep` / `load` / `store`
capability growth. Do not weaken `x86_backend` expectations, do not accept
fallback LLVM IR, and do not add testcase-named or rendered-text recognizers.

## Read First

- [ideas/open/54_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/54_x86_backend_c_testsuite_capability_families.md)
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

## Current Targets

- one bounded straight-line or otherwise already-supported-control-flow
  local-memory lane
- stack-object creation plus constant-offset local address formation
- local loads and stores through that bounded address path
- a small same-family `c_testsuite_x86_backend_*` proving cluster to be named
  explicitly before implementation starts
- preservation of the already-admitted prepared-module lane for `00131`,
  `00211`, and `00210` as regression baseline coverage

## Non-Goals

- widening the `00210` admission into `00189` indirect/global-function-pointer
  support
- treating indirect/runtime adjacency as the next family just because it is
  nearby
- mixing emitter/control-flow families such as `00057` or scalar-control-flow
  families such as `00124` into this runbook
- claiming progress through expectation downgrades, testcase shortcuts, or
  rendered-text probes

## Working Model

- The linked source idea remains open.
- The previous prepared-module checkpoint stopped honestly: `00131`, `00211`,
  and `00210` stay admitted, while `00189` still marks the adjacent indirect
  prepared-module boundary.
- The source idea's dominant remaining route is still the semantic
  local-memory family, not a forced expansion of the prepared-module boundary.
- The next coherent slice must name one bounded local-memory lane and a small
  proving cluster before any implementation work starts.

## Execution Rules

- Keep packet churn in `todo.md`; rewrite `plan.md` again only for a genuine
  route checkpoint.
- Step 1 must name the local-memory proving cluster explicitly before Step 3
  implementation starts.
- Validation ladder per packet: build, narrow backend proof, chosen
  same-family `c_testsuite_x86_backend_*` probes, then `x86_backend`
  checkpoint once a coherent slice exists.
- Preserve the admitted `00131` / `00211` / `00210` prepared-module lane as a
  regression baseline while local-memory work proceeds.
- If Step 1 shows that another family is more honest than the expected
  local-memory lane, stop and record that route note explicitly instead of
  mutating the runbook ad hoc.

## Step 1. Re-Baseline The Local-Memory Frontier

Goal: Determine the smallest honest local-memory capability cluster from the
current `x86_backend` failure surface.

Primary targets:
- current `x86_backend` checkpoint evidence
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

Actions:
- inspect the current failure surface for cases blocked by `alloca`, `gep`,
  `load`, or `store` local-memory semantics
- separate that route from prepared-module indirect/runtime neighbors such as
  `00189` and unrelated emitter/control-flow neighbors such as `00057` and
  `00124`
- choose a small same-family external-C proving cluster whose members should
  move together if one bounded local-memory lane lands
- record the adjacent out-of-scope neighbors explicitly before implementation

Completion check:
- one local-memory candidate cluster and its out-of-scope neighbors are named
  clearly without testcase-by-testcase repair framing

## Step 2. Name The Bounded Local-Memory Lane

Goal: Turn the Step 1 evidence into one implementation contract.

Actions:
- choose exactly one local-memory lane such as stack-object creation,
  constant-offset address formation, and bounded local load/store traffic
- record the intended proving cluster for that lane
- make the local-memory non-goals explicit before implementation begins
- keep the prepared-module regression baseline separate from the new proving
  cluster

Completion check:
- one bounded local-memory lane and one proving cluster are named clearly
  enough for a later implementation packet

## Step 3. Extend The Semantic Lowering Honestly

Goal: Implement the smallest shared `lir_to_bir` growth required by the chosen
local-memory lane.

Primary targets:
- [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp)
- [src/backend/bir/lir_to_bir.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir.cpp)
- [src/backend/bir/lir_to_bir.hpp](/workspaces/c4c/src/backend/bir/lir_to_bir.hpp)

Actions:
- touch only the canonical semantic lowering surfaces needed by the chosen
  lane
- reuse shared local-memory lowering paths instead of adding testcase-shaped
  matching
- keep prepared-module indirect-call work and unrelated control-flow work out
  of scope
- stop and report if the route would require expectation downgrades or a named
  testcase shortcut

Completion check:
- the chosen lane advances through shared local-memory semantics rather than
  one named testcase

## Step 4. Keep The Boundary Truthful

Goal: Describe the newly supported local-memory family and adjacent
unsupported boundaries honestly.

Primary targets:
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)

Actions:
- revise backend notes and handoff coverage to describe the admitted
  local-memory lane by capability
- keep adjacent unsupported boundaries explicit when they remain outside the
  lane
- preserve the existing prepared-module `00189` boundary note while proving
  the local-memory route

Completion check:
- backend notes and handoff tests describe the supported local-memory family
  and nearby unsupported boundaries truthfully

## Step 5. Prove Nearby Same-Family Cases

Goal: Show the slice improved a local-memory capability family instead of one
probe.

Actions:
- run the narrow backend tests for the touched semantic boundary
- run the chosen same-family `c_testsuite_x86_backend_*` probes
- re-run a narrow prepared-module regression spot-check so the admitted
  `00131` / `00211` / `00210` lane stays explicit while `00189` remains out of
  scope
- once a coherent slice exists, run the `x86_backend` checkpoint to measure
  the truthful pass-count effect

Completion check:
- the local-memory proving cluster moves together or the packet stops with an
  explicit family blocker instead of pretending success
