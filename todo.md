# Current Packet

Status: Complete
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: review/step3_2_function_body_value_fallback_review.md
Current Step ID: Step 3.2
Current Step Title: Delete Consteval Rendered Compatibility After Metadata Completion

## Just Finished

Completed Step 3.2 consteval function-body value fallback deletion:
`ConstEvalEnv::lookup(const Node*)` now consumes
`local_binding_metadata_miss` inside `lookup_rendered_nttp_compatibility()`.
Structured-key and `TextId` NTTP metadata lookups still run first, but an
authoritative local/parameter/loop metadata miss now blocks the final rendered
NTTP name-map recovery when NTTP metadata exists for the call.

Updated `consteval_local_binding_metadata.cpp` with a mixed local + NTTP
consteval case so direct NTTP metadata/producer-incomplete compatibility stays
covered while the rendered local-map predicate remains absent.

## Suggested Next

Supervisor should review/commit the completed Step 3.2 function-body value
slice, then decide whether the active plan needs reviewer scrutiny or the next
Step 3 cleanup packet.

## Watchouts

- The earlier packet only deleted the intrinsic TypeSpec-key miss fallback in
  `resolve_type()`. It did not remove the structured-name, text-id,
  rendered-name, or legacy type-binding mirror routes for TypeSpecs that do not
  carry intrinsic template-parameter metadata.
- Do not use `has_rendered_local_binding()`, rendered local maps, or an
  equivalent rendered-name predicate as the authority boundary for local
  metadata misses.
- The `local_binding_metadata_miss` deletion is intentionally limited to final
  rendered-name NTTP recovery. Direct structured/TextId NTTP hits still win, and
  producer-incomplete routes with no NTTP metadata remain compatible.
- Non-body/global consteval value routes remain outside the next packet.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 29/29
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_local_binding_metadata_cpp` and
`cpp_positive_sema_consteval_nttp_cpp`.
