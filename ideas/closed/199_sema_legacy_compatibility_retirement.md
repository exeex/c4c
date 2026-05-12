# Sema Legacy Compatibility Retirement

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/open/196_hir_pending_consteval_structured_identity.md`

## Goal

Retire or hard-fence the remaining Sema-owned legacy compatibility paths so
semantic analysis and consteval support prefer structured keys, `TextId`
carriers, and owner-aware domains without reopening rendered-string fallback on
metadata-rich misses.

## Why This Idea Exists

Sema already has structured authority for the main migrated domains: record and
enum lookup, canonical symbol identity, consteval binding metadata, template
owner keys, and static-eval support. However, several rendered-name bridges
remain for no-metadata callers, legacy `TypeSpec` tags, rendered NTTP mirrors,
and compatibility maps exposed to HIR or consteval replay.

This idea scopes a Sema-only cleanup pass after ideas 195 and 196 classify the
remaining frontend-to-BIR lookup surfaces.

## In Scope

- Inventory Sema compatibility paths already visible in source comments and
  closure ledgers, including:
  - `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility`
  - `static_eval_int_with_rendered_enum_compatibility`
  - `same_rendered_type_name_compatibility`
  - legacy rendered type-binding bridges in consteval substitution
  - rendered NTTP output mirrors and `nttp_bindings`
  - `type_binding_text_ids_by_name` / `type_binding_keys_by_name` rendered-name
    bridge maps
  - rendered `consteval_fns` compatibility lookup after structured consteval
    metadata checks
  - interpreter `by_name` local const compatibility mirrors
  - rendered `ConstEvalEnv::struct_defs` fallback for legacy layout handoff
- Delete Sema compatibility helpers that no production metadata-rich caller
  still needs after ideas 195 and 196.
- For retained no-metadata compatibility, require fail-closed behavior whenever
  complete structured metadata was present but missed.
- Add focused Sema/HIR-facing tests for stale rendered enum, consteval,
  type-binding, or layout names.
- When a newly discovered Sema compatibility path cannot be removed in this
  idea, add a nearby source comment containing `legacy` or `deprecated`, plus
  owner, limitation, and removal condition.

## Out Of Scope

- Parser syntax-carrier cleanup.
- HIR registry/lowerer cleanup outside the Sema-facing boundary.
- BIR, LIR, or backend compatibility retirement.
- Removing diagnostic/display names, ABI spelling, or source payload strings.
- Rewriting the complete consteval engine or template substitution model.

## Acceptance Criteria

- Covered Sema metadata-rich paths use structured keys or `TextId`-domain
  carriers before rendered strings.
- Complete structured misses in covered Sema paths do not recover through
  rendered enum maps, consteval function maps, NTTP maps, or struct-def maps.
- Any retained Sema compatibility bridge is explicitly no-metadata,
  diagnostic/display, source payload, or deprecated follow-up surface.
- Newly discovered retained Sema bridges have source comments containing
  `legacy` or `deprecated` and a concrete removal condition.
- Focused tests prove stale rendered Sema names cannot override structured
  identity.

## Reviewer Reject Signals

- A rendered Sema fallback remains reachable after a complete structured miss
  without an explicit no-metadata boundary.
- A raw `TextId` is treated as sufficient cross-domain semantic authority
  without owner or domain context.
- A newly discovered retained Sema bridge lacks a `legacy` or `deprecated`
  comment.
- Tests only update diagnostics, rendered names, or expected text.
- The route expands into parser, HIR, BIR, or backend work instead of staying
  Sema owned.

## Closure Ledger

Closed after the active six-step runbook completed its Sema-owned scope.
Metadata-rich enum, type-binding, NTTP, consteval function, local const, and
struct-def/layout paths now use structured key, `TextId`, or owner metadata as
semantic authority before rendered compatibility. Complete structured misses in
covered routes fail closed before rendered maps can recover stale names.

No production Sema compatibility route was fully deleted because retained
rendered bridges still serve explicit no-metadata, diagnostic/display,
source-payload, final-spelling, or HIR-boundary callers. Retained bridges were
fenced as legacy or deprecated compatibility with owner, limitation, and
removal-condition comments.

Residual parser syntax-carrier cleanup, broad HIR registry/lowerer cleanup,
BIR/LIR/backend compatibility retirement, and final-output spelling work remain
separate follow-up scope.

Close proof: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
"^(frontend_parser_tests|cpp_hir_.*structured_metadata|cpp_positive_sema_)"`
passed `923/923`; `c4c-regression-guard` comparison of `test_before.log` and
`test_after.log` passed with no new failures.
