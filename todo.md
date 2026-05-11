Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
template-struct specialization rendered-primary lookup is fenced as explicit
compatibility/no-owner behavior.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/compile_time_engine.hpp` | Renamed the rendered primary-name registration/lookup surface to `*_no_metadata_compat`; owner-key lookup now spells out that complete `CompileTimeRegistryKey` misses return `nullptr` before consulting rendered compatibility. |
| `src/frontend/hir/impl/lowerer.hpp` | Added a `find_template_struct_specializations_no_owner_compat` declaration and documented the lowerer rendered specialization map as a no-owner compatibility mirror. |
| `src/frontend/hir/impl/templates/templates.cpp` | Split the lowerer no-owner fallback into the named compatibility helper; complete owner-key hits still win, and complete owner-key misses return before fallback. |
| `tests/frontend/frontend_hir_tests.cpp` | Updated the compile-time-state closed-miss test to register and query rendered fallback through the explicit no-metadata compatibility API. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Updated the HIR lookup authority test to use the explicit no-metadata compatibility API while keeping the complete owner-key miss closed. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with one separate HIR rendered declaration/template bridge
family, such as `fn_index`, `global_index`, `struct_defs`, `template_defs`,
rendered qualified imports, or no-owner handoffs. Keep that packet separate
from template-struct specialization lookup.

## Watchouts

- `CompileTimeState::template_struct_specializations_` and
  `Lowerer::template_struct_specializations_` still exist as retained rendered
  compatibility mirrors; their removal condition is that all callers can supply
  complete primary owner metadata or an explicitly structured import/handoff
  carrier.
- `Lowerer::eval_deferred_nttp_expr_hir` still reads the lowerer rendered
  specialization mirror through `DeferredNttpTemplateLookup`; that use remains
  no-owner compatibility and was not widened in this packet.
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
