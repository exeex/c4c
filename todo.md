# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Probed the Step 5 HIR two-phase instantiated-constructor registration seam for
concrete template owners, then reverted the non-commit-ready source edit because
the delegated positive-Sema proof did not improve over the 883/884 baseline.

The reverted probe registered every constructor overload for the concrete
instantiated owner before lowering constructor bodies. That moved the failing
test past the original concrete-owner delegating lookup error:

`no constructors found for delegating constructor call to 'eastl::pair_T1_int_T2_int'`

The next failure under that probe was:

`StmtEmitter: MemberExpr base has no struct tag (field='value')`

That is the first observed boundary after concrete-owner constructor lookup:
delegating constructor lookup can find the sibling constructor once the
two-phase registry exists, but the selected member-template constructor body is
still lowered without call-specific method-template type-pack and NTTP-pack
specialization. Its nested callable parameters and body expressions still carry
unresolved pack-owned aggregate types, so member access on the lowered pack
argument collapses to a base without a structured aggregate tag.

## Suggested Next

Implement the combined HIR repair: restore two-phase concrete-owner constructor
registration and pair it with selected member-template constructor body
specialization from structured type-pack and NTTP-pack bindings. The selected
constructor body must lower callable parameters such as `tuple<Args...>` and
value-pack uses such as `index_sequence<I...>` from parser-owned structured
metadata, not from rendered names.

## Watchouts

The committed tree remains at 883/884. The remaining failure is still:

`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

The two-phase registry by itself is necessary but not sufficient. A dirty probe
showed it changes the first bad fact from missing concrete-owner constructor
lookup to missing selected member-template constructor specialization. Do not
commit the registry-only patch unless the same slice also repairs the
call-specific type-pack/NTTP-pack binding or has a focused generic proof that
the deferred/specialized lowering boundary is observable.

## Proof

Delegated positive-Sema proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Final result after reverting non-commit-ready source edits: unchanged, 883/884
passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing. The refreshed clean-tree failure remains:

`no constructors found for delegating constructor call to 'eastl::pair_T1_int_T2_int'`

A dirty two-phase registry probe using the same command also remained 883/884
but advanced that same test to `MemberExpr base has no struct tag
(field='value')`; that probe was reverted per packet because it was not
monotonic.

Proof log: `test_after.log`.
