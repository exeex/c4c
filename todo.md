Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR `struct_defs` layout lookup rendered/tag-text fallback is fenced as
explicit no-owner/no-metadata compatibility without reopening complete owner
misses.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/hir_types.cpp` | Renamed the retained layout fallback helper to `find_struct_def_by_no_owner_layout_compatibility_tag` and documented that complete structured owner misses return before rendered/tag-text `struct_defs` lookup. |
| `src/frontend/hir/impl/expr/builtin.cpp` | Renamed the builtin layout fallback name helper to `no_owner_layout_compat_name_from_text` and documented the same no-owner boundary for `sizeof`/`alignof` layout queries. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Added direct no-owner layout TypeSpec compatibility coverage while retaining the existing complete-owner miss and builtin closed-miss coverage. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with one separate HIR rendered declaration/template bridge
family, such as `fn_index`, `global_index`, `template_defs`, rendered
qualified imports, or no-owner handoffs. Keep that packet separate from
`struct_defs` layout compatibility.

## Watchouts

- The retained `struct_defs` layout fallback is still intentionally available
  only after no complete `HirRecordOwnerKey`/TypeSpec owner metadata can be
  formed.
- Complete owner-key miss coverage remains closed for both direct layout lookup
  and builtin layout queries; this packet added only no-owner compatibility
  coverage, not a new fallback path.
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
