Status: Active
Source Idea Path: ideas/open/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prior Closure Facts and Current Surfaces

# Current Packet

## Just Finished

Step 1 from `plan.md` completed an analysis-only inventory of prior closure
facts and current prepared/BIR boundary surfaces. Findings are recorded below
for use by Step 2; no implementation, tests, baselines, source idea, or
`plan.md` changes were made.

### Step 1 Inventory

#### Prior Closure Facts

- Idea 243 closure:
  - BIR semantic candidates are Route 3 memory/source identity, Route 4/5
    edge-publication identity, and Route 6 scalar call-use source identity.
  - Accepted proof was narrow: AArch64 adapters for Routes 3, 4/5, and 6 plus
    narrow x86 scalar `i32` Route 6 route-debug / `ConsumedPlans` prerequisite
    evidence. It did not prove broad x86 wrapper migration, riscv readiness, or
    whole prepared lookup retirement.
  - `PreparedBirModule` and `PreparedFunctionLookups` remained mixed public
    compatibility, private pass context, target policy product, and duplicate
    semantic cache. No whole aggregate, lookup group, or field group was
    deletion-ready.
  - Target policy stayed outside BIR: ABI, layout, register, stack,
    scratch-register, offset, instruction spelling, formatting, wrapper, and
    emission policy.
  - Diagnostics/oracles remained compatibility authority: x86 route-debug
    rows, helper-oracle statuses, `ConsumedPlans`, direct-call wrapper assembly,
    focus behavior, prepared dump equivalence, riscv
    `EdgePublicationMoveIntentStatus`, prepared publication/source-memory
    statuses, fallback behavior, unsupported fail-closed behavior, and exact
    instruction text.
  - Search evidence: `sed -n '187,318p' ideas/closed/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`.
- Idea 244 closure:
  - The selected x86 scalar `i32` direct-call argument path gained Route 6
    status diagnostics and narrow agreement-gated Route 6 source authority.
  - Prepared fallback, public `ConsumedPlans` compatibility, and direct-call
    wrapper output were preserved.
  - Broader public prepared aggregate demotion, draft retirement, riscv parity,
    and cross-target wrapper convergence stayed out of scope.
  - Search evidence: `sed -n '78,132p' ideas/closed/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`.
- Idea 245 closure:
  - riscv gained an adapter seam plus Route 5 edge/source and Route 3
    memory-source agreement diagnostics.
  - Prepared fallback, `EdgePublicationMoveIntentStatus`, prepared status
    strings, and exact riscv instruction text were preserved.
  - Prepared lookup and `PreparedBirModule` field-group deletion, draft 155
    readiness, and broad prepared-field demotion stayed deferred.
  - Search evidence: `sed -n '74,132p' ideas/closed/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`.
- Idea 246 closure:
  - Retained prepared compatibility owners remained explicit in prepared lookup
    helper and target edge-publication tests.
  - x86 Route 6 and riscv Route 5/Route 3 diagnostic rows are separately named
    beside compatibility rows and do not replace prepared fallback authority.
  - Closure did not claim adapter implementation, prepared status deletion,
    aggregate retirement, or draft 155 readiness.
  - Search evidence: `sed -n '78,96p' ideas/closed/246_phase_f1_prepared_publication_status_compatibility_retention.md`.
- Idea 247 closure:
  - No `PreparedBirModule` or `PreparedFunctionLookups` field group was safe
    for final deletion, demotion, or privatization.
  - Draft 155 disposition stayed blocked with named blockers.
  - Durable blockers named public lookup groups `call_plans`,
    `memory_accesses`, `edge_publications`, and
    `edge_publication_source_producers`; compatibility/status surfaces;
    target-policy groups; public/control fields `module`, `names`,
    `control_flow`, `call_plans`, and `store_source_publications`; and private
    pass context groups `route`, `invariants`, `completed_phases`, `notes`, and
    possibly `liveness`.
  - Search evidence: `sed -n '86,126p' ideas/closed/247_phase_f1_final_prepared_field_group_demotion_gate.md`.

#### Current Declaration And Consumer Surfaces

- `PreparedBirModule`
  - Declaration surface: `struct PreparedBirModule` in
    `src/backend/prealloc/module.hpp:38`, with public/control fields
    `module` at line 39, `route` at line 41, `invariants` at line 42, `names`
    at line 43, `control_flow` at line 44, `liveness` at line 48,
    `call_plans` at line 53, `store_source_publications` at line 54,
    `completed_phases` at line 64, and `notes` at line 65.
  - Producer/population bucket: `src/backend/prealloc/prealloc.cpp:23`,
    `:30`, and phase files such as `out_of_ssa.cpp:1557-1560`,
    `legalize.cpp:456-464`, `liveness.cpp:906-1005`,
    `regalloc.cpp:471-821`, `call_plans.cpp:2717-2937`, and
    `publication_plans.cpp:1705-1823`.
  - Consumer buckets: x86 `consume_plans` in `src/backend/mir/x86/x86.hpp:166-182`
    and route-debug entry points at `x86.hpp:258-272`; AArch64 traversal and
    block lowering in `src/backend/mir/aarch64/codegen/traversal.cpp:14-123`;
    riscv edge-publication emission in
    `src/backend/mir/riscv/codegen/emit.cpp:471` and `:764`; tests construct
    and mutate prepared modules across `tests/backend/bir` and
    `tests/backend/mir`.
  - Evidence commands: `rg -n "struct PreparedBirModule|struct PreparedFunctionLookups|call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|module;|names;|control_flow;" src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`;
    `c4c-clang-tool function-signatures src/backend/prealloc/module.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`.
- Public lookup group `call_plans`
  - Declaration surface: `PreparedFunctionLookups::call_plans` in
    `src/backend/prealloc/prepared_lookups.hpp:16`; module field
    `PreparedBirModule::call_plans` in `module.hpp:53`; inline lookup helpers
    `find_prepared_call_plans` in `module.hpp:141` and `:152`; lookup builder
    `make_prepared_call_plan_lookups` and `make_prepared_function_lookups` in
    `src/backend/prealloc/prepared_lookups.cpp`, including return assignment
    at `prepared_lookups.cpp:1538`.
  - x86 consumer bucket: `src/backend/mir/x86/x86.hpp:30-32`, `:66-67`,
    `:166-182`; `src/backend/mir/x86/module/module.cpp:1186-1295`;
    `src/backend/mir/x86/debug/debug.cpp:150-263`; public compatibility tests
    in `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp:432-565`.
  - AArch64 consumer bucket: `src/backend/mir/aarch64/module/module.hpp:83-85`,
    `src/backend/mir/aarch64/codegen/traversal.cpp:60-77`,
    `src/backend/mir/aarch64/codegen/calls.cpp:87`, `:1940`, and
    `:9287-9301`.
  - Prepared compatibility/test bucket:
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1637-1820`,
    `:1980-2073`, and `:8702-8875`; call/frame contract tests such as
    `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:6327-6752`.
  - riscv consumer bucket: no direct riscv source consumer observed for
    `call_plans` in the Step 1 targeted search; riscv evidence is currently
    edge-publication oriented.
  - Evidence commands: `rg -n "call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|find_prepared_call_plans|find_indexed_prepared_memory|find_indexed_prepared_edge|find_prepared_current_block|find_prepared_same_block|find_unique_indexed_prior|find_latest_indexed_prior" src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64 tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`;
    `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json`.
- Public lookup group `memory_accesses`
  - Declaration surface: `PreparedFunctionLookups::memory_accesses` in
    `src/backend/prealloc/prepared_lookups.hpp:18`; lookup builder assignment
    at `prepared_lookups.cpp:1541`; public lookup functions
    `find_indexed_prepared_memory_accesses_by_result_value_name` at
    `prepared_lookups.cpp:1605` and
    `find_indexed_prepared_memory_accesses_by_result_value_id` at
    `prepared_lookups.cpp:1647`.
  - AArch64 consumer bucket: `src/backend/mir/aarch64/codegen/alu.cpp:1039`
    and `:1105-1109`; call/memory fallback source use in
    `src/backend/mir/aarch64/codegen/calls.cpp:8417`; memory consumers in
    `src/backend/mir/aarch64/codegen/memory.cpp`.
  - Prepared compatibility/test bucket:
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:878-945`,
    `:5403-5430`, `:6437`, `:6691-6704`, and `:7583-7827`.
  - riscv consumer bucket: riscv current evidence is indirect through
    `source_memory_access_status` and Route 3 agreement/fallback rows in
    `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1385`,
    `:1468-1508`, `:1528-1566`, and
    `src/backend/mir/riscv/codegen/emit.cpp:165`, `:373`, `:399-443`.
  - x86 consumer bucket: no direct x86 source consumer observed in the targeted
    `memory_accesses` search; x86 prepared memory authority is mainly covered
    by separate handoff-boundary tests and x86 prepared/dispatch surfaces.
  - Evidence command: `rg -n "call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|find_prepared_call_plans|find_indexed_prepared_memory|find_indexed_prepared_edge|find_prepared_current_block|find_prepared_same_block|find_unique_indexed_prior|find_latest_indexed_prior" src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64 tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Public lookup group `edge_publications`
  - Declaration surface: `PreparedFunctionLookups::edge_publications` in
    `src/backend/prealloc/prepared_lookups.hpp:21`; lookup builder assignment
    at `prepared_lookups.cpp:1544`; public lookup declarations such as
    `find_indexed_prepared_edge_publications` in
    `src/backend/prealloc/publication_plans.hpp:576`.
  - riscv consumer bucket: `src/backend/mir/riscv/codegen/emit.cpp:471`,
    `:741-765`; tests in
    `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:288-323`,
    `:342-513`, `:701-740`, `:814-1077`, `:1446-1606`, and `:1698`.
  - x86 consumer bucket: `src/backend/mir/x86/prepared/dispatch.cpp:70`,
    `src/backend/mir/x86/prepared/prepared.hpp:154-186`,
    `src/backend/mir/x86/module/module.cpp:2480-2547`.
  - AArch64 consumer bucket:
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:124-149`,
    `:529-554`; `dispatch_edge_copies.cpp:1166-1279`;
    `memory.cpp:1358-1368`, `:1970-1981`, `:2660-2821`.
  - Prepared compatibility/test bucket:
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1307-1313`,
    `:2132-2162`, `:3250-3589`, and `:4343-5016`.
  - Evidence command: `rg -n "call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|find_prepared_call_plans|find_indexed_prepared_memory|find_indexed_prepared_edge|find_prepared_current_block|find_prepared_same_block|find_unique_indexed_prior|find_latest_indexed_prior" src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64 tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Public lookup group `edge_publication_source_producers`
  - Declaration surface:
    `PreparedFunctionLookups::edge_publication_source_producers` in
    `src/backend/prealloc/prepared_lookups.hpp:22`; lookup builder assignment
    at `prepared_lookups.cpp:1545`; public consumers include
    `find_indexed_prepared_edge_publication_source_producer` and
    `find_prepared_current_block_publication_consumption`.
  - AArch64 consumer bucket:
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:289-305` and
    `:382-399`; `comparison.cpp:442` and `:499`;
    `dispatch_edge_copies.cpp:215-216`; `memory.cpp:3606-3607`,
    `:3894`, `:4958`; `alu.cpp:1108-1109`, `:1835-1837`, `:2434`;
    `calls.cpp:6699`, `:6951-6953`, `:8290`, and `:8632`.
  - Prepared compatibility/test bucket:
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:352-443`,
    `:725-945`, `:6410`, `:6803`, `:7266-7426`.
  - riscv consumer bucket: no direct riscv source-producer lookup use observed
    in the targeted search; riscv consumes prepared edge publications and Route
    5/Route 3 agreement/status around them.
  - x86 consumer bucket: no direct x86 source-producer lookup use observed in
    the targeted source search.
  - Evidence command: `rg -n "call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|find_prepared_call_plans|find_indexed_prepared_memory|find_indexed_prepared_edge|find_prepared_current_block|find_prepared_same_block|find_unique_indexed_prior|find_latest_indexed_prior" src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64 tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- `PreparedBirModule::module`
  - Declaration surface: `src/backend/prealloc/module.hpp:39`.
  - AArch64 consumer bucket:
    `src/backend/mir/aarch64/codegen/traversal.cpp:26` uses
    `prepared.module.functions` for BIR function lookup.
  - x86 consumer bucket: tests mutate BIR through prepared modules, e.g.
    `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp:186-197`
    and `:500-513`; x86 `consume_plans` accepts the whole prepared module in
    `src/backend/mir/x86/x86.hpp:166-182`.
  - Prepared builder bucket: `PreparedBirModule` is produced by
    `prepare_semantic_bir_module_with_options` / `prepare_bir_module_with_options`
    declarations at `module.hpp:411` and `:416`.
  - Evidence commands: `rg -n "struct PreparedBirModule|struct PreparedFunctionLookups|call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|module;|names;|control_flow;" src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`;
    `c4c-clang-tool function-signatures src/backend/prealloc/module.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`;
    `rg -n "struct FunctionLoweringContext|struct BlockLoweringContext|prepared_lookups|call_plan_lookups|call_plans|control_flow|prepared" src/backend/mir/aarch64/module src/backend/mir/aarch64/codegen/traversal.cpp src/backend/mir/x86/x86.hpp`.
- `PreparedBirModule::names`
  - Declaration surface: `src/backend/prealloc/module.hpp:43`.
  - Consumer buckets: x86 name resolution and route debug in
    `src/backend/mir/x86/x86.hpp:50`, `:197`, `:258-272`; AArch64 traversal in
    `src/backend/mir/aarch64/codegen/traversal.cpp:20-21`; AArch64 call and
    memory formatting/lookup use such as `calls.cpp:6863-6871`, `:6952-6953`,
    and `memory.cpp` references from targeted search; prepared printer and
    lookup construction use names throughout `prepared_lookups.cpp`.
  - Evidence commands: `rg -n "struct FunctionLoweringContext|struct BlockLoweringContext|prepared_lookups|call_plan_lookups|call_plans|control_flow|prepared" src/backend/mir/aarch64/module src/backend/mir/aarch64/codegen/traversal.cpp src/backend/mir/x86/x86.hpp`;
    `rg -n "struct PreparedBirModule|struct PreparedFunctionLookups|call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|module;|names;|control_flow;" src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`.
- `PreparedBirModule::control_flow`
  - Declaration surface: `src/backend/prealloc/module.hpp:44`.
  - x86 consumer bucket: `src/backend/mir/x86/x86.hpp:17`,
    `:71-80`, and `:169-170`.
  - AArch64 consumer bucket: `src/backend/mir/aarch64/module/module.hpp:76`,
    `:94`; `src/backend/mir/aarch64/codegen/traversal.cpp:37-44`,
    `:69-75`, `:98-112`.
  - Prepared lookup bucket: dominance and block helpers in
    `prepared_lookups.cpp` from clang signatures, including
    `prepared_block_index_by_label`, `prepared_block_successors`,
    `make_prepared_dominance_matrix`, `prepared_bir_function`, and
    `prepared_bir_block`.
  - Evidence commands: `rg -n "struct FunctionLoweringContext|struct BlockLoweringContext|prepared_lookups|call_plan_lookups|call_plans|control_flow|prepared" src/backend/mir/aarch64/module src/backend/mir/aarch64/codegen/traversal.cpp src/backend/mir/x86/x86.hpp`;
    `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json`.
- `PreparedBirModule::call_plans`
  - Declaration surface: `src/backend/prealloc/module.hpp:53`.
  - Consumer buckets are the same as public lookup group `call_plans`; module
    lookup helper overload for `find_prepared_call_plans` accepting
    `const PreparedBirModule&` at
    `module.hpp:152` is the named declaration surface for module-level
    consumers.
  - Evidence commands: `rg -n "struct PreparedBirModule|struct PreparedFunctionLookups|call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|module;|names;|control_flow;" src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`;
    `rg -n "call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|find_prepared_call_plans|find_indexed_prepared_memory|find_indexed_prepared_edge|find_prepared_current_block|find_prepared_same_block|find_unique_indexed_prior|find_latest_indexed_prior" src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64 tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- `PreparedBirModule::store_source_publications`
  - Declaration surface: `src/backend/prealloc/module.hpp:54`.
  - Producer bucket: `src/backend/prealloc/publication_plans.cpp:1705` clears
    records and `:1823` appends store-source records.
  - Consumer bucket: prepared printer uses
    `append_store_source_publications(out, module)` at
    `src/backend/prealloc/prepared_printer.cpp:52`; source-memory compatibility
    is also tested through prepared lookup helper and riscv edge-publication
    source-memory status rows.
  - Target direct consumer evidence: no direct x86/riscv source consumer of the
    `store_source_publications` module field was observed in Step 1 targeted
    searches; current observed target-facing effect is through prepared
    source-memory lookup/status surfaces.
  - Evidence commands: `rg -n "struct PreparedBirModule|struct PreparedFunctionLookups|call_plans|memory_accesses|edge_publications|edge_publication_source_producers|store_source_publications|module;|names;|control_flow;" src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`;
    `rg -n "ConsumedPlans|Route 6|Route6|route-debug|route_debug|helper-oracle|helper oracle|missing_prepared|incomplete_prepared|unsupported_source_home|fallback|wrapper output|EdgePublicationMoveIntentStatus|prepared status|status" src/backend/mir/x86 src/backend/mir/riscv tests/backend/bir/backend_x86_route_debug_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`.
- Prepared helper/oracle/status/fallback/route-debug/wrapper-output surfaces
  - Declaration/status surfaces: prepared lookup statuses in
    `src/backend/prealloc/prepared_lookups.cpp:464-498`,
    `:1380-1492`, `:1824-1962`, `:2436`; AArch64 machine-node status strings
    in `src/backend/mir/aarch64/codegen/instruction.cpp:442-486` and
    `:853-1004`; riscv `EdgePublicationMoveIntentStatus` in
    `src/backend/mir/riscv/codegen/emit.hpp:26-37`; x86
    `ConsumedPlans` and `EdgePublicationMoveIntentStatus` in
    `src/backend/mir/x86/prepared/prepared.hpp:13`, `:154-186`; x86
    route-debug wrappers in `src/backend/mir/x86/x86.hpp:258-272`.
  - Compatibility/test consumer buckets:
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:11663-11702`
    for compatibility status names and aggregate stack source authority;
    `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp:432-565`
    for Route 6/`ConsumedPlans`/fallback/wrapper output; and
    `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:27-28`,
    `:1593-1681`, `:1723-1738` for riscv helper-oracle/status/fallback rows.
  - Target consumer buckets: x86 module/debug output
    `src/backend/mir/x86/module/module.cpp:2480-2547` and
    `src/backend/mir/x86/debug/debug.cpp:150-263`; riscv emitter
    `src/backend/mir/riscv/codegen/emit.cpp:491-699` and `:741-765`.
  - Evidence command:
    `rg -n "ConsumedPlans|Route 6|Route6|route-debug|route_debug|helper-oracle|helper oracle|missing_prepared|incomplete_prepared|unsupported_source_home|fallback|wrapper output|EdgePublicationMoveIntentStatus|prepared status|status" src/backend/mir/x86 src/backend/mir/riscv tests/backend/bir/backend_x86_route_debug_test.cpp tests/backend/bir/backend_prepared_lookup_helper_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`.
- Target-policy surfaces
  - Declaration/producer surfaces: x86 policy wiring in
    `src/backend/mir/x86/x86.hpp:166-182` and comments at `:286-291`;
    AArch64 context fields in `src/backend/mir/aarch64/module/module.hpp:73-92`;
    AArch64 traversal wiring in `src/backend/mir/aarch64/codegen/traversal.cpp:37-90`;
    riscv instruction policy in `src/backend/mir/riscv/codegen/emit.cpp`.
  - Consumer buckets: x86 ABI/frame/storage/addressing/rendering wrappers,
    AArch64 ABI/register/stack/storage/addressing/carrier/runtime-helper
    lowering, and riscv register move/address materialization/wrapper emission.
  - Evidence command:
    `rg -n "abi|layout|register|stack|storage|address|carrier|runtime helper|format|emission|wrapper|emit|asm" src/backend/mir/x86 src/backend/mir/riscv src/backend/prealloc | head -n 240`.
- Private pass context candidates
  - Declaration surfaces: `route`, `invariants`, `completed_phases`, `notes`,
    and `liveness` in `src/backend/prealloc/module.hpp:41-48` and `:64-65`.
  - Consumer buckets: prepared printer records route, completed phases,
    invariants, and notes in `src/backend/prealloc/prepared_printer.cpp:14-33`;
    liveness is consumed by prealloc/regalloc/helper planning in
    `src/backend/prealloc/call_plans.cpp:634-637`, `:1470-1594`,
    `:2739-2794`; `src/backend/prealloc/f128_runtime_helpers.cpp:190-193`,
    `:1163-1249`; `src/backend/prealloc/i128_runtime_helpers.cpp:209-212`,
    `:1212-1441`.
  - Target direct consumer evidence: no direct x86/riscv public consumer was
    observed for `route`, `invariants`, `completed_phases`, or `notes`;
    `liveness` has substantial prealloc/helper planning consumers, so Step 2
    should split it from the simpler metadata candidates.
  - Evidence command:
    `rg -n "route|invariants|completed_phases|notes|liveness" src/backend/prealloc/module.hpp src/backend/prealloc/*.hpp src/backend/prealloc/*.cpp`.

## Suggested Next

Execute Step 2 from `plan.md`: convert the Step 1 inventory into a boundary
classification matrix, splitting mixed rows instead of leaving any final
classification as `mixed`.

## Watchouts

- Step 1 evidence supports retaining prepared public authority for now; do not
  infer deletion/demotion readiness from the existence of route-native rows.
- `store_source_publications`, `edge_publication_source_producers`, and private
  metadata fields have weaker or indirect target-consumer evidence than
  `call_plans` and `edge_publications`; Step 2 should classify them separately.
- `liveness` should not be lumped with simple metadata (`route`, `invariants`,
  `completed_phases`, `notes`) because helper/runtime planning consumes it.
- Several search commands intentionally used targeted source/test paths. One
  earlier broad `rg` included nonexistent paths/globs (`include`,
  `src/backend/mir/module`, `tests/backend/mir/backend_riscv*`,
  `src/backend/mir/aarch64/src_not_existing`) and returned path errors while
  still producing useful hits; the inventory above cites the cleaned targeted
  commands where possible.

## Proof

No build/test proof required by the delegated packet. Analysis evidence was
recorded from `rg` searches plus these AST-backed queries:

```bash
c4c-clang-tool function-signatures src/backend/prealloc/module.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json
```

No `test_after.log` was produced because the packet explicitly requested no
build/test proof and no root-level log files.
