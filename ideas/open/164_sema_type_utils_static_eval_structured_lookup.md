# Sema Type Utils Static Eval Structured Lookup

Status: Open
Created: 2026-05-11

Parent Ideas:
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/158_sema_validate_string_authority_audit.md`

## Goal

Clean the remaining small sema `type_utils` static-eval string lookup path.

`src/frontend/sema/type_utils.cpp` still exposes `static_eval_int` with
`unordered_map<string, long long>` enum constants and looks up `n->name` by
rendered spelling. This should become a structured/TextId-aware helper, with
rendered-name lookup retained only for no-metadata compatibility callers.

## Why This Idea Exists

The larger sema validate and consteval authority work is closed, but
`type_utils` is a small residual edge. It is easy for it to reintroduce the
old behavior: evaluating enum/default expressions by rendered string even when
the AST has TextId or structured enum metadata.

## In Scope

- Inventory all callers of `static_eval_int` and classify whether they can
  supply TextId or structured enum constant maps.
- Add a TextId/structured-key path for enum constant lookup in `type_utils`.
- Prefer the structured path when metadata is complete.
- Treat rendered `unordered_map<string, long long>` as a documented
  compatibility bridge for legacy/no-metadata callers.
- Add focused tests where same-spelled enum constants in different structured
  domains do not collide in `type_utils` static evaluation.

## Out Of Scope

- Reworking the full consteval interpreter, already handled by idea 159.
- Reopening validate table ownership from idea 158.
- Removing diagnostic or source spelling strings.
- Large HIR/backend changes.

## Acceptance Criteria

- `static_eval_int` has a structured/TextId-aware enum constant lookup path.
- Covered callers pass structured metadata and fail closed on complete
  structured misses.
- The string map is documented as a compatibility bridge with removal
  conditions.
- Focused sema tests cover the stale/same-spelled enum case.

## Reviewer Reject Signals

- The normal covered path still does `enum_consts.find(n->name)`.
- A raw `TextId` is used without enum/domain context where scope matters.
- Tests only update diagnostics or rendered names.
