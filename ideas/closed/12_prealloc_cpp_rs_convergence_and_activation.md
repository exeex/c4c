# Prealloc Cpp Rs Convergence And Activation

## Goal

Converge the C++ prealloc pipeline toward the retained Rust reference
implementations under `src/backend/prealloc/*.rs`, starting with
`stack_layout`, then `liveness.cpp`, then `regalloc.cpp`, until the C++ path is
not just structurally similar but functionally active inside the current c4c
framework.

The end state is:

- each C++ phase has a clear one-to-one relationship with its Rust reference
- the migrated C++ implementations actually participate in the active backend
  pipeline
- targeted tests prove the migrated behavior is live under our framework rather
  than only present as dead or placeholder code

## Why This Is Separate

The current C++ prealloc files have started to take shape, but they are still a
mix of:

- provisional bridges
- retained Rust reference files
- partially aligned public contracts
- placeholder behavior that compiles without yet matching the intended phase
  semantics

That makes this a distinct initiative from ordinary backend bug-fixing. The
main work is semantic convergence and activation of a specific subsystem, not
isolated testcase repair.

Keeping this as a separate idea makes the ordering explicit:

1. stabilize and converge `stack_layout`
2. align `liveness.cpp` to the Rust analysis model
3. align `regalloc.cpp` to consume the liveness contract and really run
4. perform an explicit acceptance pass that checks one-to-one phase coverage

## Scope

### 1. `stack_layout`

- converge `src/backend/prealloc/stack_layout.cpp` and
  `src/backend/prealloc/stack_layout/*.cpp` toward the retained Rust references
  under `src/backend/prealloc/stack_layout/*.rs`
- make the active C++ stack-layout behavior materially match the Rust phase
  boundaries and intended data flow
- reduce purely provisional metadata-only behavior where it blocks real slot
  assignment semantics
- keep the retained `.rs` files as reference until the convergence pass and
  acceptance comparison are complete

### 2. `liveness.cpp`

- converge `src/backend/prealloc/liveness.cpp` toward
  `src/backend/prealloc/liveness.rs`
- make the public prepared-liveness contract support the information that
  regalloc actually needs
- prefer Rust-like program-point, interval, call-barrier, and loop-depth
  semantics over ad hoc placeholder summaries

### 3. `regalloc.cpp`

- implement and converge `src/backend/prealloc/regalloc.cpp` so it actually
  consumes the prepared liveness and stack-layout outputs
- align the C++ behavior to the retained Rust reference semantics as far as the
  surrounding BIR/prealloc design allows
- ensure the regalloc phase is active in the shared prealloc route rather than
  remaining a stub

### 4. Acceptance And Comparison

- add an explicit acceptance stage after the three convergence passes
- compare each active C++ file to its retained Rust reference file
- verify that functions, helper responsibilities, and major semantic behaviors
  have a one-to-one correspondence, or record the bounded intentional
  differences where the host framework requires adaptation
- treat missing functional coverage in the C++ file as incomplete work, not as
  documentation debt

### 5. Runtime Proof In Our Framework

- add tests or extend existing backend/prealloc tests so the migrated behavior
  is exercised inside c4c's actual pipeline
- prove that the new stack-layout, liveness, and regalloc logic is not just
  compiled but reached and behaviorally relevant
- prefer targeted, semantics-driven tests that demonstrate phase behavior under
  the c4c framework instead of narrow text-shape matching

## Core Rule

Do not accept superficial similarity.

This idea is only complete when the C++ phases both:

- closely correspond to the Rust reference behavior
- demonstrably run inside the active c4c pipeline with test coverage

Green compilation alone is not sufficient.

## Constraints

- preserve the current single active plan lifecycle; this idea should start as
  a durable open idea, not an implicit plan activation
- avoid testcase-overfit shortcuts that only make one migrated helper look
  active without implementing the surrounding phase semantics
- do not silently delete retained Rust reference files before the acceptance
  comparison pass is complete
- keep public contract changes in `src/backend/prealloc/prealloc.hpp`
  intentional and driven by the actual phase data flow
- prefer semantic tests over brittle IR text probes

## Acceptance Criteria

- [ ] `stack_layout.cpp` and sibling `stack_layout/*.cpp` materially correspond
      to the retained Rust reference behavior and are active in the pipeline
- [ ] `liveness.cpp` materially corresponds to `liveness.rs` and publishes the
      liveness data that `regalloc.cpp` actually consumes
- [ ] `regalloc.cpp` materially corresponds to its Rust reference intent and is
      no longer a stub
- [ ] each phase has an explicit C++ vs Rust comparison pass with one-to-one
      function/behavior coverage or bounded documented divergences
- [ ] tests demonstrate that the migrated phases execute and matter under the
      c4c framework
- [ ] the acceptance pass verifies both semantic convergence and runtime proof,
      not just file existence or successful build output

## Non-Goals

- deleting the retained Rust reference files before convergence is proven
- treating comment updates or README edits as phase convergence
- accepting placeholder no-op regalloc behavior because the surrounding build
  succeeds
- proving success only with one narrow testcase family while the migrated phase
  stays inactive for nearby shapes

## Good First Patch

Take `stack_layout` first: tighten the C++ stack-layout contract so it matches
the retained Rust phase responsibilities more closely, then add a bounded test
that proves the active C++ path changes prepared stack-layout output for a real
function shape inside the c4c framework.

## Closure Notes

- Closed on 2026-04-17 after the active C++ `stack_layout`, `liveness`, and
  `regalloc` routes were all runtime-proven in the backend pipeline and each
  phase gained explicit C++ vs Rust comparison coverage under
  `src/backend/prealloc/*_comparison.md`.
- Step 5 acceptance finished with bounded divergences called out explicitly
  instead of hidden in the code: retained Rust-only behavior still exists, but
  the active c4c route is no longer placeholder-driven.
- Close-time backend regression guard passed against the last plan checkpoint
  `bcefa36b`: `^backend_` improved from `70/70` passing tests to `71/71` with
  no new failures.
