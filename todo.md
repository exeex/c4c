Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C declaration-time incomplete object checks now use a declaration-local
completion path instead of `resolve_record_type_spec(...,
struct_tag_def_map)`. Direct complete `record_def` remains authoritative,
current-struct and HIR-deferred template carriers remain allowed, and only
TextId-less legacy carriers can use the parser tag map as parser-local
compatibility. Structured tag/context/qualifier carriers without direct
completion now reject stale parser-map recovery.

Focused parser coverage now checks top-level and local stale-map rejection for
structured tag-only records, plus HIR-deferred template/local typedef cases
needed to preserve EASTL parsing without restoring map authority.

## Suggested Next

Supervisor should review and commit this Step 4C declaration-check slice with
`src/frontend/parser/impl/declarations.cpp`,
`tests/frontend/frontend_parser_tests.cpp`, and this `todo.md` update. The
untracked `review/step4c_repair_route_review.md` remains outside this packet.

## Watchouts

The declaration helper intentionally does not recover structured tag TextId,
namespace, or qualifier carriers through `struct_tag_def_map`. The remaining
fallback is named/commented as parser-local compatibility for TextId-less
legacy declaration carriers only.

EASTL still depends on parser-local grammar deferral for active template-scope
local typedef records whose final `TypeSpec` has no direct record carrier. This
packet preserves that by deferring to HIR/Sema rather than by consulting the
parser record map.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_declarations_residual_structured_metadata|cpp_hir_parser_type_base_residual_structured_metadata|cpp_eastl_vector_parse_recipe|negative_tests_bad_sizeof_incomplete_struct_type|cpp_positive_sema_template_struct_advanced_cpp)$' > test_after.log`

Result: passed, 7/7 tests. Proof log: `test_after.log`.
