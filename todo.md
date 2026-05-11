Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
HIR module declaration lookup through `fn_index`/`global_index` is fenced as
explicit legacy rendered compatibility while preserving LinkNameId, concrete
GlobalId, and `ModuleDeclLookupKey` authority.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/hir_ir.hpp` | Added explicitly named rendered declaration compatibility helpers for retained `fn_index`/`global_index` lookup and renamed the structured-miss compatibility predicate wording. Source-compatible legacy wrappers remain for rendered-name-only callers outside this packet. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | Added complete structured-miss coverage for the intentionally allowed self-consistent and rendered-qualified compatibility predicates, alongside the existing no-metadata, incomplete-metadata, closed-miss, LinkNameId, concrete-id, and structured-authority assertions. |
| `todo.md` | Recorded this executor packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Continue Step 4 with a separate HIR rendered template bridge packet, such as
`template_defs`, or another rendered-qualified/no-owner handoff family. Keep it
separate from the completed `struct_defs` layout and declaration
`fn_index`/`global_index` compatibility slices.

## Watchouts

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
