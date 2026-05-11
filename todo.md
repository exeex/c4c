Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Narrow Sema and Consteval Rendered Mirrors

# Current Packet

## Just Finished

Step 3 - Narrow Sema and Consteval Rendered Mirrors: completed the NTTP
binding mirror packet in `ConstEvalEnv`.

Implementation notes:

- Inspected production construction and lookup of
  `nttp_bindings`, `nttp_bindings_by_text`, `nttp_bindings_by_key`,
  `record_nttp_binding_mirrors`, and forwarded NTTP binding lookup in
  `bind_consteval_call_env`.
- Renamed the private rendered NTTP helper to
  `lookup_nttp_binding_by_legacy_rendered_no_metadata_bridge` so the retained
  rendered map entry point is explicitly marked as a no-metadata compatibility
  bridge.
- Tightened comments on the rendered `nttp_bindings` field, the rendered
  lookup helper, `record_nttp_binding_mirrors`, and
  `lookup_forwarded_nttp_arg_by_text`: TextId/key maps remain binding
  authority for covered NTTP paths, rendered maps remain only legacy
  no-metadata mirrors, and complete key/text misses must not reopen rendered
  lookup.
- Preserved structured lookup ordering in `ConstEvalEnv::lookup`: complete
  key hits beat TextId/rendered mirrors, TextId hits beat rendered mirrors when
  no key hit exists, and complete NTTP key/text misses fail closed.
- Added focused `cpp_hir_sema_consteval_type_utils_metadata_test` coverage for
  NTTP key/text mirror authority, closed misses, forwarded NTTP miss fencing,
  and the retained no-metadata forwarded rendered fallback.

Changed files:

- `src/frontend/sema/consteval.hpp`
- `src/frontend/sema/consteval.cpp`
- `tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp`
- `todo.md`
- `test_after.log`

## Suggested Next

Continue Step 3 with the next narrow sema/consteval mirror family from the
idea 167 inventory, likely fallback canonical template names or another
remaining sema rendered mirror. Inspect production callers first, then fence or
delete one rendered bridge family and add closed-miss proof for its structured
carrier.

## Watchouts

- Keep ABI, display, diagnostics, and final spelling output-only; Step 3 should
  not remove visible text just to reduce rendered-string grep count.
- Do not fold route-local generated-name cleanup into this plan; idea 169 owns
  route-local identity domains.
- `nttp_bindings` remains retained for no-metadata consteval compatibility and
  for legacy forwarded spellings when the call-site supplies no TextId/key
  carrier. Any valid forwarded TextId plus present metadata map must continue
  to fail closed on miss.
- `record_nttp_binding_mirrors` currently records an unqualified local-style
  key for NTTP parameters because the existing consteval NTTP key domain is
  based on the parameter TextId. Do not widen this into owner/index identity;
  generated/local identity cleanup belongs to idea 169.
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
