# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.4.4.5C
Current Step Title: Add Dependent/Template Record Member-Typedef Carrier

## Just Finished

Step 2.4.4.5C added a parser/Sema structured carrier for
template/dependent record member typedef availability keyed by structured
record owner identity plus member `TextId`.

`register_record_member_typedef_bindings()` now registers template/dependent
record member typedef availability into `dependent_record_member_typedefs_by_owner`
using a `QualifiedNameKey` owner and member `TextId`, without using
`source_tag` as semantic authority for that route. Qualified typedef lookup
decomposes a structured `Owner::member` key and consults the new owner/member
carrier before the older full-member-key fallback.

## Suggested Next

Proceed to Step 2.4.4.6 only after supervisor review confirms no remaining
parser/Sema reader still requires rendered `source_tag::member` typedef
publication for this route.

## Watchouts

- The old `dependent_record_member_typedefs_by_key` full-member-key table still
  exists as a lookup fallback for compatibility; this packet stopped
  `register_record_member_typedef_bindings()` from populating it for the
  covered template/dependent route.
- `record_member_typedef_infos_by_key` still uses the existing structured full
  member key for alias-template RHS sidecar metadata; this packet only changed
  member typedef availability, not alias-template RHS metadata ownership.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_(eastl_slice7_piecewise_ctor_parse_cpp|step3_timeout_probe_baseline_parse_cpp|tuple_element_alias_mix_parse_cpp)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 3/3
passing delegated parser/Sema fixtures:
`cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp`,
`cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp`, and
`cpp_positive_sema_tuple_element_alias_mix_parse_cpp`.
