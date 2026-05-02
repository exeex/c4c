# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed direct template-instantiation authority from explicit value-arg
`$expr:` display strings in the preliminary binding pass. `parse_base_type()`
now evaluates explicit NTTP args through structured `ParsedTemplateArg::expr`
or `nttp_text_id` metadata first, clears stale display names once a value is
resolved, and gates the legacy `lex_template_expr_text(actual.nttp_name + 6,
...)` path to no-carrier args with neither parsed expression nor TextId.

Focused drift coverage now injects an explicit NTTP expression whose parsed
token kind evaluates true while its `$expr:` display spelling says `false`; the
direct instantiation must materialize the `true` value from structured parser
expression metadata.

## Suggested Next

Continue Step 4 by reviewing the remaining `lex_template_expr_text(...)`
callers and separating true no-carrier compatibility from paths where
`TemplateArgRef`, cached default tokens, parsed expressions, or TextId metadata
already exist.

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
- Retained `$expr:` carriers outside this base-instantiation handoff are
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
- The rendered base-arg reconstruction path is removed for the Step 4 base
  handoff. The only remaining fallback in that local area is rendered-tag map
  lookup after instantiation fails to attach `TypeSpec::record_def`; do not use
  that map lookup as template-argument semantic authority.
- Direct template-instantiation explicit value args now have a structured
  parsed-expression/TextId first path. The remaining `$expr:` re-lex branch in
  that preliminary binding pass is compatibility-only for captured expression
  text with no `ParsedTemplateArg::expr` and no `nttp_text_id`; a future packet
  should repair the capture producer if a high-value case still reaches it.
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
