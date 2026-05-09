# Current Packet

Status: Active
Source Idea Path: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split type-compatibility interpretation from parser bridges

## Just Finished

Step 4 split `types_compatible_p` typedef-domain selection from semantic
compatibility interpretation. The TextId overload and the rendered-name
compatibility bridge now each resolve typedefs in their own overload before
calling the shared non-template semantic comparator. Parser-owned
`types_compatible_p` still resolves only through
`std::unordered_map<TextId, TypeSpec>` and cannot recover through rendered
typedef maps after a structured TextId miss.

Existing focused coverage already exercises the boundary: stale typedef
TextId miss rejection in `tests/frontend/frontend_parser_tests.cpp` and
`tests/frontend/cpp_hir_parser_type_helper_residual_metadata_test.cpp`, plus
structured nominal identity and stale rendered tag rejection in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`.

## Suggested Next

Next packet should execute the next supervisor-selected plan step or request a
plan-owner decision if Step 4 completes the current runbook.

## Watchouts

The retained `std::unordered_map<std::string, TypeSpec>` overload remains a
documented compatibility bridge for legacy/HIR callers that only have rendered
typedef names. Do not route parser-owned structured TypeSpecs through that
bridge, and do not add rendered fallback recovery after a TextId miss.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_type_helper_residual_metadata_test && ctest --test-dir build -R 'frontend_parser_tests|frontend_parser_lookup_authority|cpp_hir_parser_type_helper_residual_structured_metadata' --output-on-failure > test_after.log 2>&1`.
