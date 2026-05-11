# HIR Lowerer Function Context TextId Authority Runbook

Status: Active
Source Idea: ideas/open/165_hir_lowerer_function_context_textid_authority.md

## Purpose

Convert HIR lowerer function-context source-name lookup away from rendered
strings where AST or HIR metadata provides honest `TextId` or structured local
domain identity.

## Goal

Classify `Lowerer::FunctionCtx` string maps, then convert covered source-level
lookups to structured/TextId authority without reworking route-local generated
handles.

## Core Rule

Do not treat rendered strings as ordinary semantic authority for source locals,
params, static locals, local constants, or function-pointer signatures when
complete `TextId` or structured local-domain metadata is available.

## Read First

- Source idea:
  `ideas/open/165_hir_lowerer_function_context_textid_authority.md`
- Parent idea:
  `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- Parent idea:
  `ideas/closed/162_link_name_id_backend_symbol_authority.md`
- `Lowerer::FunctionCtx` definition and all read/write sites
- HIR lowerer local, parameter, static local, local constant, function-pointer,
  pack parameter, and label lowering paths
- Focused HIR tests that cover local lookup, shadowing, params, local
  constants, and function-pointer signatures

## Current Scope

- Inventory `Lowerer::FunctionCtx` maps:
  - `locals`
  - `params`
  - `static_globals`
  - `local_fn_ptr_sigs`
  - `param_fn_ptr_sigs`
  - `local_const_bindings`
  - `pack_params`
  - `label_blocks`
- Classify each map as source semantic lookup, generated route-local handle,
  label control-flow handle, diagnostic/display support, or compatibility
  bridge.
- Add structured or `TextId` lookup for covered source-level names when
  complete metadata is present.
- Retain strings for route-local SSA/local-slot labels, generated backend
  handles, labels, diagnostics, and explicit compatibility boundaries.
- Add focused tests proving shadowing or same-spelled local lookup behavior
  through structured identity.
- Document retained rendered-name fallbacks with owner, limitation, and removal
  condition.

## Non-Goals

- Do not reopen template binding maps completed by idea 161.
- Do not reopen link-visible function or global symbol authority completed by
  idea 162.
- Do not convert block labels or generated local slot names into module-level
  symbols.
- Do not redesign full local SSA or value-id transport.
- Do not claim progress through dump text changes, helper renames, or
  expectation weakening.

## Working Model

- `FunctionCtx` currently mixes source semantic lookup with route-local handles
  in string-keyed maps.
- Source locals, params, static locals, local constants, and function-pointer
  signatures should prefer `TextId` or a structured local-domain key where the
  source metadata is complete.
- Labels, generated local slots, SSA-style route handles, and final display
  spelling can remain string-based when they are not semantic source identity.
- A complete structured miss for a covered source lookup should fail closed or
  stay within an explicit no-metadata compatibility boundary; it should not
  silently reopen rendered lookup.

## Execution Rules

- Start with inventory. Do not edit implementation or tests before recording
  the `FunctionCtx` map classification in `todo.md`.
- Convert one caller group at a time so proof can isolate source lookup
  behavior from route-local string handling.
- Prefer existing local-key or `TextId` carriers before adding a new carrier.
- Add a new structured key only when the route has enough scope/domain metadata
  to distinguish same-spelled source names honestly.
- Preserve string maps when their purpose is route-local control flow,
  generated names, display spelling, or explicit no-metadata compatibility.
- Every code-changing step needs fresh build or focused proof. Escalate to
  broader HIR/frontend validation if shared lowerer helper signatures or
  common `FunctionCtx` access patterns change.

## Ordered Steps

### Step 1: Inventory FunctionCtx String Maps

Goal: identify every `Lowerer::FunctionCtx` string-keyed map and classify its
semantic role.

Primary targets:
- `Lowerer::FunctionCtx` fields
- all read/write sites for `locals`, `params`, `static_globals`,
  `local_fn_ptr_sigs`, `param_fn_ptr_sigs`, `local_const_bindings`,
  `pack_params`, and `label_blocks`
- focused HIR tests for locals, params, local constants, static locals, labels,
  packs, and function-pointer signatures

Actions:
- Find every insertion, lookup, mutation, and fallback for the target maps.
- Classify each caller group as source semantic lookup, generated route-local
  handle, label control-flow handle, diagnostic/display support, or
  compatibility bridge.
- Record which paths already carry `TextId`, AST node identity, declaration
  identity, local scope identity, or no structured metadata.
- Select the first metadata-capable conversion target and recommend the narrow
  proof command in `todo.md`.

Completion check:
- `todo.md` contains the map classification, first conversion target, retained
  string boundaries, and proof recommendation.
- No implementation or test files are edited before the inventory is recorded.

### Step 2: Define Structured Authority for Convertible Source Lookups

Goal: choose the smallest honest structured key or `TextId` lookup model for
each metadata-capable source-level map.

Primary targets:
- local and parameter lookup paths
- static local lookup paths
- local constant lookup paths
- local and parameter function-pointer signature lookup paths

Actions:
- For each convertible map, define the lookup key and lifetime/domain metadata
  required to distinguish same-spelled source names.
- Reuse existing AST `TextId`, declaration identity, or local-domain key
  helpers where they already express the needed authority.
- Decide which rendered maps remain no-metadata compatibility fallbacks and
  document their owner, limitation, and removal condition in `todo.md`.
- Keep labels, pack expansion bookkeeping, generated handles, diagnostics, and
  display strings out of the semantic-key conversion unless inventory proves
  they are source semantic lookup.

Completion check:
- Every selected conversion target has a documented structured authority model.
- Every non-converted map has a documented reason to remain string-keyed.

### Step 3: Convert the First Source Lookup Group

Goal: make the first metadata-capable source lookup group prefer structured or
`TextId` authority before rendered strings.

Primary targets:
- first caller group selected in Step 1
- associated `FunctionCtx` map and helper accessors
- focused HIR tests for that lookup group

Actions:
- Add the structured or `TextId` map alongside the existing rendered map where
  compatibility still needs the rendered spelling.
- Update insertions to populate structured authority when metadata is complete.
- Update lookups to use structured authority first for covered metadata.
- Fail closed on complete structured misses, or route only explicit
  no-metadata cases through the documented compatibility bridge.
- Add focused same-spelled or shadowing coverage that would fail under ordinary
  rendered-string authority.

Completion check:
- The selected source lookup group no longer uses rendered strings as ordinary
  authority when complete metadata is present.
- Narrow focused proof passes and is recorded in `todo.md`.

### Step 4: Convert Remaining Metadata-Capable FunctionCtx Source Lookups

Goal: finish the remaining source-level maps without converting route-local
  strings or labels by mistake.

Primary targets:
- remaining convertible caller groups from Step 1
- `locals`, `params`, `static_globals`, `local_const_bindings`,
  `local_fn_ptr_sigs`, and `param_fn_ptr_sigs` as classified by inventory

Actions:
- Convert one caller group at a time using the Step 2 authority model.
- Keep no-metadata rendered fallback only behind explicit compatibility checks.
- Add or extend focused tests for each converted class of lookup where a
  same-spelled or shadowing case is reachable.
- Re-run the focused proof after each conversion.

Completion check:
- All metadata-capable source-level `FunctionCtx` lookups use structured or
  `TextId` authority.
- Route-local string maps remain string-keyed only for classified non-semantic
  purposes.

### Step 5: Fence Retained String Maps and Compatibility Bridges

Goal: make the remaining rendered maps explicit and non-accidental.

Primary targets:
- retained label, pack, generated-handle, diagnostic/display, and
  compatibility maps
- fallback helper boundaries touched during conversion

Actions:
- Document each retained string-keyed map in `todo.md` with owner, limitation,
  and removal condition.
- Rename or narrow helper APIs only where doing so clarifies structured versus
  rendered authority without causing broad churn.
- Add fail-closed or parity checks where complete structured metadata exists
  and a rendered fallback would otherwise hide a mismatch.

Completion check:
- Retained strings are classified and fenced.
- No covered source lookup silently falls back to rendered strings after a
  complete structured miss.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the idea for supervisor review and lifecycle closure.

Actions:
- Re-run the focused HIR lowerer proof that covers the converted lookup groups.
- Run broader frontend/HIR validation if shared lowerer helpers or `FunctionCtx`
  access patterns changed.
- Confirm ideas 161 and 162 were not reopened.
- Record final converted paths, retained string maps, compatibility boundaries,
  exact proof commands, and residual blockers in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- Focused HIR tests cover shadowing or same-spelled local lookup behavior for
  converted paths.
