# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.4
Current Step Title: Narrow The Non-Template Member-Typedef Mirror

## Just Finished

Step 2.4.4.4 narrowed ordinary non-template record-body member typedef
publication by removing the `register_structured_typedef_binding(member_key,
...)` write from `register_record_member_typedef_bindings`. Non-template
record-body typedefs now remain published through the record's
`member_typedef_names` / `member_typedef_types` metadata instead of creating a
generic `owner::member` typedef-table entry during record finalization.

No rendered qualified-text parsing/splitting helper, `std::string_view`
semantic lookup API, or fallback-spelling route was added.

## Suggested Next

Next executor packet should start the alias-template member-typedef carrier
route only if the supervisor selects Step 2.4.4.5A.1; do not delete the
dependent/template member-typedef bridge before that structured carrier is in
place.

## Watchouts

- `record_member_typedef_key_in_context` and
  `register_dependent_record_member_typedef_binding` still have live
  dependent/template bridge users and remain parked for the later structured
  carrier steps.
- `register_struct_member_typedef_binding(owner, member, type)` is no longer
  present in the parser API; this packet found and removed the remaining
  ordinary record-body generic typedef-table publication path.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the build output and 879/879 passing
`cpp_positive_sema_` tests.
