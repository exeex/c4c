# Current Packet

Status: Complete
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step3_2_function_body_value_fallback_review.md
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Completed Step 3.1 consteval function-body value producer/metadata exposure:
`ConstEvalEnv::lookup(const Node*)` now receives a
`local_binding_metadata_miss` bit from the structured-key and `TextId` lookup
passes. That bit is set only by populated local/parameter/loop binding metadata
maps (`local_const_scopes_by_*` or `local_consts_by_*`) and does not consult
rendered local-binding maps.

Added `consteval_local_binding_metadata.cpp` coverage for consteval parameter,
nested local, and loop-local reads through the interpreter binding producers.
No rendered fallback deletion was performed in this packet.

## Suggested Next

Move to a Step 3.2 function-body value deletion packet: consume
`local_binding_metadata_miss` to block `lookup_rendered_nttp_compatibility()`
when a structured-key or `TextId` local/parameter/loop binding miss is
authoritative.

## Watchouts

- This packet only deleted the intrinsic TypeSpec-key miss fallback in
  `resolve_type()`. It did not remove the structured-name, text-id,
  rendered-name, or legacy type-binding mirror routes for TypeSpecs that do not
  carry intrinsic template-parameter metadata.
- Do not use `has_rendered_local_binding()`, rendered local maps, or an
  equivalent rendered-name predicate as the authority boundary for local
  metadata misses.
- The new `local_binding_metadata_miss` flag is intentionally exposed but not
  consumed yet; Step 3.2 owns the behavior change that suppresses rendered NTTP
  compatibility for that covered miss.
- Direct NTTP rendered compatibility remains in place for producer-incomplete
  routes where there is no structured-key or `TextId` NTTP metadata.
- Non-body/global consteval value routes remain outside the next packet.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 29/29
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_local_binding_metadata_cpp`.
