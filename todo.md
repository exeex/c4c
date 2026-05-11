Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR static-member rendered declaration and const-value maps are now explicitly
documented and tested as no-owner/rendered compatibility mirrors while
preserving owner-key authority, base/template trait fallback paths, and
fail-closed complete owner-key misses.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/impl/lowerer.hpp` | Marked retained `struct_static_member_decls_` and `struct_static_member_const_values_` as no-owner rendered compatibility mirrors and documented the helper contract. |
| `src/frontend/hir/hir_types.cpp` | Annotated rendered static-member lookup branches so complete owner-key misses remain closed except structured base/template trait paths. |
| `tests/frontend/frontend_hir_tests.cpp` | Renamed focused rendered fallback tests/messages to no-owner rendered compatibility while keeping owner-key win and miss-closed coverage intact. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Tightened owner-key stale-rendered static-member assertions to call the rendered maps compatibility data, not authority. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with the next rendered-qualified/no-owner handoff family after
the completed `struct_defs`, declaration `fn_index`/`global_index`,
`template_defs`, and static-member declaration/const compatibility slices.

## Watchouts

- `struct_static_member_decls_` and `struct_static_member_const_values_` remain
  rendered tag/member maps only for no-owner compatibility; the
  `HirStructMemberLookupKey` maps are lookup authority.
- Complete static-member owner-key misses still fail closed before rendered map
  lookup, except existing structured base fallback and const template-trait
  evaluation paths.
- `template_defs` remains intentionally rendered-name keyed for HIR
  preservation/bookkeeping; do not treat it as AST template-definition
  authority.
- CompileTimeState `has_template_def`/`find_template_def` remains the
  authoritative structured/template definition path for deferred instantiation.
- The retained declaration rendered fallback is still intentionally available
  for missing/incomplete metadata and for the named rendered-qualified or
  self-consistent compatibility predicates after a structured miss.
- Complete structured declaration-key misses remain closed before
  `fn_index`/`global_index` unless those named compatibility predicates allow
  the rendered bridge.
- Keep HIR `FunctionCtx` local/label/generated-name cleanup out of this plan;
  idea 169 owns route-local identity domains.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Proof command, logged to `test_after.log`:

`cmake --build build --target frontend_hir_tests frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$'`

Result: passed; 2/2 focused tests passed (`frontend_hir_tests`,
`frontend_hir_lookup_tests`).

Additional check: `git diff --check` passed.
