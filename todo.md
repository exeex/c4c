Status: Active
Source Idea Path: ideas/open/129_parser_intermediate_carrier_boundary_labeling.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Audit parser-side string authority

# Current Packet

## Just Finished

Step 3: Audited parser-side string authority without behavior changes.

Local-only parser strings:
- `ParserCoreInputState::source_file` in
  `src/frontend/parser/impl/parser_state.hpp` is parser input/diagnostic
  context, not semantic name authority.
- `ParserParseContextFrame::function_name`, `ParserParseFailure::{function_name,
  expected, got, detail, stack_trace}`, and `ParserParseDebugEvent::{kind,
  function_name, detail}` in `src/frontend/parser/parser_types.hpp` are
  diagnostics/debug carriers.
- `ParserQualifiedNameRef::{qualifier_segments, base_name}` in
  `src/frontend/parser/parser_types.hpp` are parse-time carriers paired with
  `qualifier_text_ids`, `base_text_id`, and parser symbol ids; their rendered
  `spelled()` form is a compatibility projection, not the preferred authority.
- `ParserActiveContextState::{last_using_alias_name, last_resolved_typedef,
  current_struct_tag}` in `src/frontend/parser/impl/parser_state.hpp` are
  transient rollback-visible parser context when paired with the adjacent
  `TextId`/`QualifiedNameKey` fields.

Generated-name-only spelling:
- `Parser::try_parse_operator_function_id(std::string& out_name)` in
  `src/frontend/parser/parser.hpp` and `src/frontend/parser/impl/expressions.cpp`
  generates canonical `operator_*` names from tokens.
- Template instantiation/mangling helpers such as
  `build_template_struct_mangled_name`, `TemplateInstantiationKey::Argument::
  canonical_key`, and local `mangled`/`arg_refs` strings in
  `src/frontend/parser/impl/types/base.cpp` derive synthetic names for emitted
  parser artifacts.
- Anonymous/generated record names driven by
  `ParserDefinitionState::anon_counter` and generated template instance tags are
  not source-spelling authority, though they can become compatibility lookup
  keys after generation.

Suspicious cross-stage authority / follow-up material:
- `ParserDefinitionState::{defined_struct_tags, struct_tag_def_map}` in
  `src/frontend/parser/impl/parser_state.hpp` are rendered-tag compatibility
  mirrors; comments say semantic record lookup should prefer
  `TypeSpec::record_def`, but implementation references still pass
  `struct_tag_def_map` to constant evaluation and template instantiation paths.
- `ParserTemplateState::{template_struct_defs, template_struct_specializations,
  instantiated_template_struct_keys, nttp_default_expr_tokens}` in
  `src/frontend/parser/impl/parser_state.hpp` mirror key-based template tables
  with rendered string keys; mismatch counters indicate this remains an active
  compatibility boundary.
- `Parser::VisibleNameResult::compatibility_spelling`,
  `ParserNamespaceState::UsingValueAlias::compatibility_name`,
  `compatibility_namespace_name_in_context`, and string overloads of namespace /
  visible-name lookup in `src/frontend/parser/parser.hpp` carry rendered names
  beside `QualifiedNameKey`/`TextId` identity and can still feed AST `name`
  fields.
- `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`, and implementation uses that copy
  `$expr:` text into `template_arg_nttp_names` remain string-backed template
  payload bridges; `param_name_text_ids` and structured `TemplateArgRef` fields
  exist but do not fully replace these paths.
- Parser implementation writes rendered names into AST boundary fields such as
  `Node::name`, `Node::unqualified_name`, `Node::template_origin_name`,
  `TypeSpec::tag`, `TypeSpec::tpl_struct_origin`,
  `TypeSpec::deferred_member_type_name`, and `TemplateArgRef::debug_text`.
  Some are documented compatibility/display fields in `ast.hpp`, but current
  parser references still use them in lookup/substitution decisions, so Step 4
  should preserve these as follow-up idea material rather than treating this
  audit as cleanup-complete.

## Suggested Next

Delegate Step 4 to create narrow follow-up idea(s) for the suspicious
cross-stage string authority paths above. Suggested split: one idea for rendered
record/template lookup mirrors, and one idea for rendered AST/template payload
bridges, unless the plan owner prefers a single umbrella cleanup idea.

## Watchouts

- This packet intentionally changed no parser headers, implementation files,
  tests, `plan.md`, or source idea files.
- The suspicious paths are not all bugs; many are named compatibility mirrors.
  The risk is that rendered spelling can still decide semantic lookup after a
  structured identity exists.
- Step 4 owns idea creation under `ideas/open/`; executor authority for this
  packet did not include creating those files.

## Proof

No build or test proof was required for this read-only audit/todo update.
No tests were run and no new `test_after.log` was produced for this packet.
