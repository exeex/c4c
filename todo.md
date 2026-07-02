Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit route2 symbols and route6 consumers

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: audited route2 select-chain symbols,
direct-global dependency behavior, route6 consumers, and extraction safety with
AST-backed queries before source inspection.

Clang-tools availability:

- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb` found both
  tools on `PATH` under `/home/vscode/.local/bin`.
- Used `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- ...` to
  map public route2 declarations in `bir.hpp`.
- Used `c4c-clang-tool-ccdb function-signatures`, `list-symbols`,
  `find-definition`, `find-declaration`, `function-callers`,
  `function-callees`, and `type-refs` on
  `/workspaces/c4c/src/backend/bir/bir.cpp` with
  `build/compile_commands.json`.
- Followed with targeted `sed` reads around `bir.hpp` route2/route6 records and
  `bir.cpp` route2/route6 bodies, plus `rg` for cross-TU public consumers.

Public route2 declaration and record map:

- `bir.hpp` keeps the public route2 surface at lines around 4573-4677:
  `Route2SelectChainProducerKind`,
  `route2_select_chain_producer_kind_name`,
  `Route2SelectChainProducerRecord`,
  `Route2SelectChainDirectGlobalDependencyRecord`,
  `Route2SelectChainValueRecord`,
  `Route2SelectChainValueIndex`,
  `Route2SelectChainValueQuery`,
  `route2_select_chain_producer_kind`,
  `route2_select_chain_producer_record`,
  `route2_select_chain_value_record`,
  `route2_build_select_chain_value_index`, and
  `route2_find_select_chain_value_record`.
- `Route6CallArgumentDirectGlobalDependencyRecord` stores a public
  `Route2SelectChainDirectGlobalDependencyRecord`, so the route2 record type
  must remain public.

Route2 body and private helper map:

- Public bodies in `bir.cpp`: `route2_select_chain_producer_kind` at line 172,
  `route2_select_chain_producer_record` at line 194,
  `route2_select_chain_value_record` at line 292,
  `route2_build_select_chain_value_index` at line 332, and
  `route2_find_select_chain_value_record` at line 359.
- Private route2 helper in anonymous namespace:
  `route2_find_direct_global_dependency` at line 227. AST callers are itself
  and `route2_select_chain_value_record`; no route6 or external caller reaches
  it directly.
- Direct callees from route2 are stable public/shared APIs:
  `route1_produced_value`, `route1_find_same_block_scalar_producer`,
  `route1_find_materialization_availability`, and
  `route1_source_value_identity`, plus recursive calls within
  `route2_find_direct_global_dependency`.

Direct-global dependency map:

- `route2_find_direct_global_dependency` recursively follows same-block
  scalar producers through `SelectInst`, `CastInst`, and `BinaryInst`, records
  direct `LoadGlobalInst`, marks local loads as available-without-direct-load,
  and enforces the existing depth limit.
- `route2_select_chain_value_record` owns conversion from that private result
  into `Route2SelectChainValueRecord::direct_global_dependency`, including
  `root_is_select` and `root_instruction_index`.
- Route6 fallback publication uses the public route2 value record, not the
  private helper.

Route6 consumer map:

- AST callers for `route2_select_chain_value_record` are
  `route2_build_select_chain_value_index` and
  `route6_call_argument_direct_global_dependency_record`.
- `route6_call_argument_direct_global_dependency_record` at `bir.cpp` line
  2529 calls `route2_select_chain_value_record(query, call.args[arg_index])`,
  copies `route2.direct_global_dependency`, and only falls back to
  `CallArgumentDirectGlobalSelectChainDependency` relationship metadata when
  the route2 record does not report availability.
- `route6_build_call_use_source_index` calls
  `route6_call_argument_direct_global_dependency_record` while iterating call
  arguments, then later public lookup walks `Route6CallUseSourceIndex::
  direct_global_records`.
- `route6_call_argument_publication_source_record` uses
  `find_call_argument_publication_source_routing` and direct-global routing
  availability, but does not call route2 directly.

Cross-TU public consumers observed by `rg`:

- `src/backend/mir/query.cpp` reads public route2 records and calls the public
  route2 index/value lookup surface.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` calls public
  route2 APIs and compares prepared direct-global/select-chain records.
- `src/backend/bir/bir_printer.cpp` and prepared/MIR tests consume route6
  direct-global relationship/publication fields, not route2 private helpers.

Extraction safety disposition:

- Focused route2 body extraction to `src/backend/bir/bir_route2.cpp` appears
  safe as a body-only move.
- Public declarations, public route2 records/enums, and
  `Route2SelectChainDirectGlobalDependencyRecord` must remain in `bir.hpp`.
- `route2_find_direct_global_dependency` can move with route2 bodies inside the
  new TU's anonymous namespace; it does not require a narrow BIR-private
  boundary because only route2 owns it.
- Route6 must keep consuming route2 through the public
  `route2_select_chain_value_record` and public record types. Do not introduce
  a route6 dependency on route2-private implementation helpers.
- Build metadata will likely need the new route2 TU included for direct-source
  BIR tests that currently compile `bir.cpp` directly.

## Suggested Next

Execute Step 2: select `src/backend/bir/bir_route2.cpp` as the body-only
destination, keep public route2 declarations/types in `bir.hpp`, move the
anonymous route2 direct-global helper with the route2 bodies, and record the
required direct-source test build metadata implication.

## Watchouts

- Preserve public route2 declarations and record types in `bir.hpp`.
- Do not let route6 reach into private route2 implementation details.
- Do not change direct-global dependency classification, route6 publication policy, call ABI behavior, tests, or expectations.
- Route6 has one direct route2 API consumer:
  `route6_call_argument_direct_global_dependency_record`; keep that public
  call shape intact.
- Do not move comparison-only or unrelated route6 helpers into the route2 TU.
- `src/backend/mir/query.cpp` and prepared lookup tests are legitimate public
  route2 consumers, so header surface stability matters beyond route6.

## Proof

No build or ctest proof was required for this audit-only packet. Ran AST-backed
clang-tools mapping commands listed above, targeted source reads, repo-wide
public-consumer `rg`, and `git diff --check -- todo.md`.
