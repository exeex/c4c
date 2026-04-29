Status: Active
Source Idea Path: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Audit Parser And AST Boundary Text Authority

# Current Packet

## Just Finished

Step 2 audited remaining Parser and AST-boundary text authority outside the
already covered ideas 132, 133, and 134.

Structured identity:

- `ParserSymbolTable`, `ParserNameTables`, `ParserBindingState` typedef/value
  tables, local lexical tables, enum/const-int TextId bindings, and
  `QualifiedNameKey` maps are structured identity surfaces.
- `ParserQualifiedNameRef` is structured-primary: it carries
  `qualifier_text_ids`, `qualifier_symbol_ids`, `base_text_id`, and
  `base_symbol_id`; `qualifier_segments`, `base_name`, and `spelled()` are
  projection/display text.
- `ParserNamespaceContext::text_id`, `named_namespace_children`,
  `using_namespace_contexts`, and `namespace_stack` are structured namespace
  identity; `display_name`, `canonical_name`, and `current_namespace` are
  rendered namespace display/final spelling.
- `Node::unqualified_text_id`, `Node::qualifier_text_ids`,
  `namespace_context_id`, `TypeSpec::record_def`, template-param TextId arrays,
  enum-name TextId arrays, and `TemplateArgRef` typed/value fields are
  structured AST-boundary carriers.
- `eval_const_int_with_parser_tables()` uses TextId named constants; `op` text
  in unary/binary/assignment AST nodes is legitimate operator kind spelling
  because no separate operator enum exists for those expression forms.

Legitimate display or final text:

- Diagnostics/debug carriers are display-only: parse-context stack,
  parse-failure `expected`/`got`/`detail`, parse-debug event text, token-window
  formatting, AST dump names, `node_kind_name()`, and injected-parse debug
  reasons.
- Literal and source-preservation text is final/display payload:
  `NK_STR_LIT::sval`, raw float lexemes in `sval`, char/string literal
  spelling, `sizeof...` pack text, inline-asm template/constraints, pragma
  argument strings, linkage specs, local labels/goto labels, designated-init
  fields, offsetof field paths, constructor-init member names, and anonymous
  generated namespace/template parameter names.
- Mangled/generated names are final artifact spelling, not source identity:
  operator mangled suffixes, template instantiation mangled tags,
  `append_type_mangled_suffix*()`, and generated specialization names.

Compatibility fallback already covered by ideas 132-134:

- Idea 132 covers parser record/template rendered mirrors and their consumers:
  `defined_struct_tags`, `struct_tag_def_map`, rendered template primary and
  specialization maps, instantiated-template rendered keys, and rendered NTTP
  default-expression token caches. `resolve_record_type_spec()` is structured
  first through `TypeSpec::record_def`; its rendered tag map fallback is within
  idea 132.
- Idea 133 covers parser namespace/visible-name compatibility spelling:
  `VisibleNameResult::compatibility_spelling`,
  `UsingValueAlias::compatibility_name`,
  `compatibility_namespace_name_in_context()`, `bridge_name_in_context()`,
  `qualify_name*()`, and string-returning visible/qualified lookup helpers.
- Idea 134 covers parser-produced AST/template payload strings:
  `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`,
  `template_arg_nttp_names`, `TemplateArgRef::debug_text`, `Node::name`,
  `Node::unqualified_name`, `Node::template_origin_name`, `TypeSpec::tag`,
  `TypeSpec::tpl_struct_origin`, and `TypeSpec::deferred_member_type_name`.

Compatibility fallback outside ideas 132-134:

- `eval_const_int(Node*, ..., unordered_map<string,long long>*)` is a documented
  legacy/HIR compatibility overload. Parser-owned paths use the TextId overload,
  so this is a Step 3 AST-consumer/HIR handoff rather than a new parser gap.
- `resolve_typedef_chain()` and `types_compatible_p()` are public
  string-map-compatible helpers still used by HIR with empty typedef maps. The
  parser-owned builtin path uses `Parser::are_types_compatible()` and parser
  typedef lookup, so this is also a Step 3 HIR/consumer audit item.

Suspicious authority outside ideas 132-134:

- Parser function-name authority still exposes string-facing bridges that are
  not named by ideas 132-134: `has_known_fn_name(const std::string&)`,
  `register_known_fn_name(const std::string&)`,
  `register_known_fn_name_in_context()` fallback branches, and call sites that
  register rendered `qualified_op_name`, `qualified_ctor_name`, or
  `scoped_decl_name`. The backing table is structured
  `unordered_set<QualifiedNameKey> known_fn_names`, but rendered strings can
  still be reparsed into keys and used for function/call disambiguation.
  Cross-check: this is not idea 132 record/template mirror work, not idea 133
  namespace/visible-name compatibility spelling, and not idea 134
  parser-produced AST/template payload text.

Uncovered parser/AST-boundary gap assessment:

- One uncovered parser-side gap remains: known-function-name compatibility
  spelling can still act as lookup/disambiguation authority before all parser
  callers pass `QualifiedNameKey`/`TextId` directly.
- No additional uncovered parser/AST-boundary gaps were found outside that
  known-function-name family after excluding ideas 132, 133, and 134.

## Suggested Next

Proceed to Step 3 with two handoff threads:

- Ask the plan owner to create or adjust follow-up coverage for parser
  known-function-name compatibility spelling if Step 3 is allowed to queue new
  parser follow-up work.
- Audit AST-consumer/HIR text authority for the parser-support compatibility
  helpers: the string-map `eval_const_int()` overload and
  `resolve_typedef_chain()` / `types_compatible_p()` helper use from HIR.

## Watchouts

Do not fold known-function-name cleanup into ideas 132-134; it is a distinct
parser binding/disambiguation authority family. Do not treat the public
parser-support HIR helper findings as parser implementation gaps until Step 3
checks their consumer-side authority.

## Proof

Not run per packet: no build proof required for read-only Parser/AST-boundary
text-authority audit and `todo.md` update; tests were explicitly out of scope.
