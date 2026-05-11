Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Narrow Sema and Consteval Rendered Mirrors

# Current Packet

## Just Finished

Step 3 - Narrow Sema and Consteval Rendered Mirrors: completed the
template/type binding mirror packet in `ConstEvalEnv`.

Implementation notes:

- Inspected production construction and lookup of
  `type_bindings_by_text`, `type_bindings_by_key`,
  `type_binding_text_ids_by_name`, and `type_binding_keys_by_name`.
- Renamed the internal rendered-name lookup helpers to
  `lookup_type_binding_by_legacy_rendered_text_bridge` and
  `lookup_type_binding_by_legacy_rendered_key_bridge` so the only rendered-name
  entry point is marked as a legacy display-tag bridge.
- Tightened comments on the `ConstEvalEnv` type-binding fields and
  `record_type_binding_mirrors`: text/key maps remain authority; name mirrors
  only bridge an explicit legacy rendered display tag into that authority and
  must not act as ordinary lookup.
- Preserved structured lookup ordering in `resolve_type`: complete key hits
  win over TextId/rendered mirrors, complete key misses fail closed, TextId
  hits remain authoritative when no complete key carrier exists, and TextId
  misses do not recover through rendered maps.
- Added focused `cpp_hir_sema_consteval_type_utils_metadata_test` coverage for
  complete key/text misses and for name mirrors remaining inert when a
  `TypeSpec` has no direct metadata or legacy display tag.

Changed files:

- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/consteval.cpp`
- `tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp`
- `todo.md`
- `test_after.log`

## Suggested Next

Continue Step 3 with the next narrow sema/consteval mirror family from the
idea 167 inventory, likely NTTP binding mirrors or fallback canonical template
names. Inspect production callers first, then fence or delete one rendered
bridge family and add closed-miss proof for its structured carrier.

## Watchouts

- Keep ABI, display, diagnostics, and final spelling output-only; Step 3 should
  not remove visible text just to reduce rendered-string grep count.
- Do not fold route-local generated-name cleanup into this plan; idea 169 owns
  route-local identity domains.
- `type_binding_text_ids_by_name` and `type_binding_keys_by_name` remain
  retained legacy display-tag bridge indexes. They are not authority by
  themselves; complete misses in the mapped text/key tables must continue to
  fail closed.
- Current `TypeSpec` has no legacy `tag` member, so the rendered display-tag
  bridge is effectively dormant in this build. The retained helper names and
  comments document the owner and removal condition without deleting the
  compatibility surface.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Ran delegated proof command and refreshed `test_after.log`:

`cmake --build build --target cpp_hir_sema_consteval_type_utils_metadata_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_hir_tests)$'`

Result: pass. Built both delegated targets and ran 2/2 matching CTest tests:
`cpp_hir_sema_consteval_type_utils_structured_metadata` and
`frontend_hir_tests`.

Also ran `git diff --check`: pass.
