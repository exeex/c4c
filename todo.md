Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4B
Current Step Title: Delete TextId-Only Parser-Map Record Fallback

# Current Packet

## Just Finished

Step 4B completed the fallback deletion follow-up without restoring parser-map
uniqueness. Parser explicit template constructor carriers now preserve
structured template-record metadata, HIR template deduction can match
structured template origins and record-def template arg payloads, and
materialization can expand foreign method-pack refs such as `Args1#0=int`.

The final unblock was the specialized constructor handoff: constructor
overload lowering now avoids template-erased parser-instantiated constructor
clones, keeps class bindings on constructor overload records, and routes
explicit-template calls through context-aware deduction so `get<>(first_args)`
can see the active lowered `FunctionCtx` parameter type.

## Suggested Next

Supervisor should review and commit the completed Step 4B slice, or escalate to
broader validation if treating this as a milestone.

## Watchouts

Do not restore parser-map uniqueness, rendered-key matching, or
context-defaulted TextId lookup. The existing HIR tests in
`frontend_hir_lookup_tests` cover structured pack deduction and foreign-pack
materialization in isolation. The constructor repair is a general
context-aware deduction and overload-registration fix, not a named-case
shortcut for `eastl::get` or the runtime fixture.

## Proof

`test_after.log` records the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe|cpp_hir_parser_declarations_residual_structured_metadata|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp)$'`.
Build passed. `frontend_parser_tests`, `frontend_parser_lookup_authority_tests`,
`cpp_eastl_vector_parse_recipe`,
`cpp_hir_parser_declarations_residual_structured_metadata`,
`frontend_hir_lookup_tests`, and
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp` passed.
