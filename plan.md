# Static Member Compile-Time Local Enum Lifetime Runbook

Status: Active
Source Idea: ideas/open/168_static_member_compile_time_local_enum_lifetime.md

## Purpose

Define the scoped local/block enum lifetime boundary for static-member
initializer evaluation and compile-time engine state without reopening the
completed HIR lowerer route from idea 167.

## Goal

Classify static-member initializer and compile-time-engine enum lookup paths,
then convert only paths with honest scoped lifetime metadata to structured or
scoped `TextId`-aware lookup.

## Core Rule

Do not treat `CompileTimeState`, raw `TextId`, rendered spelling, or a
static-member bridge as local/block enum authority unless the path carries
explicit scope or domain lifetime metadata for the enum being resolved.

## Read First

- Source idea: `ideas/open/168_static_member_compile_time_local_enum_lifetime.md`
- Completed parent idea: `ideas/closed/167_local_block_enum_scope_static_eval_structured_mirrors.md`
- Completed parent idea: `ideas/closed/164_sema_type_utils_static_eval_structured_lookup.md`
- Static-member initializer evaluation paths
- Compile-time engine state and enum constant lookup paths
- Existing compatibility boundaries for static-member and rendered enum lookup

## Current Scope

- Inventory static-member initializer and compile-time-engine paths that can
  observe local/block enum constants.
- Classify each path as scoped-lifetime capable, rendered/static-member
  compatibility only, unreachable by well-formed local/block enum lifetimes, or
  unrelated.
- Design a scoped lifetime model before converting any compile-time-engine path.
- Convert only paths with complete scope/domain metadata.
- Document retained compatibility bridges with owner, limitation, and removal
  condition.
- Add focused collision coverage for same-spelled local/block enum constants
  for each converted path, or document why no such path is reachable.

## Non-Goals

- Do not rework the completed idea 167 HIR lowerer scoped enum maps.
- Do not reopen the global/static-member structured lookup conversion completed
  by idea 164 except at the documented compatibility boundary.
- Do not rewrite the broad consteval interpreter unless the inventory proves it
  is required for local/block enum lifetime.
- Do not make backend, ABI, parser, or diagnostic-string cleanup changes.
- Do not claim progress through expectation downgrades, helper renames, or dump
  text changes alone.

## Working Model

- Idea 167 completed the lowerer-focused local/block enum route by carrying
  scoped enum `TextId` and local-key metadata into `ConstEvalEnv`.
- Static-member initializer evaluation still sits at a separate boundary that
  may use rendered lookup or static-member compatibility bridges.
- `CompileTimeState` does not, by itself, define local/block enum scope
  lifetime. Any compile-time-engine conversion needs an explicit lifetime model
  before it can use structured local/block enum authority.
- Some candidate paths may be unreachable for well-formed local/block enum
  lifetimes. Those should be documented rather than forced through fabricated
  metadata.

## Execution Rules

- Start with inventory. Do not edit implementation or tests before recording
  the static-member and compile-time-engine path classification in `todo.md`.
- Keep idea 167 lowerer scoped maps fixed unless a read-only comparison is
  needed to understand the compatibility boundary.
- Fail closed on complete structured misses in converted paths; do not reopen
  rendered enum mirrors after complete metadata is present.
- Preserve rendered/static-member compatibility only behind explicit
  no-lifetime or no-metadata boundaries.
- Every code-changing step needs fresh build or focused proof. Escalate to
  broader validation if shared compile-time engine state or static-eval helper
  signatures change.

## Ordered Steps

### Step 1: Inventory Static-Member and Compile-Time Enum Paths

Goal: identify every static-member initializer and compile-time-engine path
that can observe local/block enum constants.

Primary targets:
- static-member initializer evaluation call sites
- `CompileTimeState` enum and constant lookup registries
- compatibility bridges that use rendered names or static-member lookup
- tests that exercise static-member initializers, consteval, and local/block
  enum constants

Actions:
- Find lookup sites that can resolve enum constants during static-member
  initializer or compile-time-engine evaluation.
- Classify each path as scoped-lifetime capable, rendered/static-member
  compatibility only, unreachable by well-formed local/block enum lifetimes, or
  unrelated.
- Identify whether any path already carries enum scope/domain metadata from the
  HIR lowerer route completed by idea 167.
- Record the inventory, classification, first packet target, and proof command
  recommendation in `todo.md`.

Completion check:
- `todo.md` names each candidate caller group and the selected first
  implementation or documentation target.
- No implementation or test files are edited before the inventory is recorded.

### Step 2: Define the Scoped Lifetime Model for Convertible Paths

Goal: describe how any convertible path proves local/block enum lifetime before
using structured lookup.

Primary targets:
- compile-time engine state that owns enum constant lookup
- static-member initializer evaluation inputs
- existing structured lookup carriers from ideas 164 and 167

Actions:
- For each scoped-lifetime capable path, define the scope/domain metadata that
  makes same-spelled local/block enum constants distinguishable.
- Reuse existing carriers where they already express the necessary lifetime.
- Add or adjust a small carrier only where the path has complete metadata.
- Keep raw `TextId`, rendered spelling, and static-member bridges as
  compatibility inputs, not structured authority.

Completion check:
- Convertible paths have an explicit scoped lifetime model.
- Paths without complete metadata remain documented compatibility or
  unreachable paths.

### Step 3: Convert the First Metadata-Capable Path

Goal: make the first honest scoped-lifetime path use structured lookup instead
of rendered or static-member compatibility authority.

Primary targets:
- first caller group selected in Step 1
- enum constant lookup boundary used by that caller group

Actions:
- Pass the scoped lifetime carrier into the selected lookup boundary.
- Resolve local/block enum constants through structured or scoped
  `TextId`-aware lookup when metadata is complete.
- Fail closed on complete structured misses.
- Leave rendered/static-member lookup only behind a documented compatibility
  bridge with owner, limitation, and removal condition.

Completion check:
- The selected path no longer uses rendered spelling, raw `TextId`, or a
  static-member bridge as ordinary local/block enum authority.
- Existing compatibility behavior is still available only through the explicit
  no-metadata boundary.

### Step 4: Add Focused Collision Coverage

Goal: prove converted paths distinguish same-spelled local/block enum constants
by scope or domain lifetime.

Primary targets:
- focused static-member initializer or compile-time-engine tests for converted
  paths

Actions:
- Add same-spelled local/block enum constants in distinct scopes for each
  converted path.
- Assert evaluated values or lookup identity, not only emitted spelling.
- Add a fail-closed case if the converted boundary has an observable diagnostic
  route for complete structured misses.
- Document in `todo.md` when inventory proves no static-member or
  compile-time-engine path is currently reachable for a collision test.

Completion check:
- Tests fail under rendered/static-member authority and pass through the
  scoped lifetime path, or `todo.md` explains why no converted path is
  reachable.

### Step 5: Convert or Fence Remaining Candidate Paths

Goal: finish the remaining candidate paths without fabricating lifetime
authority.

Primary targets:
- remaining caller groups classified in Step 1
- retained rendered/static-member compatibility boundaries

Actions:
- Convert one metadata-capable caller group at a time.
- For each non-converted path, document why it is compatibility only,
  unreachable, or unrelated.
- Add owner, limitation, and removal condition for every retained bridge.
- Re-run the focused proof after each converted caller group.

Completion check:
- All scoped-lifetime capable paths use structured or scoped `TextId`-aware
  lookup.
- All retained rendered/static-member bridges are classified and fenced.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the idea for supervisor review and eventual lifecycle closure.

Actions:
- Re-run the focused static-member or compile-time-engine local/block enum
  proof.
- Run broader build or frontend validation if shared compile-time engine state
  or static-eval helpers changed.
- Confirm idea 167 lowerer scoped maps were not reopened to force this route.
- Record final proof, converted paths, retained bridges, unreachable paths, and
  exact residual blockers in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- No converted static-member or compile-time-engine path uses rendered
  spelling, raw `TextId`, or a static-member bridge as ordinary local/block
  enum authority.
