# Current Packet

Status: Active
Source Idea Path: ideas/open/156_parser_support_constexpr_type_helper_domain_tables.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add parser-local TextId/domain-key helper entry points

## Just Finished

Step 2 added parser-support `TextId` typedef-domain overloads for
`resolve_typedef_chain` and `types_compatible_p` in
`src/frontend/parser/parser_support.hpp` and
`src/frontend/parser/impl/support.cpp`. The retained string-map overloads are
now explicit compatibility bridges, while the structured overloads resolve only
through `std::unordered_map<TextId, TypeSpec>` and treat TextId misses as
authoritative.

Focused tests now cover structured typedef-chain resolution and
type-compatibility misses with stale rendered maps present:
`tests/frontend/frontend_parser_tests.cpp` and
`tests/frontend/cpp_hir_parser_type_helper_residual_metadata_test.cpp`.

## Suggested Next

Next packet should extend the same parser-support domain-table pattern to the
next helper family identified by the supervisor, keeping parser-owned
structured metadata paths separate from rendered compatibility bridges.

## Watchouts

The delegated proof command rebuilds
`cpp_hir_parser_support_residual_metadata_test`, but this packet edits
`cpp_hir_parser_type_helper_residual_metadata_test.cpp`; the edited target was
therefore rebuilt separately before rerunning the exact proof. This branch's
`TypeSpec` no longer has a rendered typedef tag carrier, so tests prove stale
rendered maps are not used by the structured overloads rather than asserting a
synthetic rendered typedef can resolve.

## Proof

Supplemental edited-target build:
`cmake --build build --target cpp_hir_parser_type_helper_residual_metadata_test`.

Delegated proof passed and wrote `test_after.log`:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_support_residual_metadata_test && ctest --test-dir build -R 'frontend_parser_tests|cpp_hir_parser_type_helper_residual_structured_metadata' --output-on-failure > test_after.log 2>&1`.
