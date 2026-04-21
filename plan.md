# Backend Trace And Error Contract For X86 Handoff

Status: Active
Source Idea: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Activated from: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md

## Purpose

Make backend debugging for prepared x86 handoff cases readable enough that
large failures such as `tests/c/external/c-testsuite/src/00204.c` can be
diagnosed from stable CLI output instead of local instrumentation or matcher
probing.

## Goal

Turn `--dump-bir -> --dump-prepared-bir -> --dump-mir/--trace-mir` into one
coherent debug ladder with actionable rejection diagnostics at the x86 route
boundary.

## Core Rule

Do not claim progress by adding testcase-shaped logging or one-off debug
prints. Improve the stable backend debug surface and contract diagnostics that
generalize across prepared-handoff cases.

## Read First

- `ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `src/apps/c4cll.cpp`
- `src/backend/bir/`
- `src/backend/prep/`
- `src/backend/mir/x86/`
- `tests/backend/backend_codegen_route_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- CLI-visible backend debug surfaces for semantic BIR, prepared BIR, and x86
  handoff tracing
- backend-owned route and contract diagnostics for meaningful x86 rejection
  sites
- debug output changes that help a developer decide whether to inspect
  semantic BIR, prepared BIR, or x86 route consumption next

## Non-Goals

- repairing one specific x86 capability family as the completion signal
- testcase-specific debug prints or named-case trace lanes
- reopening unrelated frontend, HIR, or semantic-lowering ownership
- expanding matcher coverage as a substitute for better backend diagnostics

## Working Model

- treat `00204.c` and its nearest reductions as observability proof cases, not
  as the design boundary
- tighten the existing CLI flags before inventing parallel debug entry points
- distinguish ordinary route misses from unsupported shapes, missing prepared
  contracts, and impossible backend invariants
- keep output readable enough that large cases stay inspectable without local
  code edits

## Execution Rules

- prefer one observability seam per packet: debug-ladder coherence, prepared
  delta summary, route diagnostics, or focus controls
- keep proof on the nearest backend tests plus one motivating x86 handoff case
- update `todo.md`, not this file, for routine packet progress
- reject changes whose main effect is emitting more text without improving
  route meaning or next-step guidance

## Step 1: Baseline The Current Debug Ladder Against `00204.c`

Goal: confirm what the current flags already explain well and isolate the
highest-value information gaps that still block backend debugging on large
cases.

Primary targets:

- `tests/c/external/c-testsuite/src/00204.c`
- the nearest reduced backend route coverage
- current outputs from `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and
  `--trace-mir`

Actions:

- run the current debug ladder on `00204.c` or the nearest reduction that
  preserves the same observability problem
- record which questions are already answered at each stage and which still
  require local instrumentation or code reading
- isolate the next packet to one concrete observability gap, such as
  prepared-delta readability, final rejection clarity, or focus/filtering for
  large cases

Completion check:

- the next executor packet names one concrete debug-surface gap with a proof
  case and a clear stage boundary

## Step 2.1: Make `--dump-prepared-bir` Explain What Changed

Goal: expose the meaningful BIR-to-prepared deltas before verbose backend
detail so a developer can see what prepare contributed to x86 consumption.

Primary targets:

- `src/backend/prep/`
- `src/apps/c4cll.cpp`
- backend tests that lock the prepared dump contract

Actions:

- add or refine concise summary output for prepared-stage deltas such as phi
  removal, branch-condition legalizations, join-transfer rewrites, stack-slot
  introduction, and value-home or move-bundle formation when those facts
  exist
- keep verbose object or allocator detail available without making it the
  default first signal
- prove the new summary against the nearest backend dump coverage and the
  motivating large-case route

Completion check:

- `--dump-prepared-bir` shows the key prepare-stage facts a developer needs
  before reading the full prepared IR body

## Step 2.2: Make X86 Rejection Diagnostics Plain And Actionable

Goal: ensure the final meaningful x86 rejection identifies the missing or
  unsupported contract in plain language and points to the next inspection
  surface.

Primary targets:

- `src/backend/mir/x86/`
- backend route/trace helpers and diagnostics
- tests covering x86 handoff summaries and traces

Actions:

- replace opaque final meaningful `std::nullopt` exits at the owned x86 route
  boundary with backend-owned diagnostics that distinguish route miss,
  unsupported shape, missing prepared fact, and impossible invariant
- make `--dump-mir` and `--trace-mir` name the rejected route, the blocking
  prepared concept, and the next thing to inspect
- keep ordinary cheap probe failures lightweight where no durable debugging
  information is lost

Completion check:

- a developer can run one stable x86 trace command and understand why the
  route failed plus what to inspect next

## Step 2.3: Add Focus Controls For Large Cases

Goal: keep backend trace output usable on `00204.c`-scale cases without ad hoc
local logging.

Primary targets:

- CLI/debug option plumbing in `src/apps/c4cll.cpp`
- backend trace sinks under `src/backend/mir/x86/`
- tests that lock focused trace behavior

Actions:

- add stable focus filtering by function, block, value, or equivalent narrow
  backend concept where that filtering materially reduces search space
- ensure filtered output still preserves the final meaningful rejection and
  enough surrounding context to interpret it
- prove the focused route on a large case and the nearest reduced coverage

Completion check:

- large-case tracing can be narrowed without code edits and still preserves the
  decisive route information

## Step 2.4: Prove The Debug Ladder Is Coherent

Goal: show the accepted Step 2 packets behave like one deliberate backend
debug workflow instead of unrelated dumps.

Actions:

- verify a developer can answer, in order: what semantic lowering produced,
  what prepare changed, which x86 route rejected, what class of failure it
  was, and what to inspect next
- refresh backend tests and the motivating route proof so the CLI contract is
  stable
- record in `todo.md` any remaining observability gaps that belong to a later
  packet inside idea 67

Completion check:

- the current debug surfaces behave as one coherent ladder on the motivating
  case and nearby backend coverage

## Step 3: Continue Until Backend Handoff Debugging Stops Requiring Local Instrumentation

Goal: keep repeating Step 1 -> 2.4 until x86 handoff debugging no longer
depends on ad hoc local logging or matcher poking just to learn where a route
died.

Actions:

- keep idea 67 active while meaningful x86 route failures still require local
  instrumentation to explain
- call lifecycle review again if the next observability gap becomes oversized
  or a separate initiative emerges

Completion check:

- backend handoff debugging is readable, actionable, and stable enough that
  `00204.c`-scale failures can be diagnosed from the supported CLI surface
