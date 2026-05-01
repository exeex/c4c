# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.3
Current Step Title: Convert C-Style Cast Type-Id Member-Typedef Consumer

## Just Finished

Step 2.4.4.3 converted the qualified base-type consumer used by C-style
cast/type-id parsing so non-template record-body member typedefs are checked
through structured record/member metadata before any generic qualified typedef
fallback. `Box::AliasL` / `Box::AliasR` style aliases now use the owning
record's `member_typedef_names` / `member_typedef_types` carrier through a
`QualifiedNameRef`-based helper, without introducing rendered qualified text
or fallback-spelling helper inputs.

Added focused C++ positive coverage for namespace and global-qualified
record-member typedef reference aliases in C-style casts.

## Suggested Next

Next executor packet should continue Step 2.4.4 by auditing the remaining
parser-side qualified member typedef consumers for generic rendered-name
fallback dependency before any mirror API deletion.

## Watchouts

- The HIR `resolve_struct_member_typedef_type(std::string tag, std::string member, ...)`
  cleanup remains out of scope for idea 139 and belongs to idea 140.
- The executor proof regex `^positive_sema_` exercises C positive tests only;
  supervisor acceptance used the focused C++ C-style cast bucket so the new
  test appears in canonical before/after logs.
- Do not delete the `owner::member` mirror or storage APIs from this packet.

## Proof

Delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' >> test_after.log 2>&1`

Result: passed.

Supplemental focused C++ regression:

`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_c_style_cast_global_record_member_typedef_ref_alias_basic_cpp$'`

Result: passed.

Supervisor acceptance guard:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_c_style_cast_.*_cpp$' >> test_after.log 2>&1`

Result: passed. Regression guard compared `test_before.log` and
`test_after.log` at 43 -> 44 passing tests, with no new failures.
