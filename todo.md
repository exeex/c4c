# Current Packet

Status: Active
Source Idea Path: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Mark retained string-map overloads as compatibility bridges

## Just Finished

Step 5 marked retained parser-support string-map overloads as explicit
compatibility bridges. The declarations and definitions now state each bridge's
compatibility reason and removal condition: rendered record maps remain only
until callers carry `record_def` or structured record keys, rendered constant
maps remain only until legacy/HIR proof paths pass structured constant
TextIds, and rendered typedef maps remain only until those paths pass
structured typedef TextIds or typed HIR bindings.

No behavior changed. Existing focused coverage already proves structured
metadata misses do not run through rendered fallback recovery for record
layout, const/type binding lookup, and typedef/type-compatibility helpers.

## Suggested Next

Next packet should execute the next supervisor-selected plan step or request a
plan-owner decision if Step 5 completes the current runbook.

## Watchouts

The retained `std::unordered_map<std::string, ...>` overloads are compatibility
bridges only. Do not introduce ordinary parser-owned semantic callers for them,
and do not add rendered fallback recovery after a structured metadata miss.

## Proof

Delegated proof passed and wrote `test_after.log`:
`cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_support_residual_metadata_test cpp_hir_parser_type_helper_residual_metadata_test && ctest --test-dir build -R 'frontend_parser_tests|frontend_parser_lookup_authority|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_parser_type_helper_residual_structured_metadata' --output-on-failure > test_after.log 2>&1`.
