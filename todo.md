Status: Active
Source Idea Path: ideas/open/522_bir_route1_scalar_producer_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Route1 Symbols And Dependencies

# Current Packet

## Just Finished

Step 1 - Audit Route1 Symbols And Dependencies completed as an audit-only
packet.

Clang-tools availability:
- `command -v c4c-clang-tool` -> `/home/vscode/.local/bin/c4c-clang-tool`
- `command -v c4c-clang-tool-ccdb` ->
  `/home/vscode/.local/bin/c4c-clang-tool-ccdb`

AST-backed commands/results:
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
  succeeded and found the route1 body cluster in `bir.cpp`, including
  `produced_value_for_route1_producer` at line 29 and public route1 bodies at
  lines 190, 207, 221, 242, 246, 266, 294, 310, and 344.
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
  succeeded and confirmed public route1 body signatures plus the later
  integer-constant wrappers at lines 2480 and 2488.
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
  succeeded and confirmed public declarations remain in `bir.hpp`.
- `c4c-clang-tool function-signatures src/backend/bir/bir_private.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
  succeeded and found only the private `route_block_matches` helper there; no
  route1 declaration depends on `bir_private.hpp`.
- `c4c-clang-tool-ccdb function-callers/function-callees` succeeded for the
  route1 public bodies, route1 private helpers, and route8 TU consumers noted
  below. Querying callees for `route1_source_value_identity`,
  `route1_immediate_integer_constant`, and `route1_producer_kind` returned
  "no callees" where expected because those bodies only build records or switch
  on local values.
- `c4c-clang-tool-ccdb type-refs` succeeded for
  `Route1SourceValueIdentity`, `Route1ImmediateIntegerConstant`,
  `Route1ProducerRecord`, `Route1ProducerIndex`,
  `Route1SameBlockProducerQuery`, and
  `Route1MaterializationAvailability`, including the route8 TU.

Route1 symbol map:
- Private body helper: `produced_value_for_route1_producer(const Inst&)` in
  `bir.cpp:29`, used by `route1_produced_value`.
- Public identity/constant helpers:
  `route1_source_value_identity(const Value&, ValueNameId)` at `bir.cpp:190`,
  `route1_immediate_integer_constant(const Value&, unsigned)` at `bir.cpp:207`,
  and `route1_evaluate_same_block_integer_constant(...)` wrappers at
  `bir.cpp:2480` and `bir.cpp:2488`.
- Public producer classification and records:
  `route1_producer_kind(const Inst&)` at `bir.cpp:221`,
  `route1_produced_value(const Inst&)` at `bir.cpp:242`,
  `route1_producer_instruction_identity(const Block&, size_t)` at
  `bir.cpp:246`, `route1_producer_record(const Block&, size_t)` at
  `bir.cpp:266`, and `route1_build_producer_index(const Block&)` at
  `bir.cpp:294`.
- Public same-block queries:
  `route1_find_same_block_scalar_producer(Route1SameBlockProducerQuery,
  const Value&)` at `bir.cpp:310` and
  `route1_find_materialization_availability(Route1SameBlockProducerQuery,
  const Value&)` at `bir.cpp:344`.
- Private recursive integer evaluator:
  `route1_evaluate_same_block_integer_constant_impl` at `bir.cpp:2243`,
  used only by itself and the two public wrappers.
- Public route1 record/types live in `bir.hpp`: core identity declarations at
  lines 537-568, producer/index/query structs at lines 4480-4530, and route1
  function declarations at lines 4537-4570.

Dependency map:
- Route1 public bodies depend on `Value`, `Inst`, `Block`, `BinaryInst`,
  `LoadLocalInst`, `LoadGlobalInst`, `CastInst`, `SelectInst`,
  `BinaryOpcode`, route1 public record/types, and inline route1 helpers
  `route1_producer_kind_has_materialization` and record `operator bool`
  helpers from `bir.hpp`.
- `route1_producer_record` calls
  `route1_producer_instruction_identity`, `route1_produced_value`,
  `route1_source_value_identity`, `route1_immediate_integer_constant`, and
  `route1_producer_kind_has_materialization`.
- `route1_build_producer_index` calls `route1_producer_record`.
- `route1_find_materialization_availability` calls
  `route1_find_same_block_scalar_producer`.
- `route1_evaluate_same_block_integer_constant_impl` calls
  `route1_immediate_integer_constant`,
  `route1_find_same_block_scalar_producer`, and itself recursively.
- No route1 body calls route2/route4/route5/route6/route7/route8
  implementation helpers. No route1 body calls `route_block_matches` or any
  other `bir_private.hpp` helper.

Caller/callee map:
- `route1_produced_value` is called by `route1_producer_record` and
  `route2_select_chain_producer_record`.
- `route1_producer_kind` is called by
  `route1_producer_instruction_identity`.
- `route1_producer_record` is called by `route1_build_producer_index` and
  `route6_call_argument_source_producer_record`.
- `route1_build_producer_index` is called by
  `route2_build_select_chain_value_index`,
  `route4_build_publication_availability_index`,
  `route5_cfg_edge_publication_record`,
  `route5_current_block_join_source_records`, and
  `route6_build_call_use_source_index`, with additional non-BIR consumers in
  MIR/AArch64 code found by raw search.
- `route1_find_same_block_scalar_producer` is called by
  `route1_find_materialization_availability`,
  route2 direct dependency/value-record code, route4 publication code, route5
  CFG/join code, route6 call-argument-source code, and the private route1
  integer evaluator.
- `route1_find_materialization_availability` is directly called by
  `route2_select_chain_value_record`.
- `route1_source_value_identity` has broad direct downstream consumers across
  route2/3/4/5/6/7 and the route-index facade in `bir.cpp`, plus route8 in
  `bir_route8.cpp`.
- `route1_evaluate_same_block_integer_constant` is not called inside `bir.cpp`;
  raw cross-repo inspection found MIR query and BIR tests as public consumers.

Downstream route consumer map:
- Route2 consumes `route1_produced_value`,
  `route1_find_same_block_scalar_producer`,
  `route1_find_materialization_availability`,
  `route1_source_value_identity`, `Route1SameBlockProducerQuery`, and
  `route1_build_producer_index`.
- Route4 consumes `route1_build_producer_index`,
  `route1_find_same_block_scalar_producer`,
  `route1_source_value_identity`, and route1 producer-kind/source-kind
  conversion.
- Route5 consumes `route1_build_producer_index`,
  `route1_find_same_block_scalar_producer`,
  `route1_source_value_identity`, and route1 producer-kind/source-kind
  conversion.
- Route6 consumes `route1_producer_record`,
  `route1_build_producer_index`, `route1_find_same_block_scalar_producer`,
  `route1_source_value_identity`, and `Route1ProducerRecord`/
  `Route1MaterializationAvailability` fields.
- Route7 consumes `route1_source_value_identity` and
  `Route1SourceValueIdentity` for comparison instruction, operand,
  materialized-condition, branch-condition, and route-index reference
  validation records.
- Route8 is already in `src/backend/bir/bir_route8.cpp`; AST there shows
  `route8_value_key_matches`/`route8_identity_matches` consume
  `Route1SourceValueIdentity`, while
  `route8_return_chain_value_key` and `route8_return_chain_record` call
  `route1_source_value_identity`. Its public terminal/next finders return
  `Route1SourceValueIdentity`.

Extraction safety disposition:
- Focused route1 body extraction appears safe if Step 2 selects a new
  `bir_route1.cpp` owner that includes the public route1 bodies and the private
  route1-only helpers `produced_value_for_route1_producer` and
  `route1_evaluate_same_block_integer_constant_impl`.
- Preserve public route1 declarations and public route1 record/types in
  `bir.hpp`; moving declarations would disturb broad downstream consumers.
- The old comparison-only anonymous helpers at the top of `bir.cpp`
  (`produced_value_for_comparison_producer`,
  `comparison_producer_kind_for_inst`, `is_comparison_binary_opcode`,
  `find_unique_comparison_producer`, and
  `evaluate_comparison_integer_constant`) are not route1 scalar producer
  bodies and should not be swept into the route1 extraction.

## Suggested Next

Execute Step 2 from `plan.md`: select `src/backend/bir/bir_route1.cpp` as the
body-only route1 destination if the supervisor accepts this audit, preserving
all public declarations/types in `bir.hpp` and keeping comparison-only helpers
out of the move.

## Watchouts

Step 3 will need build metadata for any new route1 TU and direct-source BIR
tests that manually compile BIR sources. `route1_evaluate_same_block_integer_constant`
has public consumers outside `bir.cpp`, so do not treat it as dead just because
the AST caller query inside `bir.cpp` finds no local callers.

## Proof

No build or ctest proof required for this audit-only packet.

Validation run:
- `git diff --check -- todo.md`
