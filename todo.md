# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 3.2
Current Step Title: Delete Consteval Rendered Compatibility After Metadata Completion

## Just Finished

Step 3.2 deleted the first covered consteval TypeSpec rendered-compatibility
fallback: `resolve_type()` now treats
`lookup_type_binding_by_typespec_key()` returning `Miss` for an intrinsic
template-parameter `TB_TYPEDEF` TypeSpec as authoritative and returns the
unresolved TypeSpec instead of consulting structured-name, text-id,
rendered-name, or legacy type-binding mirror routes.

Follow-up testability check: a focused C++ miss-authority test is not directly
expressible for the covered intrinsic TypeSpec-key route in the current harness.
The smallest stale-rendered candidate,
`Box<char>::Alias` inside an outer `template <typename T> consteval` function,
still reaches consteval as a non-intrinsic metadata TypeSpec and incorrectly
recovers the caller's rendered `T`; adding that as a Step 3.2 test would fail
for a producer-completeness gap outside this fallback-deletion slice rather
than proving the covered intrinsic-key miss branch. The existing
`consteval_typespec_member_alias.cpp` remains the focused same-feature C++
coverage for the accepted covered route because it proves metadata-bearing
member-alias TypeSpecs still resolve through the intrinsic key `Found` path
after the miss fallback deletion; direct miss-branch coverage needs either a
producer repair that makes the stale candidate carry owner/index/text metadata
or a lower-level consteval unit harness that can construct a mismatched
metadata-bearing `TypeSpec`.

## Suggested Next

Delete the next accepted Step 3.2 fallback route for consteval function-body
value lookups now that the corresponding producer audit has been completed,
leaving non-body/global consteval value routes for a separate packet.

## Watchouts

- This packet only deleted the intrinsic TypeSpec-key miss fallback in
  `resolve_type()`. It did not remove the structured-name, text-id,
  rendered-name, or legacy type-binding mirror routes for TypeSpecs that do not
  carry intrinsic template-parameter metadata.
- A same-feature stale-rendered C++ candidate exists but is not yet covered by
  intrinsic TypeSpec metadata, so it should be routed as a producer repair
  before it becomes the miss-authority regression test.
- Non-body/global consteval value routes remain outside this packet.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 28/28
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_typespec_member_alias_cpp`.
