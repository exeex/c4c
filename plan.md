# Local Block Enum Scope Static Eval Structured Mirrors Runbook

Status: Active
Source Idea: ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md

## Purpose

Retire or narrow the remaining local/block enum-scope static-eval dependence on
mutable rendered enum mirrors while preserving explicit compatibility for paths
that cannot yet carry structured enum metadata.

## Goal

Make metadata-capable local and block enum static-eval paths use structured or
scoped `TextId`-aware enum lookup, fail closed on complete structured misses,
and keep rendered mirror lookup only as a documented no-metadata bridge.

## Core Rule

Do not treat raw spelling or raw `TextId` alone as sufficient local/block enum
authority. Covered lookup must carry enough enum/domain or scope context to
distinguish same-spelled constants across local and block enum scopes.

## Read First

- Source idea: `ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md`
- Completed parent idea: `ideas/closed/164_sema_type_utils_static_eval_structured_lookup.md`
- Static eval structured lookup implementation in `src/frontend/sema/type_utils.cpp`
- HIR lowerer paths that save, restore, or consult mutable `enum_consts_`
- Local/block enum parsing, sema, and lowering tests that exercise static
  evaluation

## Current Scope

- Inventory local and block enum paths that feed static-eval enum constants
  through rendered compatibility maps or mutable mirror state.
- Classify each path by whether it can carry structured enum-domain metadata,
  scoped `TextId` metadata, or only rendered compatibility data.
- Convert metadata-capable local/block paths to structured lookup without
  fabricating incomplete identity.
- Fence retained mutable rendered mirror paths with owner, limitation, and
  removal condition.
- Add focused collision coverage for same-spelled local/block enum constants
  when structured metadata is complete.

## Non-Goals

- Do not reopen the global/static-member lookup conversion completed by idea
  164.
- Do not rework the full consteval interpreter.
- Do not remove diagnostic or source spelling strings.
- Do not make broad backend or ABI changes.
- Do not claim progress through helper renames, expectation rewrites, or dump
  text changes alone.

## Working Model

- Idea 164 already gave covered global/static-member static-eval paths a
  structured lookup carrier and fenced rendered lookup as compatibility.
- Local/block enum scopes may still depend on mutable `enum_consts_`
  save/restore behavior, with rendered spelling acting as ordinary authority.
- Some local/block contexts may have enough ownership, scope, and `TextId`
  metadata to use structured lookup; others may only have legacy rendered
  mirrors.
- Complete structured misses in covered metadata-capable paths must fail closed
  instead of reopening rendered spelling lookup.

## Execution Rules

- Start with inventory. Do not edit implementation or tests before recording
  the local/block enum caller map in `todo.md`.
- Keep changes focused on local/block enum ownership and lifetime; do not
  revisit the parent idea's global/static-member route unless needed for a
  compatibility boundary.
- Prefer small carrier or mirror-boundary changes over broad sema/HIR rewrites.
- Document retained compatibility at the boundary that owns the rendered mirror,
  not only in tests.
- Every code-changing step needs fresh build or focused proof. Escalate to
  broader validation if a helper signature touches shared sema or lowerer
  static-eval surfaces.

## Ordered Steps

### Step 1: Inventory Local and Block Enum Static-Eval Paths

Goal: identify every local/block enum path that can reach static evaluation
through rendered enum mirrors or mutable mirror state.

Primary targets:
- HIR lowerer `enum_consts_` save/restore and lookup sites
- static-eval call sites that may evaluate local or block enum constants
- local/block enum sema and lowering tests

Actions:
- Find mutable rendered enum mirror writes, save/restore scopes, and lookup
  consumers used during static evaluation.
- Classify each path as structured-domain capable, scoped-`TextId` capable,
  rendered-compatibility only, or unrelated to local/block enum static eval.
- Identify the narrowest first caller group that can prove structured
  local/block lookup without reopening global/static-member behavior.
- Record the inventory, classifications, first packet target, and proof command
  recommendation in `todo.md`.

Completion check:
- `todo.md` names each local/block enum caller group and the selected first
  implementation target.
- No implementation or test files are edited before the inventory is recorded.

### Step 2: Define the Local/Block Enum Lookup Carrier

Goal: provide a lookup input that can represent local/block enum identity
without relying on rendered spelling as authority.

Primary targets:
- static-eval structured lookup helper types from idea 164
- HIR lowerer or sema structures that own local/block enum scope metadata

Actions:
- Inspect existing structured enum lookup carriers and determine whether they
  can represent local/block enum domains directly.
- Add or extend a small carrier only where enough scope/domain metadata exists.
- Keep raw `TextId` from becoming authority unless paired with the domain data
  needed to distinguish local/block scopes.
- Preserve an explicit compatibility input for paths that lack complete
  metadata.

Completion check:
- Metadata-capable local/block paths can describe enum constants without
  fabricated identity.
- No-metadata callers still compile through an explicit compatibility bridge.

### Step 3: Prefer Structured Lookup for the First Covered Path

Goal: convert the first metadata-capable local/block static-eval path to use
structured lookup before rendered mirrors.

Primary targets:
- first caller group selected in Step 1
- static-eval name/enum constant resolution boundary

Actions:
- Pass the local/block enum carrier from the selected caller group into static
  evaluation.
- Resolve enum constants through structured or scoped `TextId` metadata when
  complete.
- Fail closed on complete structured misses instead of falling back to mutable
  rendered mirrors.
- Add a concise compatibility-boundary comment for any retained rendered path.

Completion check:
- The selected path no longer treats rendered spelling as ordinary enum
  authority.
- Existing rendered-only behavior remains available only through the documented
  compatibility bridge.

### Step 4: Add Local/Block Collision Coverage

Goal: prove same-spelled local/block enum constants do not collide in covered
structured paths.

Primary targets:
- focused frontend sema/HIR tests for local and block enum static evaluation

Actions:
- Add tests with same-spelled enum constants in distinct local or block enum
  scopes that are statically evaluated through the converted path.
- Ensure assertions observe lookup identity or evaluated values, not only dump
  or diagnostic spelling.
- Include a fail-closed case if complete structured metadata can miss through
  an existing diagnostic route.

Completion check:
- Focused tests fail against rendered mirror authority and pass through the
  structured path.
- Nearby same-feature local/block enum cases are examined or documented in
  `todo.md` as out of reach for the current carrier.

### Step 5: Convert Remaining Metadata-Capable Paths

Goal: finish the local/block enum paths that can honestly provide structured
metadata.

Primary targets:
- remaining caller groups classified as structured-domain capable or
  scoped-`TextId` capable in Step 1

Actions:
- Convert one caller group at a time, keeping each proof narrow.
- Do not force structured lookup onto paths whose lifetime or ownership data is
  incomplete.
- For retained rendered mirror users, write owner, limitation, and removal
  condition into `todo.md` and the relevant compatibility boundary.
- Re-run focused coverage after each converted group.

Completion check:
- All metadata-capable local/block paths use structured or scoped `TextId`-aware
  lookup.
- Remaining rendered paths are explicitly classified as compatibility bridges.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the idea for supervisor review and eventual lifecycle closure.

Actions:
- Re-run the focused local/block enum static-eval proof.
- Run broader build or frontend validation if shared static-eval helpers or HIR
  lowerer signatures changed.
- Confirm global/static-member behavior from idea 164 was not reopened.
- Record final proof, converted paths, retained compatibility callers, and
  exact residual blockers in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- No covered metadata-capable local/block enum static-eval path still uses
  mutable rendered mirrors as ordinary semantic authority.
