Status: Active
Source Idea Path: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconcile Existing Follow-Up Queue

# Current Packet

## Just Finished

Step 1 reconciled the existing open follow-up queue against the suspicious
Parser, Sema, and HIR string-authority paths named by idea 131.

Coverage found:

- Parser record/template lookup mirrors are covered by idea 132:
  `ParserDefinitionState::defined_struct_tags`,
  `ParserDefinitionState::struct_tag_def_map`,
  `ParserTemplateState::template_struct_defs`,
  `ParserTemplateState::template_struct_specializations`,
  `ParserTemplateState::instantiated_template_struct_keys`, and
  `ParserTemplateState::nttp_default_expr_tokens`.
- Parser namespace and visible-name compatibility spelling are covered by idea
  133: `Parser::VisibleNameResult::compatibility_spelling`,
  `ParserNamespaceState::UsingValueAlias::compatibility_name`,
  `compatibility_namespace_name_in_context`, and string overloads of namespace
  and visible-name lookup helpers.
- Parser-to-AST and AST-to-Sema/HIR payload bridges are covered by idea 134:
  `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`, `$expr:` text copied into
  `template_arg_nttp_names`, `TemplateArgRef::debug_text`, `Node::name`,
  `Node::unqualified_name`, `Node::template_origin_name`, `TypeSpec::tag`,
  `TypeSpec::tpl_struct_origin`, and
  `TypeSpec::deferred_member_type_name`.
- Sema owner and static-member lookup authority is covered by idea 135:
  `resolve_owner_in_namespace_context`, `enclosing_method_owner_struct`,
  `lookup_struct_static_member_type`, and the unqualified variable lookup
  fallback that can recover from `n->name` when `n->unqualified_name` should be
  the structured AST spelling authority.
- HIR structured record/template lookup authority is covered by idea 136:
  `parse_scoped_function_name`, out-of-class method ownership derived from
  rendered `fn->name` or `n->name`, scoped static-member lookup derived from
  rendered names, rendered template struct primary and specialization lookup
  including owner-parity-only checks, and struct method/static-member/member
  lookup where rendered maps return results while owner-key maps only check
  parity.

Queue assessment:

- No duplicate open follow-up ideas were found among 132-136 for the same
  primary ownership slice.
- No Parser, Sema, or HIR gaps were found from the suspicious paths already
  named by ideas 131 and the active runbook.
- Idea 134 intentionally overlaps with ideas 135 and 136 at the AST consumer
  boundary, but the split is not duplicate work: 134 owns parser-produced
  payload fields, while 135 and 136 own Sema/HIR lookup consumers.
- Ideas 132 and 136 are broad within their respective Parser and HIR lookup
  families, but their scopes name concrete suspicious paths and acceptance
  criteria, so they do not need splitting before Step 2.

## Suggested Next

Proceed to Step 2: audit remaining Parser and AST-boundary text authority
outside the already covered ideas 132, 133, and 134. Cross-check any suspicious
parser finding against those ideas before asking the plan owner to create or
adjust follow-up idea coverage.

## Watchouts

Do not create new Parser/Sema/HIR follow-up ideas for the Step 1 suspicious
paths unless Step 2 or Step 3 finds a concrete uncovered path outside ideas
132-136. Treat idea 134's boundary overlap with ideas 135 and 136 as
coordination, not duplication.

## Proof

Not run per packet: no build proof required for read-only idea queue audit and
`todo.md` update; tests were explicitly out of scope.
