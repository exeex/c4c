Status: Active
Source Idea Path: ideas/open/130_sema_hir_ast_ingress_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Behavior And Close Audit Evidence

# Current Packet

## Just Finished

Completed Step 3 follow-up mapping for suspicious Sema/HIR string-authority
paths found in Steps 1 and 2.

Coverage map:

- Existing idea 131 covers the broad cross-IR inventory and follow-up queue for
  suspicious `std::string` / rendered-spelling authority.
- Existing idea 134 covers parser-produced AST/template payload spelling that
  reaches downstream Sema/HIR consumers, including `Node::name`,
  `Node::unqualified_name`, template-origin fields, and deferred member-type
  strings.
- New idea 135 covers Sema lookup-consumer cleanup for
  `resolve_owner_in_namespace_context`, `enclosing_method_owner_struct`,
  `lookup_struct_static_member_type`, and unqualified variable lookup fallback
  from `n->name` where `n->unqualified_name` or structured AST identity should
  be authoritative.
- New idea 136 covers HIR lookup-consumer cleanup for
  `parse_scoped_function_name`, out-of-class method ownership, scoped
  static-member lookup, rendered template primary/specialization lookup, and
  struct method/static-member/member-symbol lookup where owner-key maps are
  currently parity mirrors instead of returned authority.

All suspicious Sema/HIR ingress string-authority paths recorded by Step 1 and
Step 2 are now mapped to existing or new focused open ideas.

## Suggested Next

Proceed to Step 4: validate behavior and close audit evidence. Suggested
handoff is a docs/lifecycle-only proof check unless the supervisor chooses a
fresh build/test subset for the audit closure.

## Watchouts

Do not treat idea 134 as owning all downstream cleanup: it covers parser
payload production and AST boundary fields, while ideas 135 and 136 own the
Sema/HIR lookup consumers. Do not collapse legitimate generated names into the
same bucket as ingress authority: mangled/link names, specialization keys,
labels, diagnostics, dumps, and final emitted names are valid output/display
text.

## Proof

Not run per delegated proof: Step 3 touched only lifecycle/follow-up idea
coverage and `todo.md`; no implementation or test behavior changed.
