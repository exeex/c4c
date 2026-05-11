Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 4 - Retire HIR Rendered Declaration and Template Bridges:
Template-global materialization now routes synthesized rendered mangled-name
`GlobalId` lookups through an explicit template-global compatibility boundary
backed by `lookup_global_id_by_legacy_rendered_name`, rather than the broad
`lookup_global_id` alias. Behavior is unchanged for existing and newly lowered
template-global instances.

Changed files:

| File | Result |
| --- | --- |
| `src/frontend/hir/impl/templates/global.cpp` | Added a named rendered-mangled template-global lookup boundary and changed both production materialization lookups to use it. |
| `tests/frontend/frontend_hir_lookup_tests.cpp` | No changes needed; the existing template-global NTTP/TextId test still covers behavior through materialization. |
| `todo.md` | Recorded this reviewer follow-up packet and proof. |
| `test_after.log` | Updated with the delegated build and focused HIR test proof. |

## Suggested Next

Supervisor should review the Step 4 diff and decide whether the remaining
source-compatible broad aliases in `hir_ir.hpp` can stay as compatibility
wrappers or need another narrowly owned retirement packet.

## Watchouts

- `src/frontend/hir/impl/templates/global.cpp` no longer calls
  `lookup_global_id(mangled)`; `rg -n "lookup_global_id\\("
  src/frontend/hir/impl/templates/global.cpp` found no matches after the edit.
- The helper intentionally names the rendered mangled-name boundary because
  template-global instance names are synthesized HIR preservation names, not
  ordinary declaration lookup authority.
- Focused template-global NTTP/TextId behavior remains covered by
  `test_template_global_nttp_init_uses_text_id_function_ctx_binding`.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  was not touched.
- No current blockers.

## Proof

Proof command, logged to `test_after.log`:

`cmake --build build --target frontend_hir_tests frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$'`

Result: passed; 2/2 focused tests passed (`frontend_hir_tests`,
`frontend_hir_lookup_tests`).

Additional check: `git diff --check` passed.
