Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C structured-carrier rework removed the reviewer-blocked HIR semantic
recovery from `TemplateArgRef::debug_text`, `@origin:args`, rendered module
names, and `module_->struct_defs.find(rendered_name)`. The parser now keeps
nested pending template struct arguments in typed `TemplateArgRef` payloads,
and HIR realizes nested template struct carriers from `tpl_struct_origin_key`
plus typed args instead of reparsing display text.

Step 4C callable return-type substitution is now repaired. Recursive signature
template substitution writes back same-size nested `TemplateArgRef`
substitutions such as `Box<T>` to `Box<int>`, and the return-type preparation
path no longer re-routes already-realized concrete structured returns through
template-origin recovery.

## Suggested Next

Supervisor should review and commit the completed Step 4C structured-carrier
repair slice, including the parser/HIR carrier changes, callable return
substitution fix, directly required template-struct constructor registration,
and the generic aggregate value typing fallout.

## Watchouts

`resolve_record_type_spec` still preserves a parser-local compatibility bridge
for non-layout probes and declaration checks; the stricter helper is private to
constant layout. Do not route `sizeof`/`alignof`/`offsetof` back through the
public compatibility bridge for structured carriers.

Do not accept a repair whose semantic identity path reparses
`TemplateArgRef::debug_text`, `@origin:args`, rendered template instance names,
or rendered module record names. Those strings may remain diagnostics/display
metadata only.

Do not use fixture-name matching, expectation downgrades, or supported-path
weakening to claim Step 4C progress. The known blocker is nested template field
carriers like `Holder::boxed_pair` / `Holder::paired_box` lowering through
`Box_T_void` / `Pair_T_void` or `struct<?>`; this packet repaired that field
handoff and the nested template function return signature.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_support_residual_structured_metadata|frontend_hir_lookup_tests|cpp_eastl_vector_parse_recipe|cpp_positive_sema_ctor_init_delegating_unqualified_template_runtime_cpp|cpp_positive_sema_ctor_init_member_default_value_init_runtime_cpp|cpp_positive_sema_ctor_init_member_typedef_ctor_runtime_cpp|cpp_positive_sema_template_struct_advanced_cpp)$' > test_after.log`

Result: passed, 9/9 tests. Proof log: `test_after.log`.
