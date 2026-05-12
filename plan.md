# Sema Legacy Compatibility Retirement Runbook

Status: Active
Source Idea: ideas/open/199_sema_legacy_compatibility_retirement.md

## Purpose

Retire or hard-fence Sema-owned legacy compatibility paths so semantic
analysis and consteval support prefer structured keys, `TextId` carriers, and
owner-aware domains without reopening rendered-string fallback on
metadata-rich misses.

## Goal

Covered Sema metadata-rich routes must use structured semantic authority before
rendered names, and complete structured misses must fail closed or enter an
explicit no-metadata compatibility boundary.

## Core Rule

Do not claim Sema compatibility progress through helper renames, expectation
rewrites, or classification-only edits. Covered paths must stop recovering
semantic identity through rendered enum names, consteval function names, NTTP
maps, type-binding maps, or struct-def maps after complete structured metadata
misses.

## Read First

- `ideas/open/199_sema_legacy_compatibility_retirement.md`
- Sema and consteval source comments mentioning compatibility, legacy,
  deprecated, rendered, no-metadata, enum lookup, type bindings, NTTP,
  consteval functions, local const maps, or struct layout handoff.
- Parser closure ledger in recent `todo.md` history when identifying residual
  Sema-facing no-metadata callers.
- Existing Sema, HIR-facing Sema, static-eval, consteval, template
  substitution, and layout tests that exercise stale rendered names.

## Current Targets And Scope

- `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility`
- `static_eval_int_with_rendered_enum_compatibility`
- `same_rendered_type_name_compatibility`
- Legacy rendered type-binding bridges in consteval substitution.
- Rendered NTTP output mirrors and `nttp_bindings`.
- `type_binding_text_ids_by_name` and `type_binding_keys_by_name` rendered-name
  bridge maps.
- Rendered `consteval_fns` compatibility lookup after structured consteval
  metadata checks.
- Interpreter `by_name` local const compatibility mirrors.
- Rendered `ConstEvalEnv::struct_defs` fallback for legacy layout handoff.

## Non-Goals

- Do not reopen parser syntax-carrier cleanup.
- Do not clean HIR registry or lowerer state except at the Sema-facing
  boundary needed to prove this idea.
- Do not edit BIR, LIR, or backend compatibility routes.
- Do not remove diagnostic/display names, ABI spelling, source payload strings,
  or final output spelling.
- Do not rewrite the full consteval engine or template substitution model.
- Do not weaken tests, mark supported paths unsupported, or hide stale
  rendered-name behavior behind expected-output changes.

## Working Model

- Structured Sema authority includes domain-scoped `TextId` metadata,
  declaration or binding keys, owner-aware consteval metadata, canonical symbol
  identity, structured template owner keys, and static-eval input metadata.
- Rendered names may remain as display text, diagnostic spelling, source
  payload, final output spelling, or explicit no-metadata compatibility.
- A complete structured miss in a covered Sema route must not recover through
  rendered maps unless the route first proves it lacks the structured metadata
  required for that domain.
- Retained Sema bridges need nearby comments containing `legacy` or
  `deprecated`, plus owner, limitation, and removal condition.

## Execution Rules

- Start with inventory and route classification before deleting helpers.
- Prefer narrow semantic slices: fence one compatibility route, add stale
  rendered proof, then move to the next route.
- Keep routine findings in `todo.md`; edit the source idea only if source
  intent changes or separate follow-up work must be recorded.
- Use focused Sema/HIR-facing tests for stale rendered enum, consteval,
  type-binding, NTTP, layout, or local-const names.
- If a path belongs to parser, broad HIR lowerer cleanup, BIR, LIR, or backend
  ownership, record it as follow-up scope instead of expanding this runbook.
- For code-changing steps, prove with a fresh build plus the
  supervisor-selected focused CTest subset; escalate to broader validation
  after shared Sema or consteval identity interfaces change.

## Step 1: Inventory Sema Compatibility Routes

Goal: build a current, grep-friendly map of Sema-owned rendered-string
compatibility authority.

Primary targets:
- Static-eval rendered enum compatibility helpers.
- Rendered type-binding and NTTP bridge maps.
- Rendered consteval function lookup.
- Interpreter local const `by_name` mirrors.
- Rendered struct layout handoff through `ConstEvalEnv::struct_defs`.

Actions:
- Inspect Sema, static-eval, consteval, interpreter, and HIR-facing support
  comments that mention compatibility, legacy, deprecated, rendered,
  no-metadata, enum, NTTP, type binding, consteval function, local const, or
  struct defs.
- Classify each retained path as metadata-rich semantic lookup, no-metadata
  compatibility, diagnostic/display/source payload, final-output spelling, or
  separate HIR follow-up work.
- Identify the first narrow stale-rendered Sema path that can prove
  fail-closed behavior before or alongside conversion.

Completion check:
- `todo.md` records the Sema compatibility inventory and first conversion or
  fencing target, with no source idea rewrite.

## Step 2: Fence Static-Eval Enum Compatibility

Goal: stop metadata-rich enum constant evaluation from recovering through
rendered enum names after structured enum metadata misses.

Primary targets:
- `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility`
- `static_eval_int_with_rendered_enum_compatibility`

Actions:
- Prefer structured enum identity, domain-scoped `TextId`, and owner-aware
  enum metadata where already available.
- Delete rendered enum fallback when no production no-metadata caller needs it.
- Otherwise fence rendered enum lookup as explicit legacy/no-metadata
  compatibility with owner, limitation, and removal condition.
- Add focused proof that stale rendered enum names cannot override complete
  structured enum identity.

Completion check:
- Covered static-eval enum routes fail closed after complete structured misses,
  and narrow proof is recorded in `todo.md`.

## Step 3: Convert Type-Binding And NTTP Bridges

Goal: prevent rendered type-binding or NTTP bridge maps from acting as semantic
authority after complete structured binding metadata misses.

Primary targets:
- `same_rendered_type_name_compatibility`
- Rendered type-binding bridges in consteval substitution.
- Rendered NTTP output mirrors and `nttp_bindings`.
- `type_binding_text_ids_by_name`
- `type_binding_keys_by_name`

Actions:
- Route metadata-rich type-binding and NTTP callers through structured
  binding keys, domain-scoped `TextId` carriers, and owner-aware template
  metadata.
- Fence retained rendered mirrors as no-metadata compatibility or display /
  source payload state only.
- Add stale rendered type-binding or NTTP proof for at least one converted or
  fenced route.

Completion check:
- Covered metadata-rich type-binding and NTTP paths do not recover through
  rendered-name maps after complete structured misses.

## Step 4: Fence Consteval Function And Local Const Lookup

Goal: ensure consteval function and interpreter local-const lookup use
structured binding metadata before rendered names.

Primary targets:
- Rendered `consteval_fns` compatibility lookup after structured consteval
  metadata checks.
- Interpreter `by_name` local const compatibility mirrors.

Actions:
- Prefer structured consteval binding metadata and route-local local-const
  identity when present.
- Fail closed after complete structured misses instead of falling through to
  rendered consteval function or local-const maps.
- Fence retained rendered lookup as legacy/no-metadata compatibility with a
  concrete removal condition.
- Add focused stale rendered consteval or local-const proof.

Completion check:
- Covered consteval and interpreter lookup routes do not recover through stale
  rendered names after complete structured misses.

## Step 5: Audit Struct-Def And Layout Handoff Compatibility

Goal: prevent rendered struct-def layout handoff from acting as semantic
authority when Sema has complete structured layout identity.

Primary targets:
- Rendered `ConstEvalEnv::struct_defs` fallback for legacy layout handoff.
- Sema-facing layout or struct-def compatibility helpers discovered during
  inventory.

Actions:
- Prefer direct structured record/layout metadata and domain-aware identity
  for metadata-rich layout callers.
- Fence retained rendered struct-def lookup as legacy/no-metadata handoff
  compatibility with owner, limitation, and removal condition.
- Add focused proof that stale rendered layout or struct-def names cannot
  override complete structured identity.

Completion check:
- Covered Sema layout handoff routes fail closed after complete structured
  misses, and retained rendered struct-def use is explicitly no-metadata or
  diagnostic/display scope.

## Step 6: Closure Ledger And Broader Sema Proof

Goal: leave a reviewer-auditable ledger for Sema compatibility retirement.

Actions:
- Summarize deleted, converted, fenced, and intentionally retained Sema
  compatibility routes in `todo.md`.
- State residual parser, HIR, BIR, LIR, or backend work as separate follow-up
  scope instead of hidden plan expansion.
- Run the supervisor-selected broader Sema/HIR-facing validation for the final
  compatibility retirement slice.

Completion check:
- The ledger covers enum, type-binding, NTTP, consteval function, local const,
  and struct-def/layout compatibility routes.
- Focused proofs cover structured success, stale-rendered fail-closed
  behavior, and any retained no-metadata compatibility path.
