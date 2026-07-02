Status: Active
Source Idea Path: ideas/open/521_bir_route8_return_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Route8 Symbols And Dependencies

# Current Packet

## Just Finished

Step 1 - Audit Route8 Symbols And Dependencies is complete. Clang-tools are
available on `PATH`:

- `command -v c4c-clang-tool` -> `/home/vscode/.local/bin/c4c-clang-tool`
- `command -v c4c-clang-tool-ccdb` ->
  `/home/vscode/.local/bin/c4c-clang-tool-ccdb`

AST-backed commands run:

- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool list-symbols src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool-ccdb function-callees ...` and
  `function-callers ...` for route8 helpers and public finders:
  `route8_value_key_matches`, `route8_identity_matches`,
  `route8_key_matches`,
  `route8_return_chain_binary_opcode_is_scalar_publication`,
  `route8_is_named_value`, `route8_value_matches_name`,
  `route8_record_conflicts`, `route8_publish_return_chain_record`,
  `route8_publish_return_chain_records_for_block`,
  `route8_missing_block_status`, `route8_return_chain_value_key`,
  `route8_return_chain_record`, `route8_build_return_chain_index`,
  `route8_find_return_chain_record`,
  `route8_find_return_chain_terminal_value`, and
  `route8_find_return_chain_next_operand_value`.
- `c4c-clang-tool-ccdb type-refs ...` for
  `Route8ReturnChainValueKey`, `Route8ReturnChainRecord`,
  `Route8ReturnChainIndex`, `Route8ReturnChainStatus`, and
  `Route1SourceValueIdentity`.
- `c4c-clang-tool-ccdb function-callers ...` for selected route1,
  route6, route7, and facade symbols:
  `route1_source_value_identity`, `route1_find_same_block_scalar_producer`,
  `route1_build_producer_index`, `route6_build_call_use_source_index`,
  `route7_build_comparison_condition_index`, and
  `route_index_reference_facade`.

Route8 symbol map:

- Declarations remain public in `src/backend/bir/bir.hpp`: records/status at
  lines 5617-5688 and declarations at lines 6221-6247.
- Public route8 definitions in `src/backend/bir/bir.cpp`: value-key builder
  at 4188, record builder at 4210, index builders at 4245 and 4256, record
  finder at 4264, terminal-value finder at 4295, and next-operand finder at
  4305.
- Private route8 implementation helpers in anonymous namespace:
  `route8_value_key_matches` at 3955, `route8_identity_matches` at 3977,
  `route8_key_matches` at 3987,
  `route8_return_chain_binary_opcode_is_scalar_publication` at 4015,
  `route8_is_named_value` at 4047, `route8_value_matches_name` at 4051,
  `route8_record_conflicts` at 4056,
  `route8_publish_return_chain_record` at 4065,
  `route8_publish_return_chain_records_for_block` at 4086, and
  `route8_missing_block_status` at 4155.

Dependency map:

- Route1 identity is the main public dependency. `route8_return_chain_value_key`
  calls `route1_source_value_identity` for the chain value, and
  `route8_return_chain_record` calls it for terminal and optional next operand
  values. Route8 finders return `Route1SourceValueIdentity`.
- Route8 matching uses `Route1SourceValueIdentity` fields directly:
  `name_id`, `name`, `type`, `value`, `integer_constant`, and `operator bool`.
- Route8 body logic uses BIR model types already declared in `bir.hpp`:
  `Function`, `Block`, `Value`, `BinaryInst`, `BinaryOpcode`,
  `TerminatorKind`, and route8 records/status/index.
- Route8 has no AST-backed dependency on route6 bodies, route7 public index
  builders/finders, or route-index facade bodies.
- Route8 does depend on one non-route8 private helper:
  `route8_key_matches` and `route8_missing_block_status` call anonymous
  route7 helper `route7_block_matches` at `bir.cpp` line 3631.

Caller/callee map:

- `route8_value_key_matches`: callees none; callers
  `route8_identity_matches` and `route8_key_matches`.
- `route8_identity_matches`: callees `Route1SourceValueIdentity::operator bool`
  and `route8_value_key_matches`; caller `route8_record_conflicts`.
- `route8_key_matches`: callees `route7_block_matches` and
  `route8_value_key_matches`; callers `route8_publish_return_chain_record`
  and `route8_find_return_chain_record`.
- `route8_return_chain_binary_opcode_is_scalar_publication`: callees none;
  caller `route8_publish_return_chain_records_for_block`.
- `route8_is_named_value`: callees none; callers `route8_value_matches_name`
  and `route8_publish_return_chain_records_for_block`.
- `route8_value_matches_name`: callee `route8_is_named_value`; caller
  `route8_publish_return_chain_records_for_block`.
- `route8_record_conflicts`: callee `route8_identity_matches`; caller
  `route8_publish_return_chain_record`.
- `route8_publish_return_chain_record`: callees `route8_key_matches` and
  `route8_record_conflicts`; caller
  `route8_publish_return_chain_records_for_block`.
- `route8_publish_return_chain_records_for_block`: callees route8 named-value,
  opcode, publish, key, and record helpers; callers both
  `route8_build_return_chain_index` overloads.
- `route8_missing_block_status`: callees `Route1SourceValueIdentity::operator
  bool` and `route7_block_matches`; caller `route8_find_return_chain_record`.
- `route8_return_chain_value_key`: callee `route1_source_value_identity`;
  caller `route8_publish_return_chain_records_for_block`. Raw search also
  shows public consumers in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  and `src/backend/mir/aarch64/codegen/alu.cpp`.
- `route8_return_chain_record`: callees `Route1SourceValueIdentity::operator
  bool`, assignment, and `route1_source_value_identity`; caller
  `route8_publish_return_chain_records_for_block`, with test-only manual
  duplicate construction also present.
- `route8_build_return_chain_index`: callee
  `route8_publish_return_chain_records_for_block`; raw search shows public
  consumers in backend route8 tests and AArch64 ALU codegen.
- `route8_find_return_chain_record`: callees `route8_key_matches` and
  `route8_missing_block_status`; callers route8 terminal/next finders plus
  backend route8 tests and AArch64 ALU codegen.
- `route8_find_return_chain_terminal_value` and
  `route8_find_return_chain_next_operand_value`: both call
  `route8_find_return_chain_record`; external public consumers include backend
  route8 tests and AArch64 ALU codegen helpers.

Extraction safety disposition:

- A focused route8 body extraction is not directly safe if it blindly moves the
  route8 cluster into a separate TU while leaving `route7_block_matches` as an
  anonymous helper in `bir.cpp`.
- The route8 cluster otherwise appears narrowly scoped: no route6, route7
  public builder/finder, route-index facade, tests, expectation, or producer
  behavior changes are needed for the body audit.
- Step 2 should either keep the bodies in `bir.cpp` or select a focused
  boundary that also preserves the block-match dependency without creating new
  route7/facade coupling, for example by introducing a narrow shared/private
  block-match helper rather than moving route8 declarations.

## Suggested Next

Suggested next packet: execute Step 2 - Select Route8 Body Boundary, deciding
whether the `route7_block_matches` dependency should block extraction or be
handled by a narrow shared/private helper boundary.

## Watchouts

The only extraction hazard found in Step 1 is the private
`route7_block_matches` dependency. Do not move route8 declarations out of
`bir.hpp`, change route1 scalar producer identity, touch route6/facade logic,
or rewrite tests/expectations. If Step 2 chooses extraction, it must preserve
the existing route8 block matching semantics and avoid creating broad route7
coupling.

## Proof

No build or ctest proof was required for this audit-only packet. Local proof:
`git diff --check -- todo.md`.
