Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR `Module::template_defs` reads for rendered `source_template` and
template-info names now go through an explicitly named rendered-preservation
helper while preserving deferred instantiation behavior and output spelling.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/hir_ir.hpp` | Documented `template_defs` as a rendered HIR preservation map and added `find_template_def_by_rendered_preservation_name` accessors. |
| `src/frontend/hir/hir_types.cpp` | Routed template-instantiation LinkNameId lookup through the rendered-preservation helper. |
| `src/frontend/hir/impl/compile_time/engine.cpp` | Routed deferred-instantiation template metadata reads through the helper and documented that CompileTimeState remains AST definition authority. |
| `tests/frontend/frontend_hir_tests.cpp` | Updated template-definition preservation coverage to use the named helper while retaining output/link-name assertions. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Updated deferred template-call instantiation coverage to use the named helper while retaining TextId NTTP preservation assertions. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with the next rendered-qualified/no-owner handoff family after
the completed `struct_defs`, declaration `fn_index`/`global_index`, and
`template_defs` compatibility slices.

## Watchouts

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
