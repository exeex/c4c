Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Alias-Template Parameter Payloads Structured-Primary

# Current Packet

## Just Finished

Plan Step 1 `Classify AST and Template Payload String Authority` completed as
a read-only source inspection packet. Implementation files were not edited.

Classification map:

- `ParserAliasTemplateInfo::param_names`: `structured-primary bridge` with
  remaining string authority. Writers in `declarations.cpp` store names beside
  `param_name_text_ids`, `param_is_nttp`, pack/default metadata, and the
  aliased `TypeSpec`. The alias-template substitution path in
  `types/base.cpp` already prefers `param_name_text_ids` for keying, but still
  reads `param_names` for fallback TextId derivation, generated argument
  rendering, and binding-map names. First semantic consumers are alias-template
  parsing/substitution in `types/base.cpp` and parser template helpers that
  still compare `template_param_names` by string.
- `ParserTemplateArgParseResult::nttp_name`: `semantic string authority` for
  unresolved NTTP identifiers and deferred `$expr:` payloads. Writers in
  `types/declarator.cpp` set it to identifier spelling, `nullptr` for literals
  and type args, or `$expr:<tokens>` for captured expressions. Parser writers
  copy it into `Node::template_arg_nttp_names`; parser/HIR consumers use the
  string to bind NTTP arguments, evaluate deferred expressions, select
  specialization keys, and render template arg refs.
- `$expr:` text copied into `template_arg_nttp_names`: `semantic string
  authority` plus `generated spelling`. Writers in `expressions.cpp`,
  `declarations.cpp`, and `types/struct.cpp` project parsed template args onto
  AST nodes. First consumers include `sema/consteval.cpp`,
  `hir/impl/templates/deduction.cpp`, `hir/impl/templates/value_args.cpp`, and
  `hir/impl/expr/call.cpp`, where strings index NTTP binding maps or feed
  deferred expression evaluators.
- `TemplateArgRef::debug_text`: `diagnostic/debug-only` by declaration
  comment, but currently `semantic string authority` in template
  instantiation/materialization. Parser writers in `types/base.cpp` render
  structured args into debug text for `tpl_struct_args`; HIR writers preserve
  or regenerate it. Consumers in `parser/ast.hpp`, parser
  `types/types_helpers.hpp`, and HIR template code use it for dependency
  detection, canonical arg keys, binding-map lookup, NTTP expression handling,
  and fallback type substitution.
- `Node::name`: mixed `generated spelling` and semantic boundary identity.
  Parser writers set declaration/ref/record names, often namespace-qualified or
  mangled after structured lookup. Sema/HIR still use it for symbol lookup,
  struct/template registration, generated symbol names, and diagnostics. It is
  not display-only.
- `Node::unqualified_name`: `structured-primary bridge` plus local fallback.
  Parser fills it with source base spelling and `unqualified_text_id` through
  qualified-name/declaration namespace helpers. First semantic consumers use it
  for local symbol fallback after parser canonicalization, HIR text-id
  reconstruction, template primary lookup, and constructor/delegating ctor
  matching.
- `Node::template_origin_name`: `semantic string authority` and generated
  template-family spelling. Parser record/template paths write it for
  specializations and instantiated methods. HIR registers specializations,
  finds primary templates, resolves template family roots, and matches
  constructors from this string.
- `TypeSpec::tag`: `semantic string authority` for type identity today, with
  many generated-spelling cases. Parser stores typedef/record tags, mangled
  template instantiation names, and deferred owner spellings here; Sema/HIR use
  it for type comparison, template param detection, struct/member lookup, and
  mangling.
- `TypeSpec::tpl_struct_origin`: `fallback-only structured bridge` for pending
  template struct owners, but still string-addressed. Parser writes primary
  template family spelling for unresolved template-dependent structs. HIR
  defers, canonicalizes, and resolves pending template types by finding primary
  templates from this string.
- `TypeSpec::deferred_member_type_name`: `semantic string authority` for
  deferred member typedefs. Parser writes member names for `Owner::type`-style
  cases; HIR resolves pending member typedefs by pairing this member string
  with `tag` or `tpl_struct_origin`.

## Suggested Next

Execute `plan.md` Step 2 as a bounded alias-template parameter payload packet.
Focus on `ParserAliasTemplateInfo::param_names`, `param_name_text_ids`,
alias-template parsing in `declarations.cpp`, substitution/projection in
`types/base.cpp`, and focused parser tests. Convert remaining alias-template
substitution bindings to use `param_name_text_ids`, `TemplateParamId`, `TextId`,
parser symbol ids, or existing structured template parameter records wherever
both sides have structured identity. Leave `param_names` only for display,
generated spelling, or an explicit no-TextId compatibility fallback.

## Watchouts

The plan names `src/frontend/ast/ast.hpp`, but this checkout stores the AST
payload definitions in `src/frontend/parser/ast.hpp`. The highest-risk paths
for later packets are `TemplateArgRef::debug_text`, `$expr:` NTTP strings, and
`TypeSpec::deferred_member_type_name`; they are still used for actual
substitution or lookup, not just dumps. Do not collapse all string payloads at
once: `TypeSpec::tag` and `Node::name` are broad cross-stage identity carriers
and need smaller follow-up slices.

## Proof

Classification-only packet; no build or test proof required and no
`test_after.log` was generated.
