# Sema Type Utils Static Eval Structured Lookup

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/158_sema_validate_string_authority_audit.md`

Follow-up Ideas:
- `ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md`

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

## Completion Summary

Completed through the active runbook ending at Step 6.

- Added structured/TextId-aware enum lookup input for `static_eval_int`.
- Made complete structured lookup take authority over rendered-name lookup for
  converted covered paths, with rendered lookup retained as a documented
  no-metadata compatibility bridge.
- Converted covered global/static-member paths, including non-template static
  constexpr struct member initializer evaluation, static member declaration
  initializer fallback, and NTTP-backed static member evaluation paths.
- Added focused collision coverage proving same-spelled enum constants in
  distinct structured domains do not collapse through shared rendered spelling.
- Recorded removal conditions for the remaining rendered compatibility bridge.

Close-time proof:

- Step 5 focused collision proof passed 1/1 before the final proof ledger:
  `cpp_hir_sema_consteval_type_utils_structured_metadata`.
- Focused final proof passed 7/7:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_alias_deferred_nttp_static_member|cpp_positive_sema_template_constexpr_member_runtime_cpp)$' > test_after.log`
- Accepted full-suite baseline after the code slices was available in
  `test_baseline.log` with 3135 passing and 0 failing tests.

## Residual Scope Split

Local/block enum-scope structured mirrors are not required to satisfy this
idea's covered global/static-member scope. They remain useful follow-up work
because they can reduce reliance on mutable `enum_consts_` save/restore
compatibility behavior. That residual scope is split to
`ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md`
instead of blocking closure here.

## Reviewer Reject Signals

- The normal covered path still does `enum_consts.find(n->name)`.
- A raw `TextId` is used without enum/domain context where scope matters.
- Tests only update diagnostics or rendered names.
