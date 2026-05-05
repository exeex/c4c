# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Implemented the prerequisite Step 5 parser-to-HIR carrier sub-seam for explicit
constructor template-id calls. The parser now projects the structured owner
`TypeSpec` returned by `parse_base_type()` onto synthesized constructor `NK_CALL`
and callee `NK_VAR` nodes, including explicit template-argument arrays and the
owner `TypeSpec` template-arg carrier.

A focused generic parser metadata test now proves
`ns::Owner<int, long>(...)` reaches the call/callee path with:

- `has_template_args=true` and `n_template_args=2` on the callee.
- Structured `template_arg_types` for `int` and `long`.
- A concrete owner `TypeSpec` with `tpl_struct_args` on both callee and call.

The full positive-Sema proof still reports 883/884, but the remaining failure
moved from unresolved owner `eastl::pair_T1_T_T2_T` to concrete owner
`eastl::pair_T1_int_T2_int`, showing the explicit constructor owner args now
reach HIR.

## Suggested Next

Reapply the HIR two-phase instantiated-constructor registry repair for concrete
template owners. Instantiated template struct methods need all constructor
overloads registered for the concrete owner before any constructor body is
lowered, so delegating constructor lookup can find sibling constructor
templates on `Owner<Ts...>`.

## Watchouts

The delegated positive-Sema proof is still 883/884. The remaining failure is
still:

`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

The missing parser carrier is repaired; do not regress it back to rendered-name
recovery. The next HIR repair should consume the structured concrete owner and
constructor `Node` metadata directly, then continue into method type-pack and
NTTP-pack deduction.

## Proof

Focused metadata proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^cpp_hir_parser_declarations_residual_structured_metadata$'`

Result: passed, 1/1.

Delegated positive-Sema proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: unchanged, 883/884 passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing. The failure now names concrete owner `eastl::pair_T1_int_T2_int`.
Proof log: `test_after.log`.
