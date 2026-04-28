# x86 Prepared Module Renderer Recovery Runbook

Status: Active
Source Idea: ideas/open/121_x86_prepared_module_renderer_recovery.md
Activated From: ideas/open/121_x86_prepared_module_renderer_recovery.md

## Purpose

Recover the x86 prepared-module rendering path on semantic grounds so x86
handoff and codegen tests can execute real scalar and control-flow assembly
again.

## Goal

Restore documented, tested x86 prepared-module rendering capability for
supported scalar and control-flow forms without testcase-shaped shortcuts.

## Core Rule

Do not prove x86 handoff by adding narrow pattern dispatch, instruction-count
assumptions, or fixture-shaped rendering; restore renderer capability through
general semantic lowering and nearby same-feature coverage.

## Read First

- `ideas/open/121_x86_prepared_module_renderer_recovery.md`
- `review/step5_x86_handoff_dirty_slice_review.md`
- `review/step4_current_x86_control_flow_route_review.md`
- `review/step3_accumulated_scalar_local_slot_route_review.md`
- `review/step3_accumulated_local_slot_renderer_review.md`
- `src/backend/mir/x86/module/module.cpp`
- Existing x86 prepared-module, handoff, and backend codegen tests.
- Current MIR, prepared-module, and target-specific x86 backend interfaces.

## Current Targets

- x86 prepared-module rendering entry points and helper boundaries.
- Missing or replaced headers and APIs that currently prevent x86 backend
  handoff tests from compiling.
- Scalar instruction rendering for supported x86 prepared-module forms.
- Control-flow rendering and prepared label consumption through the x86 path.
- Prepared producer identity publication for control-flow and parallel-copy
  records that the x86 consumer must validate.
- x86 backend BIR, handoff, and codegen tests that distinguish semantic
  renderer support, explicit unsupported boundaries, and fixture-shaped output.

## Non-Goals

- Do not add narrow pattern dispatch that only recognizes current tests.
- Do not weaken supported-path tests or rewrite expectations to hide missing
  renderer capability.
- Do not treat this plan as acceptance progress for idea 120 unless the active
  lifecycle state is explicitly switched back and the proof consumes prepared
  label ids directly.
- Do not broaden into unrelated target backend rewrites unless a blocker is
  recorded and the plan is deliberately revised.

## Working Model

- x86 renderer recovery is separate from raw-label fallback cleanup.
- Compile compatibility comes before behavior claims: handoff tests must build
  against current headers and backend APIs before they can prove execution.
- Scalar and control-flow support should be expressed as renderer rules over
  supported prepared-module constructs, not as named-case recognition.
- Prepared control-flow label proofs are valid only when the x86 path consumes
  prepared label ids directly and rejects missing or drifted ids.

## Execution Rules

- Start with inventory and compile-surface repair before changing renderer
  semantics.
- Prefer existing backend abstractions and helper APIs over introducing a new
  rendering layer.
- Keep every code-changing step paired with fresh build or compile proof and a
  narrow x86/backend test subset chosen by the supervisor.
- When a renderer form is restored, add or update nearby same-feature tests so
  one known handoff fixture is not the only proof.
- Keep stale x86 backend BIR/handoff tests in scope when they block acceptance:
  repair their API or harness drift so failures describe renderer capability,
  not obsolete test infrastructure.
- Treat expectation downgrades, unsupported-path rewrites, and fixture-shaped
  output matching as route drift.
- Stop the accumulated Step 3 local-slot route before further renderer growth:
  split Step 4 short-circuit/control-flow work away from scalar local-slot
  work, then either retire the boundary-specific scalar helper chain or
  replace it with a reusable prepared local-slot expression/statement renderer.
- Do not add another exact-sequence scalar helper for the red
  `minimal local-slot add-chain guard route` or the current
  `minimal local-slot i16/i64 subtract return route`; instruction counts,
  hard-coded instruction indexes, and fixture-shaped dispatch are blockers
  here.
- Treat the i16 widened arithmetic register-home mismatch identified in
  `review/step3_accumulated_local_slot_renderer_review.md` as a Step 3
  authority bug: emitted registers and memory operands must match prepared
  value homes and prepared memory accesses, not only prove that some home
  exists.
- Escalate to broader backend validation before the source idea is considered
  complete, because this path affects target codegen and prepared control flow.

## Ordered Steps

### Step 1: Inventory X86 Prepared Renderer And Compile Surface

Goal: map the current x86 prepared-module renderer state and identify the first
compile-compatible recovery packet.

Primary targets:
- `src/backend/mir/x86/module/module.cpp`
- x86 prepared-module and handoff test files.
- Headers and APIs referenced by existing x86 backend tests.
- Any current unsupported markers or skipped x86 prepared-module coverage.

Concrete actions:
- Inspect the current x86 prepared-module rendering entry points and helper
  structure.
- Identify missing headers, renamed APIs, or stale test harness assumptions
  that prevent x86 backend handoff tests from compiling.
- Classify existing x86 prepared-module tests as compile-blocked,
  renderer-blocked, control-flow-blocked, or already valid.
- Record the first narrow compile or test proof command in `todo.md` once the
  supervisor delegates execution.
- Do not change implementation files in this lifecycle step.

Completion check:
- `todo.md` identifies the first executable recovery packet, target files, the
  suspected compile or renderer blocker, and the narrow proof command.

### Step 2: Restore X86 Handoff Test Compile Compatibility

Goal: make the x86 backend handoff tests build against the current backend
interfaces without weakening supported-path contracts.

Primary targets:
- x86 backend handoff tests and their includes.
- x86 backend BIR tests that should express the supported/unsupported renderer
  matrix.
- Current prepared-module, MIR, and target x86 public interfaces.
- Test harness helpers used by backend codegen or handoff tests.

Concrete actions:
- Replace stale includes or helper calls with current APIs when the replacement
  route is available.
- Keep supported-path tests supported; do not mark them unsupported to get a
  green result.
- Preserve test intent while separating compile-harness repair from renderer
  behavior changes.
- Keep unsupported boundaries explicit in tests; do not silently skip stale
  x86 BIR coverage instead of repairing the harness or recording the blocker.
- Run the delegated build or narrow test proof after each bounded repair.

Completion check:
- The selected x86 BIR, handoff, or codegen test subset compiles far enough to
  expose real renderer behavior instead of stale interface failures, and any
  remaining unsupported forms are named boundaries rather than hidden skips.

### Step 3: Consolidate The Prepared Local-Slot Renderer Route

Goal: stop the accumulated Step 3 helper chain and recover scalar local-slot
work only through a reusable prepared expression/statement renderer, or park
unsupported subtract-return boundaries before another boundary-specific helper
is added.

Primary targets:
- x86 prepared-module renderer helpers in `src/backend/mir/x86/module/`.
- Scalar instruction selection or emission surfaces used by prepared-module
  tests.
- Dirty Step 4 short-circuit/control-flow changes currently mixed with scalar
  local-slot helpers.
- Nearby same-feature scalar tests.

Concrete actions:
- First split or retire the accumulated helper chain before adding any new
  scalar boundary:
  - keep Step 4 short-circuit/control-flow authority assertions and helpers out
    of the Step 3 scalar packet;
  - keep the already-green local-slot return, i32 guard, and i16 increment
    guard evidence as regression constraints, not as justification for another
    helper;
  - park the current `minimal local-slot i16/i64 subtract return route` as an
    explicit unsupported boundary unless it can be handled by the reusable
    prepared local-slot renderer in this step;
  - remove, merge, or retire helpers whose primary selection rule is current
    red-boundary shape instead of prepared statement/expression semantics.
- If scalar local-slot support continues, define one reusable prepared
  local-slot expression/statement renderer before touching any new red
  boundary:
  - consume prepared frame-slot ids, prepared memory accesses, prepared value
    homes, typed scalar operations, return moves, and branch conditions through
    their semantic records;
  - render stores, loads, casts, binary expressions, statement sequencing,
    returns, and branch predicates from those records rather than from a known
    testcase sequence;
  - validate emitted register names, memory operands, widths, and access
    authorities against the prepared records, including widened i16 arithmetic
    homes whose prepared register must match the emitted register;
  - support only the scalar widths and operations that the current backend can
    prove semantically, and record explicit unsupported boundaries for the rest.
- Add required negative coverage with the generalized route:
  - missing or drifted prepared memory access for each used statement;
  - missing or drifted frame-slot id and wrong access size;
  - divergent load/store access authority;
  - missing value homes for store, load, cast, binary, and return carriers;
  - prepared register-home drift for widened arithmetic results and return
    carriers;
  - return source/destination drift;
  - branch-condition missing or drifted identity for guard-lane expressions.
- Do not claim progress by making only the red
  `minimal local-slot i16/i64 subtract return route` or
  `minimal local-slot add-chain guard route` pass; nearby add/sub guard-lane
  and return cases must prove the same semantic renderer path.
- Keep raw label and Step 4 control-flow authority work out of this step unless
  the supervisor deliberately delegates a separate Step 4 packet.

Completion check:
- The dirty implementation has been split or retired so Step 3 contains only a
  coherent scalar local-slot route; the remaining scalar route is generalized
  over prepared local-slot expression/statement records, validates emitted
  operands against prepared homes/accesses, includes the required negative
  coverage, and the delegated x86 handoff subset is green. If this cannot be
  achieved without boundary-specific dispatch, record the subtract-return
  boundary as unsupported and stop for plan review instead of adding another
  helper chain.

### Step 4: Recover Prepared Control-Flow Rendering Semantics

Goal: restore control-flow rendering, including branches and labels, without
falling back to raw or drifted identity. If the x86 consumer reaches missing
or drifted prepared identity, repair the prepared producer records before
adding any x86-side acceptance path.

Primary targets:
- Prepared-module producer records for control-flow block identity,
  branch-owned carrier or bridge blocks, and out-of-SSA parallel-copy bundles.
- x86 control-flow rendering in the prepared-module path.
- Prepared label id consumption and validation boundaries.
- Branch, conditional branch, and label-target tests for the x86 backend.

Concrete actions:
- Treat the current blocker as a producer publication gap inside this plan:
  the next Step 4 packet must publish explicit prepared identity for mutated
  or bridge carrier blocks, or document a semantic unsupported boundary before
  returning to x86 rendering.
- Publish or preserve enough prepared identity for the consumer to tie every
  live control-flow block and authoritative parallel-copy bundle to the exact
  prepared edge or branch metadata it represents.
- Route control-flow emission through prepared label identity where available.
- Reject or surface missing and drifted label ids rather than recovering
  through broad raw-string matching, compare-join-specific validator
  exceptions, or same-successor parallel-copy guesses.
- Cover nearby branch and conditional-branch cases, not only the handoff case
  that exposed the drift.
- Add same-feature negative coverage for missing, drifted, and ambiguous
  prepared identities before treating the Step 4 route as progress.
- Do not add or accept standalone scalar local-slot renderers in this step.
  Local-slot code may remain in Step 4 only when it is inseparable from a
  prepared branch, label, branch-plan, continuation, or parallel-copy control
  flow proof. Route no-branch local-slot return, i16 increment, and other
  scalar-width lowering through Step 3 or record an explicit unsupported
  boundary before continuing Step 4.
- Keep scalar rendering assumptions explicit so control-flow proof does not
  hide unrelated renderer gaps.

Completion check:
- Prepared producer records publish the identity needed by x86 control-flow
  rendering; x86 prepared control-flow cases consume prepared label ids
  directly, reject invalid identity where appropriate, and pass the delegated
  narrow proof without consumer-side raw-label or missing-identity escapes.

### Step 5: Reprove X86 Handoff And Decide Lifecycle Outcome

Goal: determine whether the recovered renderer now satisfies the source idea
and produces a reusable handoff proof for later raw-label cleanup work.

Primary targets:
- x86 handoff/codegen tests that exercise real scalar and control-flow asm.
- x86 backend BIR tests that define the supported/unsupported renderer matrix.
- Prepared label id validation tests.
- Broader backend validation selected by the supervisor.

Concrete actions:
- Run the supervisor-selected narrow x86 handoff proof after scalar and
  control-flow recovery.
- Verify the proof consumes prepared label ids directly and rejects missing or
  drifted ids.
- Verify x86 backend BIR/handoff tests are usable acceptance surfaces: supported
  forms run as supported, unsupported forms are explicit, and closure does not
  depend on a single handoff fixture.
- Run the required broader backend validation before requesting closure.
- Record any remaining renderer gaps in `todo.md`; if they are durable source
  scope, ask the plan owner to revise or split the plan instead of closing.

Completion check:
- The source idea acceptance criteria are met, x86 backend BIR/handoff tests
  provide a usable supported/unsupported acceptance surface, broader validation
  is recorded, and the supervisor has enough evidence to either close the idea
  or request a focused lifecycle revision.
