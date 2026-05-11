Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Narrow Sema and Consteval Rendered Mirrors

# Current Packet

## Just Finished

Step 3 - Narrow Sema and Consteval Rendered Mirrors: completed the first
rendered enum-constant mirror packet around `static_eval_int`.

Implementation notes:

- Inspected all production callers of `static_eval_int` and all assignments to
  `StaticEvalIntEnumLookupInput::rendered_enum_consts`.
- Removed the broad ordinary rendered-map overload
  `static_eval_int(Node*, const std::unordered_map<std::string, long long>&)`
  from the public surface and replaced it with the explicitly named
  `static_eval_int_with_rendered_enum_compatibility`.
- Added `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility` so HIR
  static-member initializer paths opt into the rendered enum mirror by name
  while still passing `enum_consts_by_key` as structured authority.
- Kept direct `static_eval_int(Node*, StaticEvalIntEnumLookupInput)` callers
  working. Complete structured key/text misses still fail closed before any
  rendered compatibility lookup.
- Updated `cpp_hir_sema_consteval_type_utils_metadata_test` to use the named
  compatibility boundary and to prove ordinary no-compatibility lookup does
  not infer rendered enum authority.

Changed files:

- `src/frontend/sema/type_utils.hpp`
- `src/frontend/sema/type_utils.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/impl/expr/scalar_control.cpp`
- `tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp`
- `todo.md`
- `test_after.log`

## Suggested Next

Continue Step 3 with the next narrow sema/consteval mirror family from the
idea 167 inventory, likely template/type/NTTP binding mirrors or fallback
canonical template names. Inspect production callers first, then fence or
delete one rendered bridge family and add closed-miss proof for its structured
carrier.

## Watchouts

- Keep ABI, display, diagnostics, and final spelling output-only; Step 3 should
  not remove visible text just to reduce rendered-string grep count.
- Do not fold route-local generated-name cleanup into this plan; idea 169 owns
  route-local identity domains.
- `StaticEvalIntEnumLookupInput::rendered_enum_consts` remains as a retained
  no-metadata compatibility mirror. New ordinary callers should use structured
  key/text metadata and should not assign the field directly.
- The rendered enum compatibility API preserves source/link compatibility for
  no-metadata callers only; complete structured key/text misses must continue
  to fail closed.
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
