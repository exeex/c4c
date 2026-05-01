# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5A.2
Current Step Title: Resolve Alias Instantiation Through The Structured Carrier

## Just Finished

Step 2.4.4.5A.2 made alias-template instantiation derive the alias
`QualifiedNameKey` from preserved structured type metadata before consulting
fallback context lookup, so qualified aliases such as `lib::select_t<...>` reach
their own `ParserAliasTemplateMemberTypedefInfo` carrier instead of relaying
through rendered qualified spelling or a same-named current-context alias.

The structured carrier resolver remains the success route for
`typename Owner<Args>::member`: it substitutes parsed carrier owner args through
alias template params, selects the owner primary/specialization from the carrier
owner key, and finds the member typedef by member `TextId`. The retained
rendered/deferred `TypeSpec` fields are still temporary compatibility fallback,
not the accepted route.

## Suggested Next

Next coherent packet is Step 2.4.4.5A.3 review/acceptance of the carrier route
before any Step 2.4.4.5B bridge deletion. Keep bridge deletion out of scope
until the supervisor accepts the structured route.

## Watchouts

- `find_alias_template_info_in_context(current_namespace_context_id(), ...)`
  remains as a fallback after the structured alias key probe; it is not the
  success criterion for qualified alias-template member typedefs.
- `record_member_typedef_key_in_context` and
  `register_dependent_record_member_typedef_binding` still have live
  dependent/template bridge users and remain parked for later structured
  carrier steps.
- The four known bridge-deletion regressions were covered by the delegated
  `cpp_positive_sema_` subset; no named-test shortcut or expectation downgrade
  was used.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the build output and 881/881 passing
`cpp_positive_sema_` tests, including the new
`cpp_positive_sema_template_alias_member_typedef_qualified_carrier_runtime_cpp`
same-feature disagreement case and the four known bridge-deletion regression
tests named in `plan.md`.
