# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step3_2_function_body_value_fallback_review.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Accepted Step 3.2 progress remains limited to the TypeSpec fallback deletion:
`resolve_type()` now treats `lookup_type_binding_by_typespec_key()` returning
`Miss` for an intrinsic template-parameter `TB_TYPEDEF` TypeSpec as
authoritative and returns the unresolved TypeSpec instead of consulting
structured-name, text-id, rendered-name, or legacy type-binding mirror routes.

Rejected route: `review/step3_2_function_body_value_fallback_review.md`
rejected the attempted Step 3.2 function-body value deletion that used
`has_rendered_local_binding()` /
`suppress_rendered_nttp_for_local_binding_metadata_miss`. That route is drift
because it asks rendered local-binding maps whether a structured or `TextId`
miss should suppress rendered NTTP compatibility. Do not continue or repeat
that predicate.

## Suggested Next

Return to Step 3.1 for one producer/metadata repair packet before any further
Step 3.2 function-body value fallback deletion:

- Identify how `ConstEvalEnv::lookup(const Node*)` can observe an
  authoritative local/parameter/loop binding miss from structured or `TextId`
  metadata itself, without consulting rendered local-binding maps.
- Repair or expose that parser/Sema-owned producer metadata if the ownership
  boundary is clear.
- If the metadata cannot be made authoritative inside parser/Sema scope, park
  the exact direct NTTP producer-completeness gap here and leave that rendered
  compatibility fallback out of the next deletion packet.

Only after that producer/metadata result is recorded should a Step 3.2 packet
delete the covered function-body value fallback.

## Watchouts

- This packet only deleted the intrinsic TypeSpec-key miss fallback in
  `resolve_type()`. It did not remove the structured-name, text-id,
  rendered-name, or legacy type-binding mirror routes for TypeSpecs that do not
  carry intrinsic template-parameter metadata.
- Do not use `has_rendered_local_binding()`, rendered local maps, or an
  equivalent rendered-name predicate as the authority boundary for local
  metadata misses.
- A same-feature stale-rendered C++ candidate exists but is not yet covered by
  intrinsic TypeSpec metadata, so it should be routed as a producer repair
  before it becomes the miss-authority regression test.
- Direct NTTP rendered compatibility remains producer-incomplete unless the
  next packet can prove the function-body local-binding miss from metadata.
- Non-body/global consteval value routes remain outside the next packet.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 28/28
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_typespec_member_alias_cpp`.
