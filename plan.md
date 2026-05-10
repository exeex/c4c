# Sema Consteval Domain Table Authority Runbook

Status: Active
Source Idea: ideas/open/159_sema_consteval_domain_table_authority.md

## Purpose

Move Sema consteval lookup authority away from rendered string maps and toward
structured keys and `TextId` maps wherever complete metadata exists.

## Goal

Make covered consteval value, function, type, NTTP, interpreter-local, and
record-layout lookup paths prefer structured or text-keyed tables, and fail
closed on complete structured misses instead of reopening rendered-name
fallbacks.

## Core Rule

Rendered `std::string` maps may remain for diagnostics, display, source syntax
payloads, legacy no-metadata compatibility, and documented bridges only. They
must not be the ordinary semantic authority for covered metadata-rich paths.

## Read First

- `ideas/open/159_sema_consteval_domain_table_authority.md`
- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/consteval.cpp`
- Nearby tests covering sema consteval calls, template bindings, NTTPs,
  local interpreter bindings, and record-layout constant folding.

## Current Targets And Scope

- `ConstEvalEnv::lookup` and `ConstEvalEnv::lookup_by_key`
- `lookup_type_binding_by_text`, `lookup_type_binding_by_key`, and
  `apply_type_bindings_to_type`
- `lookup_forwarded_nttp_arg_by_text` and related NTTP binding lookup paths
- `bind_consteval_call_env` output mirrors for type and value bindings
- `lookup_consteval_function` and `evaluate_consteval_call`
- `InterpreterBindings::by_name`, `by_text`, and `by_key`
- Constant-layout record lookup through `struct_defs`,
  `struct_def_owner_index`, and `link_name_texts`

## Non-Goals

- Do not migrate full HIR `TypeBindings` or `NttpBindings` storage.
- Do not clean up all of `src/frontend/sema/canonical_symbol.cpp`.
- Do not rewrite the consteval interpreter control-flow model.
- Do not remove diagnostic text, source spelling, literal payloads, or deferred
  syntax strings.
- Do not remove every rendered string map in one pass when it is still needed
  as a no-metadata compatibility bridge.
- Do not weaken tests, mark supported consteval paths unsupported, or claim
  progress through expectation rewrites.

## Working Model

- Parser owns spelling, token streams, name `TextId`s, template parameter
  order/kind metadata, and deferred syntax payloads.
- Sema validate owns eager declaration and visibility validation, including
  registration of consteval functions, constants, scopes, and structured
  handoff metadata.
- Sema consteval owns interpretation lookup for constants, functions, local
  interpreter bindings, template type and NTTP bindings, and record-layout data.
- HIR may provide late record/layout/template context, but Sema consteval should
  consume structured handoff tables when present.

## Execution Rules

- Start with inventory and classification before changing authority.
- Prefer semantic-domain keys over raw rendered names; do not treat raw
  `TextId` alone as cross-domain identity without domain context.
- For a covered lookup with complete structured metadata, a structured miss is
  authoritative and must not fall through to rendered string lookup.
- Keep rendered maps only as explicit compatibility bridges. Record each
  retained bridge with its owner, limitation, and removal condition.
- Add focused tests for stale rendered names instead of broad expectation churn.
- Use a validation ladder for code-changing steps: build or compile proof,
  narrow sema consteval tests, then broader parser/HIR/frontend checks if
  touched by handoff changes.

## Steps

### Step 1: Inventory rendered consteval authority

Goal: Classify every string-keyed consteval map and lookup surface named by the
source idea.

Primary Target: `src/frontend/sema/consteval.hpp` and
`src/frontend/sema/consteval.cpp`

Concrete Actions:
- Inspect `ConstMap`, `TypeBindings`, `ConstEvalEnv` maps,
  `InterpreterBindings::by_name`, `evaluate_consteval_call`,
  `bind_consteval_call_env`, and record-layout lookup paths.
- Classify each rendered string surface as semantic authority,
  compatibility bridge, diagnostic/display, syntax payload, or no-metadata
  fallback.
- Identify which lookup paths already have complete structured or text-keyed
  metadata available.
- Add concise in-code documentation for retained bridges where the source code
  has no owner/limitation/removal note.

Completion Check:
- The executor can point to each retained rendered map and explain whether it is
  authority, compatibility, diagnostic/display, syntax payload, or fallback.
- No semantic behavior has been changed beyond documentation and mechanical
  classification.

### Step 2: Make value and local binding lookup structured-first

Goal: Make consteval value lookup and interpreter-local lookup use structured
or text-keyed authority before rendered names.

Primary Target: `ConstEvalEnv::lookup`, `ConstEvalEnv::lookup_by_key`, and
`InterpreterBindings` lookup code.

Concrete Actions:
- Route complete metadata value lookups through `ConstStructuredMap` or
  `ConstTextMap` before `ConstMap`.
- Ensure complete structured misses for covered value domains fail closed.
- Make `InterpreterBindings::by_text` and `by_key` authoritative before
  `by_name` when metadata is complete.
- Keep rendered `by_name` and `ConstMap` fallback only for documented
  no-metadata compatibility paths.
- Add tests proving stale rendered local or consteval names cannot override
  structured metadata.

Completion Check:
- Tests fail before the behavior change and pass after it.
- Narrow sema consteval validation passes.
- No supported consteval path is downgraded or marked unsupported.

### Step 3: Make consteval function lookup structured-first

Goal: Make consteval call resolution prefer structured function maps and
text-keyed maps over rendered function names.

Primary Target: `lookup_consteval_function` and `evaluate_consteval_call`.

Concrete Actions:
- Prefer `ConstEvalFunctionStructuredMap` for complete structured call
  metadata.
- Use `ConstEvalFunctionTextMap` where text identity is the strongest available
  metadata.
- Restrict rendered `consteval_fns` lookup to documented no-metadata
  compatibility.
- Ensure complete structured misses do not fall back to rendered function names.
- Add focused tests for stale rendered consteval function names.

Completion Check:
- Covered consteval call lookup cannot be redirected by stale rendered names.
- Narrow function-call consteval tests pass.
- Any touched parser/HIR/frontend handoff tests are included in validation.

### Step 4: Make template type and NTTP binding lookup structured-first

Goal: Make template type and NTTP binding lookup use structured or text-keyed
binding maps as authority where metadata exists.

Primary Target: `lookup_type_binding_by_text`, `lookup_type_binding_by_key`,
`apply_type_bindings_to_type`, `lookup_forwarded_nttp_arg_by_text`, and
`bind_consteval_call_env`.

Concrete Actions:
- Prefer `TypeBindingStructuredMap` and `TypeBindingTextMap` for type binding
  lookup.
- Prefer `ConstStructuredMap` and `ConstTextMap` for NTTP binding lookup.
- Audit `bind_consteval_call_env` so output string maps are compatibility
  mirrors, not reopened authority after complete metadata misses.
- Add tests where stale rendered template parameter or NTTP names cannot
  override structured metadata.

Completion Check:
- Covered type and NTTP lookup paths fail closed on complete structured misses.
- Compatibility string outputs remain available only for no-metadata bridges.
- Narrow template/NTTP consteval validation passes.

### Step 5: Make record-layout lookup prefer owner keys

Goal: Make record-layout constant folding prefer record-owner metadata before
rendered record tags.

Primary Target: `struct_defs`, `struct_def_owner_index`, `link_name_texts`, and
record-layout lookup used for `sizeof`, `alignof`, and constant folding.

Concrete Actions:
- Identify complete record-owner metadata available during consteval
  evaluation.
- Prefer `struct_def_owner_index` and owner-linked text metadata before
  rendered `struct_defs`.
- Keep rendered record tag lookup only as a documented compatibility bridge.
- Add tests where stale rendered record tags cannot override owner metadata.

Completion Check:
- Covered record-layout lookups use owner metadata first and fail closed on
  complete owner-key misses.
- Record-related consteval folding tests pass.

### Step 6: Consolidate bridge documentation and validation

Goal: Leave consteval authority boundaries documented and prove the migration
with focused and broader validation.

Primary Target: consteval authority comments, focused tests, and validation
logs.

Concrete Actions:
- Review retained rendered maps and ensure each has owner, limitation, and
  removal condition documented close to the bridge.
- Remove or tighten any temporary helper that reintroduces rendered-name
  authority for covered paths.
- Run sema consteval tests plus any parser/HIR/frontend subsets touched by
  handoff behavior.
- Escalate to broader validation if multiple domains were changed in one slice
  or if handoff behavior crosses subsystem boundaries.

Completion Check:
- Acceptance criteria from the source idea are satisfied.
- Validation covers sema consteval tests and all touched handoff subsets.
- Reviewer reject signals from the source idea are not present in the diff.
