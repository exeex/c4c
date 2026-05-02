# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed the template default-expression re-lex retry from the structured
direct template-primary instantiation path in `parse_base_type()`. Both the
preliminary specialization-selection fill and the later concrete-argument fill
now use `eval_deferred_nttp_default(...)` cached token metadata for defaulted
NTTP expressions; if that structured carrier is missing, the instantiation is
left incomplete/deferred instead of re-lexing
`primary_tpl->template_param_default_exprs[...]` or manufacturing `$expr:`
source text.

Focused drift coverage now poisons the rendered default-expression text on
`is_signed_helper`, drives the direct `parse_base_type()` path for
`is_signed_helper<int>`, and requires the materialized defaulted NTTP value to
come from structured cached tokens. The same coverage still proves dependent
member-typedef base instantiation attaches `record_def` before Sema despite
rendered metadata drift.

## Suggested Next

Continue Step 4 by reviewing the remaining parser/Sema `$expr:` compatibility
routes that are not default-expression re-lex fallbacks in the direct
template-primary or structured base-handoff paths.

## Watchouts

- The source idea remains active; Step 4 progress is not source-idea closure.
- Ordinary global-scope C++ overload declarations are still a separate blocker
  because the current AST handoff does not let Sema distinguish them from C
  global redeclarations without weakening C behavior. Preserve C and
  `extern "C"` rejection until a structured language/linkage carrier exists.
- Static-member lookup now depends on parser/Sema owner and member metadata.
  A generated `NK_VAR` for `Owner::member` must carry `qualifier_text_ids`,
  `unqualified_text_id`, and the correct owner namespace context when the
  parser has the owner identity. Do not restore rendered owner lookup to cover
  missing carriers.
- `tpl_struct_origin` remains a compatibility/display spelling and is still
  passed to template-instantiation helpers for emitted names. Do not use it to
  rediscover template primaries or decide Sema `TypeSpec` equivalence when
  `tpl_struct_origin_key` is present.
- The directly adjacent parser change in
  `src/frontend/parser/impl/expressions.cpp` is deliberately narrow: it only
  avoids splitting a concrete instantiated owner TextId when no
  `tpl_struct_origin` carrier is present. Dependent template-origin routes now
  also have `tpl_struct_origin_key` where parser producers already know the
  primary key.
- Retained `$expr:` carriers and template default-expression re-lex paths are
  compatibility fallbacks, not completed structured cleanup. Successful Step 4
  routes should continue to use parser/Sema structural metadata where present.
- The structured base-instantiation typed-argument path no longer re-lexes
  `template_param_default_exprs` after `eval_deferred_nttp_default(...)` misses.
  If a future case needs that route, add or repair the cached token/default
  metadata producer instead of restoring source-string evaluation.
- The direct template-primary instantiation path now has the same policy:
  defaulted NTTP expressions require cached default-token metadata, and a miss
  leaves the instantiation incomplete/deferred rather than reopening rendered
  `template_param_default_exprs` text.
- The rendered base-arg reconstruction path still contains a no-carrier
  compatibility default-expression re-lex fallback. It is explicitly behind the
  absence of `tpl_struct_args`; do not use it as structured progress.
- The alias-template comma-split/rendered arg-ref fallback still exists only for
  template-arg lists with unstructured debug-only refs and no structured
  carrier. Do not restore it as a recovery path for mixed carrier lists or stale
  `TemplateArgRef::debug_text`.
- `zero_value_arg_ref_uses_debug_fallback(...)` still represents legacy
  compatibility policy elsewhere; this packet only made the alias-template
  fallback gate treat `nttp_text_id` as structured carrier metadata.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build succeeded. CTest passed `886/886` matched tests, including
`frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
