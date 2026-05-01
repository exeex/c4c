# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 2.4.4.6
Current Step Title: Delete Or Park The Remaining Member-Typedef Mirror

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

Execute Step 2.4.4.6: delete the remaining live rendered scoped
member-typedef mirror, or park only a clearly non-semantic compatibility cache.

Primary target: `dependent_record_member_typedefs_by_key` and any remaining
parser/Sema lookup path that still lets rendered scoped member-typedef storage
decide semantic identity.

Executor target:

- Re-audit current writers/readers of `dependent_record_member_typedefs_by_key`
  and any remaining rendered `owner::member` member-typedef storage.
- Delete the mirror writer/storage if every semantic reader is now covered by
  direct record/member metadata, `QualifiedNameKey`, namespace context plus
  member `TextId`, `template_instantiation_member_typedefs_by_key`,
  `ParserAliasTemplateInfo::member_typedef`, or
  `dependent_record_member_typedefs_by_owner`.
- If any remaining cache must stay, rename or isolate it only when its
  non-semantic purpose is explicit and it cannot decide parser/Sema semantic
  identity.
- Do not replace the mirror with a helper that renders, splits, reparses, or
  otherwise recovers owner/member identity from `owner::member`,
  `source_tag::member`, `std::string`, `std::string_view`, or fallback spelling.
- If the only remaining blocker is outside parser/Sema ownership, record the
  blocker as a separate open idea instead of extending this cleanup route.

Suggested proof command:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_' >> test_after.log 2>&1`

## Watchouts

- The old `dependent_record_member_typedefs_by_key` full-member-key table still
  exists as a lookup fallback for compatibility; Step 2.4.4.6 owns deleting it
  or proving it is parked as non-semantic.
- Supervisor review accepted Step 2.4.4.5C: the old
  `dependent_record_member_typedefs_by_key` fallback is the cleanup target, not
  a blocker to 5C acceptance.
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

Supervisor follow-up after commit `8575251c7` also confirmed:

- `register_record_member_typedef_bindings()` no longer uses `source_tag` as
  semantic authority for the dependent/template record member typedef route.
- Full `cpp_positive_sema_` passed.
- Full `^cpp_` passed.
