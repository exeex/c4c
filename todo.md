# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.2
Current Step Title: Prove Direct Record Member-Typedef Lookup

## Just Finished

Step 2.4.2 repaired the first broader parser smoke failure by making
`starts_with_value_like_template_expr` reject parser-known alias-template heads,
including heads reached through structured using-alias metadata, before
classifying `Name<...>(...)` as value-like. This uses parser-owned `TextId`
normalization and structured alias-template keys, not rendered qualified text.

The direct record/member typedef expression route remains on the
`probe_qualified_type` direct payload: it still copies
`record_member_typedef_type` and consumes from the already-parsed qualified-name
boundary instead of running the tentative rendered-text comparison.

Step 2.4.2 also repaired the remaining broader parser smoke failure by
preserving qualified `TypeSpec` source metadata across combined typedef
resolution. `parse_base_type` now keeps the parsed base-name `TextId`, resolved
namespace context, global qualifier flag, and qualifier `TextId` arrays when it
copies the registered typedef payload, so `ns::Alias` and
`typename ns::Alias` retain parser-owned source identity.

## Suggested Next

Have the supervisor review and commit the completed Step 2.4.2 slice, then
select the next Step 2.4 packet from the active plan.

## Watchouts

- The expression branch still uses normal `parse_base_type` for non-direct
  typedef/type routes; this packet only removed the direct member-typedef
  dependency on the tentative rendered-text probe.
- The alias-template smoke assertion
  `resolved alias-template probes should use parser-owned spelling` and both
  qualified `TypeSpec` metadata assertions are repaired by the current slice.
- Do not delete the `owner::member` mirror, delete APIs, or claim storage
  narrowing from this packet.
- Continue rejecting helpers that accept rendered `owner::member` text,
  `std::string`, or `std::string_view` qualified text and parse it back into
  owner/member identity.
- `find_typedef_type(TextId)` scoped-string handling remains a metadata
  blocker, not an acceptable conversion template.
- Instantiated member typedefs such as `ns::holder_T_int::type` still need
  concrete owner-member `TextId` or equivalent domain metadata before broader
  cleanup.
- HIR-only rendered lookup cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Delegated proof was run and is preserved in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_(qualified_member_typedef_functional_cast_frontend_cpp|c_style_cast_member_typedef_ref_alias_basic_cpp|qualified_type_start_probe_parse_cpp|qualified_type_start_shared_probe_parse_cpp|template_member_type_direct_parse_cpp|template_member_typedef_cache_roundtrip_parse_cpp|record_member_typedef_using_parse_cpp|c_style_cast_qualified_typedef_ref_alias_basic_cpp))$' > test_after.log 2>&1`

Result: passed, 9/9 tests. `frontend_parser_tests` passed both
`qualified TypeSpec should carry the base-name TextId metadata` and
`qualified typename TypeSpec should carry the base-name TextId metadata`.
