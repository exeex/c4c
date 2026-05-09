# Current Packet

Status: Active
Source Idea Path: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Enforce structured record and typedef precedence

## Just Finished

Step 3 audited the parser-support record and typedef helper paths and found the
current implementation already enforces the required precedence. Structured
typedef overloads for `resolve_typedef_chain` and `types_compatible_p` resolve
only through `std::unordered_map<TextId, TypeSpec>` and do not consult rendered
typedef maps after a TextId miss. Record layout helpers use a direct complete
`record_def` as structured authority and keep rendered map fallback only for
TextId-less legacy/final-spelling carriers; public record lookup rejects stale
rendered tags after structured record metadata misses.

Existing focused stale-fallback tests cover record `record_def` precedence,
structured record miss rejection, typedef TextId precedence, and typedef TextId
miss rejection in `tests/frontend/frontend_parser_tests.cpp`,
`tests/frontend/cpp_hir_parser_support_residual_metadata_test.cpp`, and
`tests/frontend/cpp_hir_parser_type_helper_residual_metadata_test.cpp`.

## Suggested Next

Next packet should execute Step 4 by splitting any remaining
`types_compatible_p` semantic interpretation from parser compatibility bridges,
or prove the current TextId-domain overload and retained rendered-name bridge
already satisfy that step's boundary.

## Watchouts

No code change was needed for Step 3. Do not weaken the existing record
compatibility bridge into a rendered-key fallback after a structured miss:
`resolve_record_type_spec` may scan the compatibility map for matching
structured `TextId`/context carriers, while constant layout remains stricter
and only accepts direct `record_def` or metadata-free legacy spelling.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_support_residual_metadata_test cpp_hir_parser_type_helper_residual_metadata_test && ctest --test-dir build -R 'frontend_parser_tests|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_parser_type_helper_residual_structured_metadata' --output-on-failure > test_after.log 2>&1`.
