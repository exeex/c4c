Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Type-Binding And NTTP Bridges

# Current Packet

## Just Finished

Step 3 fenced the first type-binding bridge slice without broad HIR cleanup.

`same_rendered_type_name_compatibility` is now explicitly documented as
legacy/no-metadata compatibility owned by Sema type-binding equivalence, with
its limitation and removal condition recorded. The final consteval rendered
`TypeBindings` fallback in `resolve_type` now has matching owner, limitation,
and removal-condition notes: complete key/TextId misses and name-mirror misses
fail closed before the rendered map can be consulted.

Focused stale-rendered coverage already existed in
`cpp_hir_sema_consteval_type_utils_structured_metadata` for complete
structured-key and TextId type-binding misses, name mirrors not acting as
ordinary lookup authority, and direct metadata authority winning over rendered
maps, so this packet did not add testcase-shaped coverage.

## Suggested Next

Continue Step 3 with the next bounded Sema-owned type-binding or NTTP bridge:
audit `record_type_binding_mirrors`, `record_nttp_binding_mirrors`, and
`bind_consteval_call_env` output mirrors for any remaining rendered-map
authority after complete metadata misses, without broad HIR lowerer cleanup.

## Watchouts

- The rendered enum bridge is intentionally retained because HIR/static-member
  no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- `TypeSpec` in this build has no legacy `tag` field, so tests cannot
  manufacture a rendered typedef display tag through
  `set_legacy_tag_if_present`; rely on existing key/TextId stale-rendered
  coverage for this ABI and avoid adding impossible rendered-tag assertions.
- Do not broaden Step 3 into HIR lowerer cleanup unless the supervisor assigns
  that scope; the current plan is Sema-owned compatibility retirement.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_parser_tests)$"' > test_after.log 2>&1`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.
