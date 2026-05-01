# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step2_4_5b_3_blocked_projector_deletion_review.md
Current Step ID: Step 2.4.4.5B.2
Current Step Title: Implement Top-Level Template Using-Alias RHS Structured Carrier Follow-Up

## Just Finished

Step 2.4.4.5B.3 attempted deletion of
`apply_alias_template_member_typedef_compat_type` and its only remaining call
site in `src/frontend/parser/impl/declarations.cpp`.

Result: blocked. The code builds, and 879/881 `cpp_positive_sema_` tests pass,
but the two required fixtures return to parse timeouts without the projector:
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`.

## Suggested Next

Next executor packet stays within Step 2.4.4.5B and should add the missing
structured carrier needed by top-level template using-alias RHS propagation for
`typename Owner<Args>::member`.

Packet contract:

- Preserve the blocked B.3 deletion result as context, not implementation
  progress.
- Thread owner identity as structured parser/Sema metadata such as direct
  record/template owner metadata, `QualifiedNameKey`, namespace-aware key, or
  another reviewed domain key.
- Preserve member identity as `TextId` from the parsed member token or reviewed
  structured metadata source.
- After the carrier covers the two timeout fixtures, retry deleting
  `apply_alias_template_member_typedef_compat_type`.

## Watchouts

- Exact blocker: deleting the projector removes the last fallback that turns a
  top-level template using-alias RHS sidecar for
  `typename Owner<Args>::member` into a resolvable parser type carrier. The
  committed record-scope member-alias sidecar is not sufficient for the two
  STL iterator fixtures by itself.
- Do not unblock this by restoring rendered/deferred `TypeSpec`,
  `debug_text`, split `Owner::member`, expectation downgrades, or named-fixture
  shortcuts.
- Do not claim implementation progress from the B.3 deletion attempt; it only
  discovered the top-level template using-alias RHS carrier blocker.
- `clang-format` is not installed in this environment, so no automatic format
  pass was available.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: failed. `test_after.log` contains a fresh successful build plus
879/881 passing `cpp_positive_sema_` tests. The failing tests are:
`cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp` and
`cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp`, both with
`[PARSE_TIMEOUT]` after 30s.
