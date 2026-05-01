# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step2_4_5b_1_failed_record_scope_carrier_review.md
Current Step ID: Step 2.4.4.5B.3
Current Step Title: Delete The Using-Alias Projector

## Just Finished

Step 2.4.4.5B.3 attempted deletion of
`apply_alias_template_member_typedef_compat_type` and its only remaining call
site in `src/frontend/parser/impl/declarations.cpp`.

Result: blocked. The code builds, and 879/881 `cpp_positive_sema_` tests pass,
but the two required fixtures return to parse timeouts without the projector:
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`.

## Suggested Next

Next packet should add the missing structured carrier needed by top-level
template using-alias RHS propagation for `typename Owner<Args>::member`, then
retry deleting `apply_alias_template_member_typedef_compat_type`.

## Watchouts

- Exact blocker: deleting the projector removes the last fallback that turns a
  top-level template using-alias RHS sidecar for
  `typename Owner<Args>::member` into a resolvable parser type carrier. The
  committed record-scope member-alias sidecar is not sufficient for the two
  STL iterator fixtures by itself.
- Do not unblock this by restoring rendered/deferred `TypeSpec`,
  `debug_text`, split `Owner::member`, expectation downgrades, or named-fixture
  shortcuts.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: failed. `test_after.log` contains a fresh successful build plus
879/881 passing `cpp_positive_sema_` tests. The failing tests are:
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`, both with
`[PARSE_TIMEOUT]` after 30s.
