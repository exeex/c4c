# Backend Trace And Error Contract For X86 Handoff

Status: Open
Created: 2026-04-21
Last-Updated: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Give backend bring-up its own readable trace and error-contract system so
stalled x86 prepared-handoff cases can be debugged from backend facts instead
of from silent `std::nullopt` exits or ad hoc temporary logging.

This idea exists because current backend investigation is too expensive:

- semantic and prepared BIR dumps can now show what facts exist
- but x86 consumption still often fails through many silent "not supported"
  paths
- and those paths usually return `std::nullopt` without an authoritative
  reason string or a structured route trace

The result is repeated packet churn, route ambiguity, and overfit risk because
the cheapest way to learn "why it failed" is often to patch local matcher code
until something moves.

## Current Surface

The repo now has a real first-pass backend debug surface in `c4cll`, even if
it is not yet the full finished tool this idea wants:

- `--dump-bir`:
  semantic backend BIR facts
- `--dump-prepared-bir`:
  prepared backend BIR plus phase metadata such as legalize, stack layout,
  liveness, and regalloc output
- `--dump-mir`:
  concise x86 handoff summary
- `--trace-mir`:
  lane-by-lane x86 handoff trace

These surfaces are already useful because they answer different questions:

- `--dump-bir`:
  did semantic lowering form the expected CFG, phi, branch, call, and value
  shape?
- `--dump-prepared-bir`:
  what changed after prepare, and what backend-owned facts now exist for x86
  consumption?
- `--dump-mir`:
  which top-level x86 lane matched or failed?
- `--trace-mir`:
  which lanes were tried, and where did rejection happen?

This means the idea no longer starts from zero. It should now focus on making
that surface sufficient for large-case debugging instead of inventing a second
parallel debug path.

## Problem Statement

The backend currently lacks two things that the frontend effectively already
has in spirit:

1. a scoped, readable route-trace surface for "what did the backend try, and
   where did it reject?"
2. a backend-owned contract/error system for "this path is impossible,
   unsupported, or missing required prepared facts"

Today many backend decision points collapse into:

- `return std::nullopt`
- generic `std::invalid_argument(...)`
- temporary local `stderr` prints

That is too weak for large cases such as `00204.c`, where the main blocker is
not "are facts present?" but "which x86 consumer rejected which prepared
contract, and for what exact reason?"

Observed with the current surface:

- the stage split itself is useful:
  `dump-bir -> dump-prepared-bir -> dump-mir/trace-mir` is the right
  debugging route
- but large prepared dumps still make the important signal hard to isolate
- and rejection text is often still too internal-jargon-heavy to tell a
  developer what to inspect next

## Primary Goals

### 1. Add A Readable Backend Trace Tool

The backend should provide a first-class trace mode for x86 prepared-handoff
investigation.

That trace should be readable by humans without backend-internal guesswork.

Preferred properties:

- function-scoped and block-scoped trace nesting
- explicit "trying route", "accepted route", and "rejected route" events
- white-box naming that matches prepared concepts such as:
  - branch condition
  - join transfer
  - continuation labels
  - value home
  - move bundle
  - local-slot render lane
- optional focus filtering by function name, block label, or value name so
  large cases such as `00204.c` stay inspectable

The trace should answer questions like:

- which renderer lane was tried first?
- which prepared facts were seen at the decision point?
- why was that lane rejected?
- which rejection was final?

For this idea, "readable" should now be interpreted more concretely:

- a developer should be able to correlate the BIR dump, prepared BIR dump, and
  MIR trace without guessing which stage renamed or erased the important fact
- a large-case trace should make the final meaningful rejection visually
  obvious instead of burying it inside many ordinary route misses
- the trace should point toward the next inspection step when possible, not
  only restate internal route wording

### 2. Add A Backend-Owned Error / Contract System

The backend should stop using plain `std::nullopt` as the main carrier for
meaningful route failure.

It needs a small contract/error system that distinguishes at least:

- ordinary route miss:
  this renderer did not match, but another one may still apply
- unsupported backend shape:
  the backend recognized the shape but does not support it yet
- violated internal prepared contract:
  required prepared facts were missing or inconsistent
- impossible state / bug:
  backend invariants were broken and execution should stop immediately

This should fill the same role that `llvm::unreachable`-style assertions and
backend-owned diagnostics fill in more mature compiler backends.

The key rule is:

- silent `std::nullopt` is acceptable only for cheap local probing where no
  durable debugging information would be lost
- once a route is materially meaningful for backend bring-up, its failure must
  be explainable through a backend-owned diagnostic path

## Scope

This idea owns backend observability and contract infrastructure for prepared
x86 handoff and nearby shared backend routing.

Expected work includes:

- a dedicated x86 route trace surface
- scoped trace helpers analogous in spirit to frontend parse tracing, but
  designed for backend routing rather than parser rollback
- a backend-specific route/error type or equivalent contract helpers
- consistent error text for missing prepared facts and violated handoff rules
- migration of the most important x86 handoff sites away from opaque
  `std::nullopt`

Expected work may also include improving the existing flags rather than adding
new ones, for example:

- tightening `--dump-prepared-bir` so the most important phase deltas are
  summarized before verbose detail
- making `--dump-mir` and `--trace-mir` more explicit about which prepared
  contract was missing, unsupported, or inconsistent
- adding stable focus controls for large cases instead of relying on ad hoc
  local logging

## Out Of Scope

This idea does not directly own:

- repairing one specific x86 failure family such as idea 60's scalar-emitter
  work
- changing source idea ownership for currently active packets
- adding one-off testcase-specific debug prints
- expanding emitter matcher coverage as a substitute for better diagnostics

If a concrete backend case becomes green while doing this work, that is a side
effect, not this idea's completion signal.

## Immediate Motivating Case

`c_testsuite_x86_backend_src_00204_c` is the current proof case for why this
idea is needed.

Semantic and prepared dumps can now show that the `match` function reaches
prepared control-flow facts such as:

- branch conditions
- select-materialization join transfers
- local-slot and pointer-backed addressing records

But those dumps still do not explain, in plain language, why the x86 consumer
rejects the route afterward.

That gap is exactly what this idea should close.

## What 00204.c Must Be Able To Tell Us

For a case at the scale of `00204.c`, backend debugging is only good enough if
one stable command sequence can answer all of the following without local code
instrumentation:

1. did semantic lowering produce the expected branch, join, select, call,
   addressing, and local-memory shape?
2. what did prepare change before x86 consumption?
3. which x86 lane consumed or rejected that shape?
4. which rejection was the final meaningful one?
5. was the failure:
   - an ordinary route miss
   - an unsupported backend shape
   - a missing prepared contract
   - or an impossible backend invariant?
6. what should the developer inspect next?

That last item matters because raw rejection text alone is not enough on a
large case. The tool needs to reduce search space, not only dump more text.

## Recommended Design Direction

Preferred shape:

1. keep semantic/prepared BIR dumps as the "what facts exist?" surface
2. add a separate backend trace surface for "how did x86 consume or reject
   those facts?"
3. add a backend route/error abstraction so route miss, unsupported shape,
   broken contract, and impossible invariant are not all encoded the same way

Good first building blocks:

- `X86RouteTrace` or equivalent trace sink
- scoped trace helpers for function and block dispatch
- explicit reject reasons at top-level renderer boundaries
- backend contract helpers for:
  - unsupported route
  - missing prepared fact
  - violated invariant
  - unreachable backend state

Useful next-layer improvements on top of the current flags:

- a concise BIR-to-prepared delta summary, such as:
  - phi nodes removed
  - branch-condition legalizations
  - join-transfer rewrites
  - stack slots introduced
  - value-home or move-bundle records added
- focus filtering by function, block, or value for large modules
- rejection text that includes the missing prepared concept in plain language
  and names the next debug surface to inspect

## Design Constraints

- do not copy frontend `ParseGuard` literally; backend does not need parser
  rollback snapshots
- do reuse the good part of that pattern: scoped, readable entry/exit tracing
- do not replace every `std::nullopt` blindly; start with the x86 handoff
  routes where current silence blocks real debugging
- do not let temporary debug output become the permanent API; add a named,
  stable backend debug mode instead
- error text should be plain-language first, not only internal jargon

## Concrete Acceptance Signals

This idea is making real progress when all of the following are true:

- the existing `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and
  `--trace-mir` surfaces feel like one coherent debug ladder rather than
  unrelated raw dumps
- a developer can run one stable backend trace command on `00204.c` or its
  reduction and see which x86 route was rejected
- the rejection text identifies the concrete missing or unsupported contract in
  plain language
- a developer can tell from the output whether the next step is to inspect
  semantic BIR, prepared BIR, or the x86 route boundary itself
- top-level x86 handoff paths no longer rely on silent `std::nullopt` for the
  final meaningful rejection
- backend bug states are separated from ordinary route mismatch states

This idea is complete when x86 handoff debugging no longer depends on ad hoc
instrumentation or matcher-poking just to learn where the route died.
