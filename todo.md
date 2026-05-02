# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 tightened the parser expression handoff for
`Template<Args>::member`: when `parse_base_type()` exposes structured template
owner metadata, the generated member reference now carries the materialized
owner `TextId` from `record_def` or `tag_text_id` and does not synthesize
qualifier TextIds by splitting rendered owner text after a structured miss.
The route still preserves `tpl_struct_origin` as display/HIR spelling. Added
lookup-authority coverage that corrupts rendered owner/member spelling for
`Trait<int>::value` while preserving parser-carried owner/member TextIds and
verifies Sema resolves through the structured metadata.

## Suggested Next

Continue Step 4 with the next parser-to-Sema metadata handoff gap selected by
the supervisor, prioritizing any remaining route where an available
`QualifiedNameKey`, `TextId`, `TemplateArgRef`, `record_def`, or `tag_text_id`
carrier is still shadowed by rendered compatibility text.

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
- Template static-member expression handoff now keeps `tpl_struct_origin` only
  as display/HIR spelling. If the parsed owner has `tpl_struct_origin_key` or
  structured arg carriers but no materialized owner `TextId`, the parser leaves
  the qualifier metadata absent instead of manufacturing it from rendered text.
  Repair the missing structured carrier rather than reintroducing splitting.
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
  handoff. The Step 4 base-instantiation path now gates rendered-tag map
  recovery behind absence of structured carriers. If structured carriers exist,
  the handoff validates the instantiated node's origin and concrete args before
  attaching `record_def`; do not weaken that check into rendered-name authority.
- The base-instantiation caller now passes a fresh output `TypeSpec` to
  `ensure_template_struct_instantiated_from_args(...)`. Do not change that back
  to passing `inst->base_types[bi]` directly: when the existing carrier already
  has a tag, the producer cannot populate its normal returned `record_def`
  fallback. Consume the returned `TypeSpec` wholesale and preserve only
  deferred-member lookup metadata.
- Direct template-instantiation explicit value args now have a structured
  parsed-expression/TemplateArgRef-expression/TextId first path. The remaining
  `$expr:` re-lex branch in that preliminary binding pass is compatibility-only
  for captured expression text with no `ParsedTemplateArg::expr`, no
  value-arg `TypeSpec::array_size_expr`, and no `nttp_text_id`; a future packet
  should repair the capture producer if a high-value case still reaches it.
- The alias-template comma-split/rendered arg-ref fallback has been removed.
  The only exercised fully no-carrier `TB_VOID` debug-only type-ref case is now
  the focused parser lookup-authority fixture, which intentionally remains
  debug-only; no real producer was exposed by the delegated proof subset.
- Origin-key materialization is deliberately conservative: it uses simple
  structured args only, rejects debug-only refs, nested template-origin args,
  and unevaluated NTTP expression carriers, and preserves deferred member
  metadata while attaching `record_def`.
- Member-typedef lookup now has the same conservative origin-key materializer
  for `tpl_struct_origin_key` plus simple structured `tpl_struct_args`; it
  rejects debug-only type refs, nested template-origin args, and unevaluated
  NTTP expression carriers instead of reopening rendered owner/template text.
- Simple type-only pending template args now use `TemplateArgRef` structure.
  Nested template-origin args still stay on the legacy display path to avoid
  recursive canonical type keys.
- `zero_value_arg_ref_uses_debug_fallback(...)` still represents legacy
  compatibility policy elsewhere; recent packets made the alias-template
  fallback gate treat `nttp_text_id` and resolved template-origin carriers as
  structured metadata.
- Do not reintroduce rendered qualified-text parsing, `$expr:` debug-text
  semantic authority, string/string_view semantic lookup parameters, fallback
  spelling, expectation downgrades, or named-test shortcuts.
- If a handoff requires a cross-module carrier outside parser/Sema ownership,
  record a separate metadata idea instead of expanding idea 139.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build succeeded, rebuilding `expressions.cpp` and dependent frontend
targets.
CTest passed `886/886` matched tests, including
`frontend_parser_lookup_authority_tests`,
`eastl_cpp_external_utility_frontend_basic_cpp`, and the
`cpp_positive_sema_.*` subset. Final proof log path: `test_after.log`.
