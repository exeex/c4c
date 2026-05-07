# Current Packet

Status: Active
Source Idea Path: ideas/open/148_hir_static_member_carrier_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Preserve Generated And Member-Typedef Payload Folding

## Just Finished

Step 4 variadic carrier repair is complete. The scalar/static-member owner
route now recovers the primary template origin from exact structured owner
metadata when available, and from the qualified-ref owner segment only when it
resolves to a registered primary template. That lets
`X<sizeof...(Ts)>::value` re-evaluate against each function-template
specialization's pack bindings instead of reusing the earlier `X_N_0` owner.

The reviewer-blocker repair remains intact: rendered suffix owner-tag and
template-primary recovery loops stay removed, member identity tries structured
`unqualified_text_id` before `unqualified_name`, and generated member payload
is still used only after structured owner authority exists.

## Suggested Next

Next packet: supervisor or reviewer can re-review the Step 4 route-clean slice
for commit readiness.

## Watchouts

- `rg` no longer finds the reviewed rendered suffix owner/template-primary
  recovery patterns in `scalar_control.cpp` or `hir_types.cpp`.
- The variadic repair depends on exact structured qualifier metadata and
  registered primary-template identity; it does not restore rendered owner
  suffix recovery.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ./build/tests/frontend/frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `./build/tests/frontend/frontend_hir_lookup_tests` passed and
CTest records 357/357 passing. `test_after.log` is the preserved proof log.
