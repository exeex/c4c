# Current Packet

Status: Blocked
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Attempted to extend the uncommitted Step 4 parser carrier slice into the HIR
constructor specialization seam. The source edits were reverted because the
delegated proof did not improve over the 883/884 baseline.

Observed progress before revert:
- Registering instantiated template-struct constructors made the public
  instantiated constructor callable and moved the failure past the original
  `no constructors found for delegating constructor call` boundary.
- Carrying the parsed constructor `TypeSpec` onto parser-synthesized
  constructor callees let HIR infer `auto p` as a structured
  `eastl::pair<int, int>` object, resolving `p.first`/`p.second` ownership.
- On-demand member-template constructor specialization produced a specialized
  public constructor call, but its lowered signature still carried
  `first_args`/`second_args` as unresolved `tuple<T>`-shaped parameters.

## Blocker

The first remaining bad carrier boundary is HIR callable parameter
substitution for nested method-template type packs. The parser-side constructor
`Node` has method-template prelude metadata, and direct constructor lowering
can deduce `Args1#0=int` / `Args2#0=int` from structured tuple arguments, but
`lower_struct_method`/`append_callable_params` still lower the specialized
constructor parameters as `tuple<Args...>` with the nested pack unresolved.
That leaves the delegated constructor body calling `get<I>(first_args)` with
`first_args: struct<?>`, and the frontend fails at:

`StmtEmitter: MemberExpr base has no struct tag (field='value')`

This is not safely fixable in the current slice without a focused repair to
expand/substitute nested template-argument packs inside HIR callable parameter
types. No source changes are left in the tree from this blocked attempt.

## Suggested Next

Split or delegate the next implementation seam around HIR callable signature
substitution for nested pack references: when a method-template parameter type
contains `Owner<Pack...>`, the lowered parameter type must consume structured
pack bindings such as `Pack#0` and realize `Owner<Concrete>` before the method
body is lowered. Keep the constructor registry and direct-constructor owner
work as a separate re-implementable slice once that signature substitution
boundary is ready.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: blocked, not monotonic. The refreshed `test_after.log` remains
883/884 passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp` failing.
