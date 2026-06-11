Status: Active
Source Idea Path: ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prepared Diagnostic And Oracle Consumers

# Current Packet

## Just Finished

Completed Step 1 inventory for prepared diagnostic and oracle consumers that
still read prepared route facts or `PreparedFunctionLookups`.

Consumer inventory grouped by surface and route family:

- Prepared printer surface [dump-only, debug-only]:
  `src/backend/prealloc/prepared_printer.hpp` exposes
  `prepare::print(const PreparedBirModule&)`, implemented by
  `src/backend/prealloc/prepared_printer.cpp`. The printer appends prepared
  sections through `append_function_summaries`,
  `append_prepared_control_flow`, `append_value_locations`,
  `append_stack_layout`, `append_frame_plan`, `append_dynamic_stack_plan`,
  `append_call_plans`, `append_store_source_publications`,
  `append_select_chain_materializations`, `append_variadic_entry_plans`,
  `append_regalloc`, `append_storage_plans`, special carrier/runtime-helper
  appenders, and `append_addressing`. Route families observed: Route 1
  producer/constant and value-home facts, Route 2 select-chain/direct-global,
  Route 3 memory/addressing/source, Route 4 publication, Route 5 edge/join
  source, Route 6 call-use/call-boundary, Route 7 comparison/condition, plus
  target/prepared-only frame, stack, regalloc, storage, variadic, carrier,
  intrinsic, inline-asm, and runtime-helper facts.

- Prepared printer tests and expected snippets [oracle-only, dump-only]:
  `tests/backend/bir/backend_prepared_printer_test.cpp` is the direct printer
  oracle (`backend_prepared_printer` in `tests/backend/bir/CMakeLists.txt`).
  CLI dump oracles in `tests/backend/bir/CMakeLists.txt` include
  `backend_cli_dump_prepared_bir_is_prepared`,
  `backend_cli_dump_prepared_bir_exposes_contract_sections`,
  `backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg`,
  `backend_cli_dump_prepared_bir_local_arg_call_contract`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
  and prepared focus tests for function, block, and value filtering. These
  assert stable prepared section headers, function summaries, control-flow,
  block-entry publications, stack/frame/dynamic-stack, call plans, storage,
  addressing, publication, branch-condition, join-transfer, and focus
  behavior. Ownership is clear: retained dump/oracle coverage.

- Backend CLI dump routing [dump-only, route-debug-only]:
  `src/apps/c4cll.cpp` owns public flags `--dump-prepared-bir`, `--dump-mir`,
  `--trace-mir`, and focus options. `src/backend/backend.cpp` routes
  `BackendDumpStage::PreparedBir` to `dump_generic_prepared_bir(...)` and
  `prepare::print(prepared)`, while `BackendDumpStage::MirSummary` and
  `BackendDumpStage::MirTrace` use `dump_target_local_prepared_mir(...)` for
  target-local route debug. `dump_stage_uses_target_local_route_debug(...)`
  keeps prepared BIR dumps distinct from MIR summary/trace. Route families:
  all prepared-printer families for `--dump-prepared-bir`; x86 route/debug
  families for `--dump-mir`/`--trace-mir`.

- x86 prepared route-debug surface [route-debug-only, compatibility-only]:
  `src/backend/mir/x86/debug/debug.hpp/.cpp` expose
  `summarize_prepared_module_routes(...)` and
  `trace_prepared_module_routes(...)`; implementation funnels through
  `render_report(...)` and grouped authority reporting such as
  `append_grouped_authority(...)`. Public forwarding exists in
  `src/backend/mir/x86/x86.hpp` and
  `src/backend/mir/x86/codegen/route_debug.hpp`. Tests include
  `tests/backend/bir/backend_x86_route_debug_test.cpp`, handoff checks in
  `backend_x86_handoff_boundary_lir_test.cpp`, multi-defined rejection route
  debug in `backend_x86_handoff_boundary_multi_defined_rejection_test.cpp`,
  and frame/stack/call contract route-debug checks in
  `backend_prepare_frame_stack_call_contract_test.cpp`. Route families:
  wrapper/debug observations over x86 prepared fast-path, Route 6 call-use
  reuse, short-circuit/compare routing, memory-return focus, grouped
  spill/reload, and multi-function rejection summaries. Retain until equivalent
  route-native summary/trace exists.

- `PreparedFunctionLookups` aggregate and direct lookup consumers
  [production-adjacent, compatibility-only, oracle-only]:
  `src/backend/prealloc/prepared_lookups.hpp/.cpp` define
  `PreparedFunctionLookups` with `call_plans`, `address_materializations`,
  `memory_accesses`, `move_bundles`, `value_homes`, `edge_publications`, and
  `edge_publication_source_producers`, built by
  `make_prepared_function_lookups(...)`. AArch64 lowering constructs and
  threads the aggregate in
  `src/backend/mir/aarch64/codegen/traversal.cpp` into
  `module::FunctionLoweringContext` fields declared in
  `src/backend/mir/aarch64/module/module.hpp`. x86 compatibility consumes the
  aggregate through `x86::ConsumedPlans::prepared_lookups`,
  `shared_function_lookups()`, `shared_call_plan_lookups()`,
  `consume_prepared_function_lookups(...)`, and `consume_plans(...)` in
  `src/backend/mir/x86/x86.hpp`. RISC-V still builds prepared lookups in
  `src/backend/mir/riscv/codegen/emit.cpp`. These are not all diagnostics, but
  they block contraction and provide fallback/oracle context for diagnostic
  replacement planning.

- AArch64 diagnostic-adjacent and oracle lookup tests [oracle-only,
  production-adjacent]:
  Prepared lookup threading is asserted across
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `backend_aarch64_branch_control_lowering_test.cpp`,
  `backend_aarch64_current_block_join_routing_test.cpp`,
  `backend_aarch64_prepared_scalar_alu_records_test.cpp`, and
  `backend_aarch64_prepared_memory_operand_records_test.cpp`. These use
  `make_prepared_function_lookups(...)`, attach the aggregate to
  `FunctionLoweringContext`, and sometimes mutate lookup maps for negative or
  fallback cases. Route families: Route 1 producer/constant, Route 2
  select-chain, Route 3 memory/source, Route 4 publication, Route 5
  edge/join-source, Route 6 call-use, Route 7 comparison, plus return-chain and
  target-local home/storage/move facts.

- Prepared lookup helper oracles [oracle-only]:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` is the broad
  prepared helper oracle (`backend_prepared_lookup_helper` in
  `tests/backend/bir/CMakeLists.txt`). It covers aggregate construction and
  positive, negative, ambiguous, invalid, missing-lookup, and fallback statuses
  for call plans, prior preserved values, move bundles, frame-address offsets,
  memory accesses, edge publications, source producers, current/block-entry
  publication facts, same-block producer/source facts, select-chain/direct
  global, store-source publication, fused compare, and x86 Route 6
  `ConsumedPlans` compatibility. This is the main retained oracle surface for
  Step 3 coverage equivalence.

- Target wrapper compatibility tests [compatibility-only, oracle-only]:
  x86 wrappers consume prepared lookups and route views in
  `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`,
  `backend_x86_handoff_boundary_direct_extern_call_test.cpp`, and related
  handoff-boundary tests. RISC-V prepared edge-publication threading is covered
  by `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.
  These are not route-native diagnostics, but they are compatibility consumers
  that Step 2 must either preserve or explicitly exclude from diagnostic
  replacement scope.

- Expected-output and ownership references [oracle-only, compatibility-only]:
  `tests/backend/OWNERSHIP.md` names `backend_prepared_printer`,
  `backend_cli_dump_bir_*`, `backend_cli_dump_prepared_bir_*`, and the
  `backend_cli_dump_mir_*` / `backend_cli_trace_mir_*` route-debug tests as
  retained ownership surfaces. Snippet expectations live inline in CMake test
  definitions rather than separate expected-output files for the inspected
  prepared dump and route-debug CLI cases.

Unclear ownership:

- No separate non-x86 generic route-debug implementation was found beyond the
  shared backend dump dispatcher and x86 target-local summary/trace wrappers.
  AArch64 has `print_prepared_machine_nodes(...)` in
  `src/backend/mir/aarch64/codegen/asm_emitter.*`, but that is selected machine
  node printing rather than a prepared route-debug replacement surface.

## Suggested Next

Execute Step 2 by defining route-native replacement diagnostics for the
prepared printer, CLI prepared dump, x86 MIR summary/trace route-debug, and
retained lookup-helper oracle surfaces named above.

## Watchouts

Several inventory entries are production-adjacent or compatibility-only rather
than diagnostics. Treat them as contraction blockers and fallback/oracle
context, not as permission to implement consumer migrations under this planning
route. Do not weaken inline CMake snippets, prepared-printer tests, x86
route-debug tests, or `backend_prepared_lookup_helper` expectations.

## Proof

Docs/lifecycle-only packet; no build required and no `test_after.log` produced.
Verification was the targeted `rg` inventory plus this canonical `todo.md`
update.
