# Prealloc Schema Header Decomposition Runbook

Status: Active
Source Idea: ideas/open/272_prealloc_schema_header_decomposition.md

## Purpose

Split `src/backend/prealloc/prealloc.hpp` into focused public schema headers
without changing prepared BIR behavior.

Goal: make prepared schema ownership visible by family while keeping
`prealloc.hpp` as the compatibility umbrella.

Core Rule: this is a behavior-preserving header decomposition. Do not change
prepared output, field semantics, allocation behavior, stack layout behavior,
or test expectations.

## Read First

- `ideas/open/272_prealloc_schema_header_decomposition.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/prealloc/prepared_printer.hpp`

## Current Targets

- Public schema declarations currently concentrated in
  `src/backend/prealloc/prealloc.hpp`
- Compatibility umbrella: `src/backend/prealloc/prealloc.hpp`
- New focused public headers under `src/backend/prealloc/`
- Include adjustments in implementation files and tests only as needed to
  compile against the split

## Non-Goals

- Do not split `prealloc.cpp`, `regalloc.cpp`, or `prepared_printer.cpp`
  implementation bodies except for include adjustments required by the header
  split.
- Do not rename public prepared structs or enums for aesthetics.
- Do not remove the `prealloc.hpp` compatibility umbrella.
- Do not redesign prepared facts, stack layout, register allocation, ABI moves,
  or dump output.
- Do not weaken tests or expected prepared dumps.

## Working Model

`prealloc.hpp` should remain the public include that downstream consumers can
use unchanged. Focused headers should own coherent prepared fact families and
should include only their real dependencies. Implementation files may later
adopt narrower headers, but this runbook should not require a broad consumer
migration to succeed.

Candidate header families from the source idea:

- `names.hpp`
- `module.hpp`
- `frame.hpp`
- `addressing.hpp`
- `liveness.hpp`
- `regalloc.hpp`
- `value_locations.hpp`
- `calls.hpp`
- `variadic.hpp`
- `runtime_helpers.hpp`
- `special_carriers.hpp`
- `control_flow.hpp`

## Execution Rules

- Keep each step compile-proven before moving to another family.
- Prefer moving declarations and inline helpers as intact blocks; avoid
  semantic edits while moving code.
- Keep include direction acyclic. Focused headers must not simply include the
  umbrella.
- Preserve public names and namespace placement.
- Use forward declarations where practical, but do not force them if a
  concrete type dependency is clearer and lighter.
- If a family boundary is ambiguous, record the decision in `todo.md` and keep
  the source idea unchanged unless source intent truly changes.
- Treat expectation rewrites, prepared dump changes, or behavior changes as
  blockers for this plan.

## Ordered Steps

### Step 1: Inventory current schema families and dependency edges

Goal: build an ownership map for `prealloc.hpp` before moving declarations.

Primary target: `src/backend/prealloc/prealloc.hpp`

Actions:

- Inspect the current declaration order, inline helpers, and required standard
  and project includes.
- Group the declarations into the candidate header families listed above.
- Identify unavoidable cross-family dependencies such as names, frame objects,
  addressing, value homes, control-flow helpers, and `PreparedBirModule`.
- Record any ambiguous ownership choices in `todo.md`; do not edit the source
  idea for routine boundary notes.

Completion check:

- `todo.md` names the first extraction family and any dependency constraints an
  executor must respect.
- No behavior or implementation files have changed in this inventory-only
  step unless the supervisor explicitly delegates implementation work.

### Step 2: Extract names and base declarations

Goal: establish the lowest-level public headers that other schema families can
depend on.

Primary targets:

- `src/backend/prealloc/names.hpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- Move prepared IDs, `PrepareOptions`, `PrepareNote`, `PreparedNameTables`, and
  name lookup helpers into `names.hpp` or another low-level focused header if
  the dependency map proves that name clearer.
- Keep only forward declarations and stable prepare API declarations in the
  umbrella while the remaining families are extracted.
- Defer moving the `PreparedBirModule` aggregate until the headers for its
  member families exist, so `module.hpp` can include real focused headers
  without depending on the umbrella.
- Keep `infer_call_arg_abi`, `BirPreAlloc`, and prepare entry declarations
  reachable from the compatibility umbrella.
- Update includes so `prealloc.hpp` remains the stable public entry point.

Completion check:

- Existing consumers that include `prealloc.hpp` still compile.
- The umbrella includes the new focused headers without circular include
  dependencies.

### Step 3: Extract frame, addressing, liveness, and regalloc schema

Goal: move the high-traffic prepared analysis and allocation families into
  focused headers.

Primary targets:

- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/liveness.hpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- Move stack objects, frame slots, frame plans, dynamic stack plans, addressing
  facts, liveness facts, register allocation facts, value homes, move bundles,
  and associated inline lookup helpers into their natural headers.
- Keep helper placement tied to the data family it queries.
- Use narrower cross-includes between focused headers only when a moved type
  requires a full definition.

Completion check:

- The moved families compile through the umbrella.
- No allocation decisions, value-home semantics, or prepared dumps change.

### Step 4: Extract calls, variadic, storage, carriers, helpers, control flow, and module

Goal: finish the remaining major prepared fact families and leave the umbrella
as an include aggregator plus stable API declarations.

Primary targets:

- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prealloc.hpp`

Actions:

- Move call plans, variadic entry plans, storage plans, i128/f128 carriers,
  atomics, intrinsics, inline asm carriers, runtime helpers, control-flow
  facts, and associated inline query helpers.
- Keep behavior-specific helpers in the owning family header when they are
  already public inline schema queries.
- Move the `PreparedBirModule` aggregate and module-level lookup helpers into
  `module.hpp` after the aggregate's member-family headers exist.
- Avoid creating a broad helper header that becomes a second monolith.

Completion check:

- `prealloc.hpp` no longer owns all prepared schema families directly.
- Focused headers own the major prepared fact families from the source idea.

### Step 5: Tighten includes and prove behavior preservation

Goal: verify the split is compile-safe and behavior-preserving.

Primary targets:

- `src/backend/prealloc/*.hpp`
- Prealloc implementation and test include sites touched by the split

Actions:

- Replace unnecessary umbrella includes in nearby implementation files only
  where the narrower header dependency is obvious and low risk.
- Keep broader consumer migration out of scope.
- Run a fresh build or compile proof, then the supervisor-selected
  backend/prealloc test subset.
- Escalate to broader validation if include churn crosses multiple backend
  buckets or prepared dump output is touched.

Completion check:

- Build or compile proof is fresh.
- Backend/prealloc tests selected by the supervisor pass.
- No prepared output or test expectations changed.

## Acceptance Checklist

- `prealloc.hpp` mainly acts as a compatibility umbrella and stable API entry.
- Focused headers exist for the major prepared fact families.
- Include dependencies are acyclic and do not route focused headers through
  the umbrella.
- Existing public prepared type names remain stable.
- Existing backend/prealloc tests compile and pass without behavior changes.
