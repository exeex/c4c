# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Step 5 was probed and blocked; no source edits were kept.

The HIR-side constructor registry probe showed the first repair seam: instantiated
template struct methods currently lower constructor bodies before all constructor
overloads for that instantiated owner are registered. A temporary two-phase
registration probe moved the focused failure from:

`no constructors found for delegating constructor call`

to:

`StmtEmitter: MemberExpr base has no struct tag (field='first')`

That exposed the next, earlier carrier boundary: the direct qualified constructor
call `eastl::pair<int, int>(...)` reaches HIR as `Var(eastl::pair)` with
`has_template_args=false`, `n_template_args=0`, and no `TypeSpec` template-arg
carrier on either the call or callee node. Without that structured explicit
owner-argument carrier, HIR can only instantiate/lower the unresolved
`eastl::pair_T1_T_T2_T` owner, so method type-pack deduction would still be
grounded on an unresolved owner instead of the concrete `int, int` constructor
specialization.

## Suggested Next

Implement the prerequisite Step 5 sub-seam: repair the parser-to-HIR handoff for
qualified explicit constructor template-id expressions so
`eastl::pair<int, int>(...)` reaches HIR with structured callee template
arguments on the `NK_CALL`/callee `NK_VAR` path. After that carrier is
observable, reapply the HIR two-phase instantiated-constructor registry repair
and then specialize method-template constructor overloads from structured
type-pack and NTTP-pack bindings.

## Watchouts

The delegated positive-Sema proof is still 883/884. The remaining failure is
still:

`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

Do not recover the missing `pair<int, int>` owner from rendered names,
`debug_text`, `_t` spellings, `tag_ctx`, module dumps, or the test shape. The
next repair needs the structured explicit template-id carrier before HIR method
pack deduction can be made semantic.

## Proof

Delegated positive-Sema proof after reverting non-commit-ready probe edits:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: unchanged, 883/884 passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing. Proof log: `test_after.log`.
