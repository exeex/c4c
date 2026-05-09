Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Deferred Text Payload And Key Paths

# Current Packet

## Just Finished

Completed Step 1 inventory for deferred syntax text payload and semantic-key
paths. Classification from focused reads/searches:

- Legitimate syntax payloads: `ParserTemplateArgParseResult::captured_expr_tokens`
  and `cache_nttp_default_expr_tokens` / `nttp_default_expr_tokens_by_key`
  preserve deferred NTTP/template-argument expression tokens keyed by
  `QualifiedNameKey + param_idx`.
- Legitimate parsed-expression carriers: `template_arg_exprs` and
  `TypeSpec::array_size_expr` provide structured expression carriers before
  string fallback.
- Legitimate debug/display mirrors: `TemplateArgRef::debug_text`,
  `encode_template_arg_debug_ref`, `encode_template_arg_debug_list`,
  `template_arg_debug_text_at`, `format_pending_type_ref_for_display`, and
  `encode_pending_type_ref` are documented or shaped as diagnostics/display
  text.
- Retained compatibility mirrors: `template_param_default_exprs`,
  `template_arg_nttp_names`, `template_origin_name`,
  `deferred_member_type_name`, `tpl_struct_origin`, and HIR pending
  `display_key` remain string mirrors/legacy bridges when structured
  carriers are unavailable.
- Structured semantic carriers already present: `TextId` fields for template
  params/NTTP args/member typedefs, `QualifiedNameKey` owner/origin keys,
  `PendingTemplateTypeKey`, NTTP text/key binding maps, and record/qualifier
  metadata.
- Suspicious semantic-key paths: `$expr:` strings are still re-lexed/evaluated
  as semantic deferred NTTP expressions after structured carriers miss in
  `src/frontend/parser/impl/types/base.cpp` and HIR materialization/value-arg
  paths. String-key NTTP/name maps remain compatibility fallbacks after TextId
  lookup misses in Sema/HIR, but are more bounded because they are binding-map
  bridges rather than raw syntax expression replay.

First repair target: demote `$expr:` raw-string replay so structured
`captured_expr_tokens`, `template_arg_exprs`, or keyed deferred-default tokens
are required before evaluating deferred NTTP expressions. Start with
`src/frontend/parser/impl/types/base.cpp` around the explicit
`lex_template_expr_text(actual_args[pi].nttp_name + 6, ...)` fallback, then
mirror the rule in `src/frontend/hir/impl/templates/value_args.cpp` and
`src/frontend/hir/impl/templates/materialization.cpp` if the parser-side
change leaves HIR `$expr:` evaluation reachable.

## Suggested Next

Implement the first repair packet: require structured token/expression
carriers for deferred NTTP expression evaluation and demote `$expr:` string
replay to display/compatibility only. Keep legitimate token payloads and
diagnostic/debug spelling intact.

## Watchouts

- `capture_template_arg_expr_text` itself is syntax capture/display text; the
  hazard is consumers re-lexing `$expr:` text as semantic input after carriers
  miss.
- `deferred_member_type_name` has structured owner/member TextId metadata in
  current inspected paths; retain it as display/diagnostic compatibility unless
  a concrete text-only lookup is proven.
- `TemplateArgRef::debug_text` is still used in formatting and legacy
  materialization. Do not delete it wholesale; make semantic users prove a
  structured carrier first.
- Initial focused validation subset for the repair should cover parser lookup
  authority and HIR template NTTP/materialization paths:
  `ctest --test-dir build -R "(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case)" --output-on-failure`.

## Proof

Inventory-only packet; no build required and `test_after.log` was not touched.
Inspected with:

- `rg -n "capture_template_arg_expr|eval_captured_template_arg_expr_tokens|eval_deferred_nttp_default|eval_deferred_nttp_expr_tokens|cache_nttp_default_expr_tokens" src/frontend/parser -S`
- `rg -n "template_arg_debug_text_at|encode_template_arg_debug_ref|encode_template_arg_debug_list|set_template_arg_debug_refs_text|template_arg_refs_text|rendered_template_arg_refs_from_parsed_args|render_template_arg_ref|debug_text" src/frontend/parser src/frontend/sema -S`
- `rg -n "deferred_member_lookup_name|resolve_deferred_member|deferred_member_type_name|deferred_member_type_text_id|deferred_member_type_owner_key" src/frontend/parser src/frontend/sema -S`
- `rg -n "deferred_nttp_expr_text\\(|is_deferred_nttp_expr_ref\\(|kDeferredNttpExprRefPrefix|lex_template_expr_text\\(|\\$expr:" src/frontend -S`
- `rg -n "deferred|nttp|template_arg|debug_text|captured_expr|expr_text|template_param_default_exprs|deferred_member_type_name|deferred_member_type_text_id|QualifiedNameKey|TextId" src/frontend/hir -S`
- Focused ranges: `src/frontend/parser/ast.hpp:180-206`,
  `src/frontend/parser/ast.hpp:410-432`,
  `src/frontend/parser/ast.hpp:705-760`,
  `src/frontend/parser/parser_types.hpp:143-151`,
  `src/frontend/parser/impl/types/types_helpers.hpp:2057-2235`,
  `src/frontend/parser/impl/types/types_helpers.hpp:2268-2274`,
  `src/frontend/parser/impl/types/declarator.cpp:984-1024`,
  `src/frontend/parser/impl/types/template.cpp:1060-1110`,
  `src/frontend/parser/impl/types/template.cpp:1680-1785`,
  `src/frontend/parser/impl/declarations.cpp:2455-2510`,
  `src/frontend/parser/impl/types/base.cpp:1188-1245`,
  `src/frontend/parser/impl/types/base.cpp:1335-1705`,
  `src/frontend/parser/impl/types/base.cpp:5715-5885`,
  `src/frontend/sema/consteval.cpp:180-310`,
  `src/frontend/sema/consteval.cpp:780-870`,
  `src/frontend/sema/type_utils.cpp:630-675`,
  `src/frontend/sema/validate.cpp:1680-1785`,
  `src/frontend/hir/compile_time_engine.hpp:360-550`,
  `src/frontend/hir/compile_time_engine.hpp:620-745`,
  `src/frontend/hir/hir_types.cpp:428-445`,
  `src/frontend/hir/hir_types.cpp:1170-1185`,
  `src/frontend/hir/impl/templates/value_args.cpp:735-775`,
  `src/frontend/hir/impl/templates/materialization.cpp:575-705`,
  `src/frontend/hir/impl/templates/materialization.cpp:745-765`,
  `src/frontend/hir/impl/expr/call.cpp:770-810`.
