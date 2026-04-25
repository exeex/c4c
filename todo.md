Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add HIR module declaration lookup key

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1. No implementation
packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: add the HIR module declaration lookup key and
hashing/equality helpers without changing lookup behavior.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Do not broaden into struct/type, member/method, parser, or sema rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

No implementation proof yet. First code-changing packet should at minimum run:

- `cmake --build --preset default`
- `ctest --test-dir build -R "frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain" --output-on-failure`
