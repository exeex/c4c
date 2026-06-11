Status: Active
Source Idea Path: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory lookup groups and readers

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: inventoried the
`PreparedFunctionLookups` aggregate fields, construction/publication paths, and
current reader categories.

Declaration and construction:

- Aggregate declaration:
  `src/backend/prealloc/prepared_lookups.hpp` declares seven lookup groups:
  `call_plans`, `address_materializations`, `memory_accesses`, `move_bundles`,
  `value_homes`, `edge_publications`, and
  `edge_publication_source_producers`.
- Aggregate construction:
  `make_prepared_function_lookups(...)` in
  `src/backend/prealloc/prepared_lookups.cpp` builds every group from the
  per-function prepared module state. It finds call plans, value locations, and
  addressing; builds value-home lookups first; uses them while building memory
  accesses and edge publications; separately builds move bundles and source
  producers; then returns the aggregate.
- Production publication/threading:
  AArch64 traversal constructs one stack-local aggregate per prepared function,
  stores `FunctionLoweringContext::prepared_lookups`, and also publishes
  separately rebuilt domain pointers for `call_plan_lookups`,
  `address_materialization_lookups`, `move_bundle_lookups`, and
  `value_home_lookups`.
- Target-wrapper publication:
  x86 stores `std::optional<PreparedFunctionLookups>` in
  `ConsumedPlans`, exposes it through `shared_function_lookups()`, and exposes
  `call_plans` through `shared_call_plan_lookups()`. RISC-V prepared emission
  builds local lookups with `make_prepared_function_lookups(...)` and passes
  pointers into edge-publication helpers.

Lookup group inventory and current reader set:

| Lookup group | Exposed sublookups/helpers | Production readers | Printer/debug readers | Target-wrapper readers | Oracle-test readers | Unknown/none |
| --- | --- | --- | --- | --- | --- | --- |
| `call_plans` | `PreparedCallPlanLookups::{calls_by_position,outgoing_stack_argument_areas_by_position,immediate_arguments_by_position_and_abi,prior_preserved_by_value,first_stack_preserved_by_call_index,block_entry_republication_effects_by_block}` plus `find_indexed_prepared_call_plan`, immediate-argument, prior-preserved, first-stack-preserved, outgoing-stack-area, and block-entry-republication helpers. | AArch64 `calls.cpp` consumes the projected `context.function.call_plan_lookups` through helper calls. Construction in AArch64 traversal rebuilds this group directly, not by projecting from the aggregate. | None found as a direct `PreparedFunctionLookups::call_plans` reader. | x86 `ConsumedPlans::shared_call_plan_lookups()` returns `&shared_function_lookups()->call_plans`; `find_consumed_call_plan` and related wrapper helpers consume it through the adapter. | AArch64 branch/current-block/dispatch tests attach `&prepared_lookups.call_plans`; dispatch tests also inspect the projected pointer. | No unknown readers found. |
| `address_materializations` | `PreparedAddressMaterializationLookups::materializations_by_block` plus indexed address-materialization and frame-address-offset helpers. | AArch64 `globals.cpp`, `memory.cpp`, and `memory_store_retargeting.*` consume the projected `context.function.address_materialization_lookups` through helpers. Traversal rebuilds this group directly. | None found as a direct aggregate reader. | None found. | AArch64 branch/current-block/dispatch tests attach `&prepared_lookups.address_materializations`; prepared lookup helper tests use standalone address-materialization lookups for oracle coverage. | No unknown readers found. |
| `memory_accesses` | `PreparedMemoryAccessLookups::{accesses_by_position,accesses_by_result_value_name,accesses_by_result_value_id}` plus indexed/unique memory-access lookup helpers. | AArch64 `alu.cpp` directly reads `context.function.prepared_lookups->memory_accesses` for return-chain/value-home fallback paths. Shared publication-plans helpers consume memory-access lookup pointers when building store-source publication facts. | None found. | None found. | `backend_prepared_lookup_helper_test.cpp` reads `&lookups.memory_accesses`; store-source publication tests build standalone memory-access lookups. | No unknown readers found. |
| `move_bundles` | `PreparedMoveBundleLookups::{bundles_by_position,before_call_argument_moves_by_position_and_abi,before_return_abi_moves_by_source_and_bank,after_call_result_lane_bindings,after_call_result_lane_bindings_by_position_and_value}` plus indexed move-bundle, call-argument move, return-ABI move, and after-call lane helpers. | AArch64 `memory.cpp`, `calls.cpp`, `alu.cpp`, and `module.cpp` consume the projected `context.function.move_bundle_lookups` through helpers. Traversal rebuilds this group directly. | None found as a direct aggregate reader. | None found. | AArch64 branch/current-block/dispatch tests attach `&prepared_lookups.move_bundles`; dispatch tests install empty move lookups for negative cases; prepared lookup helper tests cover standalone move-bundle lookups. | No unknown readers found. |
| `value_homes` | `PreparedValueHomeLookups::{homes_by_id,value_ids}` plus indexed value-home/id lookup helpers and decoded-home/formal-publication inputs. | Many AArch64 production files consume projected `context.function.value_home_lookups`: `memory.cpp`, `calls.cpp`, `comparison.cpp`, `dispatch_producers.cpp`, `dispatch_publication.cpp`, `select_materialization.cpp`, `operands.cpp`, `prologue.cpp`, and `dispatch_lookup.cpp`. AArch64 memory operand record helpers also read `lookups->value_homes` when a fixture passes the aggregate. | None found. | x86 prepared wrapper helpers pass `value_home_lookups = nullptr` in several decoded-home adapter inputs, so no aggregate value-home wrapper reader was found. | AArch64 branch/current-block/dispatch tests attach `&prepared_lookups.value_homes`; prepared-memory-operand tests set fixture inputs from `lookups.value_homes`; BIR/prealloc tests build standalone value-home lookups for oracle coverage. | No unknown readers found. |
| `edge_publications` | `PreparedEdgePublicationLookups::{publications,publications_by_edge_destination}` plus indexed/unique edge-publication, edge-copy-source-facts, block-entry parallel-copy, stack-source, and publication-match helpers. | AArch64 `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, and `memory.cpp` directly read `context.function.prepared_lookups->edge_publications`, sometimes with local rebuilt fallback lookups. Shared publication-plans helpers consume this group through helper inputs. | None found. | RISC-V prepared emission reads `lookups->edge_publications` and iterates local `lookups.edge_publications.publications`; x86 prepared dispatch reads `lookups->edge_publications` through `ConsumedPlans::shared_function_lookups()`. | RISC-V and x86 wrapper tests read `lookups.edge_publications`; AArch64 dispatch tests mutate/read `prepared_lookups.edge_publications.publications`; prepared lookup helper tests use `lookups.edge_publications` extensively. | No unknown readers found. |
| `edge_publication_source_producers` | `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name` plus source-producer, same-block scalar producer, current-block publication-consumption, select-chain/direct-global dependency, scalar-materialization, store-source, comparison, and call/publication helper APIs. | AArch64 `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, `memory.cpp`, `calls.cpp`, `comparison.cpp`, and `alu.cpp` directly read `context.function.prepared_lookups->edge_publication_source_producers` or use a locally generated fallback. Shared prepared helpers in `select_chain_lookups`, `call_plans`, `publication_plans`, `addressing`, and `comparison` consume source-producer pointers. | `src/backend/prealloc/prepared_printer/select_chains.cpp` consumes `PreparedEdgePublicationSourceProducerLookups` directly for select-chain printer/debug output. | None found as a x86/RISC-V wrapper field reader of the aggregate source-producer group. | AArch64 dispatch tests mutate/read `prepared_lookups.edge_publication_source_producers`; prepared lookup helper, store-source publication, prepared-memory-operand, and frame/stack/call contract tests build or pass source-producer lookup groups directly. | No unknown readers found. |

Reader mode summary:

- Direct aggregate field readers: AArch64 production code reads
  `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers`; AArch64 `alu.cpp` can also construct a
  local compatibility aggregate fallback and read its source producers.
- Helper/projection readers: AArch64 production usually consumes
  `call_plans`, `address_materializations`, `move_bundles`, and `value_homes`
  through projected domain pointers on `FunctionLoweringContext`, not by
  dereferencing the aggregate field at the use site.
- Compatibility adapters: x86 `ConsumedPlans` wraps the full aggregate and
  exposes `shared_function_lookups()`/`shared_call_plan_lookups()`; RISC-V
  prepared emission builds local aggregate lookups for edge-publication
  consumption.
- Oracle-test readers: backend BIR/prealloc tests and AArch64/x86/RISC-V
  target tests directly build, attach, mutate, or inspect aggregate lookup
  fields for coverage.

## Suggested Next

Execute Step 2 by classifying each lookup group as BIR annotation,
transient pass context, target/prepared policy, compatibility adapter,
retained oracle, or blocked/unknown. Use the reader-mode split above to avoid
mistaking projected helper use for ownership readiness.

## Watchouts

- Do not rename, split, delete, or privatize `PreparedFunctionLookups` fields
  under this audit.
- Do not classify target/prepared policy as BIR-owned just because a selected
  route consumer moved.
- Do not weaken oracle coverage, tests, unsupported markers, or expectations to
  make a lookup group appear ready.
- `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` still have direct AArch64 aggregate
  readers; aggregate contraction cannot treat the projected-pointer fields as
  representative of the whole type.
- `edge_publication_source_producers` is also a printer/debug input; Step 2
  should distinguish printer/debug retention from production fallback needs.
- x86 and RISC-V are compatibility-adapter readers of the aggregate, not proof
  that all aggregate fields have cross-target semantic route ownership.

## Proof

Docs/lifecycle-only inventory packet. No build/test proof was required and no
`test_after.log` was generated. Validation/searches performed:

- `rg -n "PreparedFunctionLookups" -S .`
- `rg -n "prepared_function_lookups|function_lookups|lookups\\." ...`
- field-specific `rg` scans for `call_plans`, `address_materializations`,
  `memory_accesses`, `move_bundles`, `value_homes`, `edge_publications`, and
  `edge_publication_source_producers`
- targeted reads of `prepared_lookups.hpp/.cpp`, AArch64 traversal/context,
  x86 `ConsumedPlans`, RISC-V prepared edge-publication fixtures, and the
  lookup owner headers for calls, addressing, value locations, publication
  plans, and select-chain/source-producer helpers
