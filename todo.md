Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add HIR module declaration lookup key

# Current Packet

## Just Finished

Step 1: Add HIR module declaration lookup key. Added a HIR-owned structured
module declaration lookup key for functions/globals with declaration kind,
namespace context id, global-qualified state, qualifier segment TextIds,
unqualified declaration TextId, deterministic equality, deterministic hashing,
and a builder from `NamespaceQualifier`. Existing lookup behavior is unchanged.

## Suggested Next

Execute Step 2 from `plan.md`: add declaration-side `name_text_id` metadata to
HIR functions and globals while preserving rendered names unchanged.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Treat `ModuleDeclLookupKey` as a passive key until the next packet explicitly
  wires a mirror map; Step 1 did not populate or query it.
- Do not broaden into struct/type, member/method, parser, or sema rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain)$"`
