Status: Active
Source Idea Path: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify Prepared Lookup and Field Groups

# Current Packet

## Just Finished

Completed plan.md Step 3 classification for the remaining prepared lookup and
`PreparedBirModule` field groups.

Inspected source/symbol surfaces:

- `src/backend/prealloc/prepared_lookups.hpp`: public
  `PreparedFunctionLookups` aggregate fields.
- `src/backend/prealloc/prepared_lookups.cpp`: construction and helper
  consumers for call-plan, address-materialization, memory-access, move-bundle,
  value-home, edge-publication, and source-producer lookup groups.
- `src/backend/prealloc/module.hpp`: `PreparedBirModule` field inventory and
  public find helpers for target/prepared products.
- `src/backend/prealloc/calls.hpp`,
  `src/backend/prealloc/addressing.hpp`,
  `src/backend/prealloc/value_locations.hpp`, and
  `src/backend/prealloc/publication_plans.hpp`: lookup struct contents,
  status enums, and status-string helpers.
- `src/backend/mir/x86/x86.hpp`,
  `src/backend/mir/x86/module/module.cpp`, and
  `src/backend/mir/x86/prepared/dispatch.cpp`: x86 `ConsumedPlans`,
  Route 6 scalar agreement use, grouped authority comments, and prepared edge
  publication move-intent status handling.
- `src/backend/mir/riscv/codegen/emit.cpp` and
  `src/backend/mir/riscv/codegen/emit.hpp`: riscv per-function
  `make_prepared_function_lookups(...)`, prepared edge-publication iteration,
  value-home lookups, source-memory facts, and `EdgePublicationMoveIntentStatus`.
- `src/backend/mir/aarch64/codegen/traversal.cpp` plus representative
  AArch64 consumers in `dispatch_edge_copies.cpp`, `dispatch_producers.cpp`,
  `memory.cpp`, `alu.cpp`, and `calls.cpp`: AArch64 keeps local
  `PreparedFunctionLookups` as an adapter surface for edge/source and call
  lowering.
- `src/backend/prealloc/prepared_printer/*.cpp` and tests referenced by `rg`
  under `tests/backend/bir/`: prepared lookup/status rows are externally
  observed by printer, helper-oracle, and target move-intent tests.

AST-backed inspection used:

- `c4c-clang-tool-ccdb list-symbols
  /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp
  build/compile_commands.json`
- `c4c-clang-tool type-refs src/backend/prealloc/module.hpp
  PreparedBirModule -- --std=c++17 -I/workspaces/c4c/src
  -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp
  make_prepared_function_lookups build/compile_commands.json`

`PreparedFunctionLookups` classification:

| Group | Destination | Evidence | Constraint |
| --- | --- | --- | --- |
| `call_plans` | Mixed: private pass context plus compatibility adapter; Route 6 source identity is a BIR-owned index candidate; ABI call layout remains target-policy product. | `make_prepared_call_plan_lookups(...)` indexes prepared calls, outgoing stack areas, immediate args, preserved values, and block-entry republication effects. x86 `ConsumedPlans` exposes `shared_call_plan_lookups()` and uses Route 6 only after prepared `source_value_id` agreement. AArch64 and printers still consume call records directly. | Cannot move as a whole. Call/source identity can migrate behind Route 6 agreement; argument/result registers, outgoing stack areas, preservation, clobbers, grouped spans, and call-boundary status strings must stay stable and target-owned until replacement consumers exist. |
| `address_materializations` | Target-policy product and compatibility adapter. | Built from `PreparedAddressingFunction::address_materializations` by block label; AArch64 globals/lowering and stack/frame offset helpers consume the materialization list with stack-layout and frame-slot facts. | Not a deletion candidate. Frame/global address spelling, offsets, materialization timing, and fallback behavior are target/emission policy, even if selected source identity may later be checked against BIR Route 3. |
| `memory_accesses` | Mixed: BIR-owned semantic index candidate for access/source identity; compatibility adapter and target-policy product for concrete addressing. | Lookup maps by `(block_label, inst_index)`, result value name, and prepared value id. Publication helpers and AArch64/riscv consumers read base kind, frame slot/global/pointer base, byte offset, size, align, volatile/address-space, and materialization flags. | Blocked public authority. Route 3 can own portable memory/source identity only after x86/riscv consumers compare or consume it; concrete base-plus-offset legality, volatile/address-space handling, and emitted addressing constraints must remain stable. |
| `move_bundles` | Target-policy product plus private pass context. | Lookup maps by move phase/block/instruction and indexes before-call argument moves, before-return ABI moves, and after-call result lane bindings. x86/riscv prepared dispatch consumes move intent/status; AArch64 consumes move bundles for edge/call lowering. | Not BIR-owned as a group. BIR may own edge/source facts, but parallel-copy scheduling, ABI moves, return-bank moves, register placement, cycle-temp handling, and emitted move spelling are target policy. |
| `value_homes` | Target-policy product and compatibility adapter; prepared value-id naming is a blocker until BIR has replacement identity indexes. | Lookup maps prepared value id to `PreparedValueHome` and value name to prepared value id. riscv uses it to find pointer-base/register homes; x86 and AArch64 use homes for storage, decoded-home, move, and call logic. | Not deletable. Register/stack/immediate/pointer-base homes are regalloc/storage products. Any BIR-owned identity replacement must preserve current value-name/value-id compatibility and fallback behavior. |
| `edge_publications` | Mixed: BIR-owned index candidate for edge/source/destination identity; compatibility adapter and target-policy product for homes, moves, memory facts, and statuses. | Builder walks control-flow join transfers, attaches destination/source names and ids, value homes, source producer facts, memory access facts, parallel-copy bundle facts, move pointers, Route 4 attribution, and aggregate stack source authority. x86 and riscv move-intent code consume prepared publications and status enums directly. | Whole-group demotion is blocked. Route 4/5 records can become semantic authority only behind agreement gates; public `PreparedEdgePublicationLookupStatus`, x86/riscv `EdgePublicationMoveIntentStatus`, helper-oracle strings, fallback cases, and wrapper output must remain unchanged. |
| `edge_publication_source_producers` | BIR-owned index candidate plus compatibility adapter. | Lookup maps value name to same-block source producer records for load-local, load-global, cast, binary, and select. Prepared helpers use it for current-block publication consumption, same-block scalar producer lookup, integer constant evaluation, global-load access, and load-local stored-value source; AArch64 and riscv consumers still use these prepared facts. | Blocked public authority. Producer/consumer identity belongs in portable BIR route records, but current consumers rely on prepared value names, instruction pointers, same-block fallback rules, and status strings. Do not delete until every source-producer helper has a route-native replacement or adapter. |

`PreparedBirModule` field-group classification:

| Field group | Destination | Evidence | Constraint |
| --- | --- | --- | --- |
| `module` | BIR-owned index / semantic IR. | It is the underlying `bir::Module`; x86 builds Route 6 from the BIR function and prepared lookup construction cross-checks BIR function/block/value names. | Keep as semantic authority. It does not replace prepared target products by itself. |
| `target_profile` | Target-policy product. | Prealloc, regalloc, storage plans, variadic entry, and target codegen use ABI/register/layout properties from the resolved target profile. | Must not move into BIR semantic facts; x86/riscv/AArch64 policy must stay target-owned. |
| `route`, `invariants`, `completed_phases`, `notes` | Private pass context plus diagnostic/status compatibility. | `BirPreAlloc` and phases push invariants, phase names, and notes; printer/debug/test surfaces can observe prepared phase and note text. | Keep private unless a later diagnostic plan proves string-stable replacement. |
| `names` and prepared/BIR name bridging | Compatibility adapter and blocker. | `prepared_lookups.cpp`, label identity, printers, and target consumers translate function, block, value, link, and slot ids between prepared and BIR name tables. | Public authority cannot move yet because prepared value/block/function ids still key lookup tables, diagnostics, and fallback paths. A BIR-owned replacement needs explicit id/name compatibility. |
| `control_flow` | Mixed: BIR-owned index candidate for block/edge/join identity; compatibility adapter for prepared join transfers and block-entry publications. | Edge-publication builders walk prepared join transfers and use prepared block labels; Route 4/5 can describe parts of this but consumers still ask prepared control flow. | Blocked until x86/riscv wrappers consume route-native edge/join facts without changing block-entry fallback/status rows. |
| `value_locations`, `regalloc`, `storage_plans`, `register_group_overrides` | Target-policy product. | Regalloc builds homes/move bundles and storage plans; x86 grouped authority comments, AArch64 lowering, and riscv emission consume register, stack, immediate, pointer-base, and grouped-span products. | Not BIR semantic authority. Only value/source identity may be checked against BIR; allocation, homes, storage encoding, overrides, and grouping are target policy. |
| `stack_layout`, `frame_plan`, `dynamic_stack_plan` | Target-policy product. | Address-materialization helpers, call preservation, variadic entry, x86 frame comments, AArch64 memory lowering, and riscv stack operands read frame slots, offsets, sizes, alignments, saved registers, and dynamic stack operations. | Must remain target/wrapper policy. Do not classify frame or stack products as deletion candidates. |
| `addressing` and `store_source_publications` | Mixed: BIR-owned memory/source facts candidate plus target-policy compatibility adapter. | `addressing` stores prepared memory accesses and materializations; publication/source helpers use it for global-load and load-local source identity, address base kind, offsets, alignment, and store-source publication statuses. | Route 3 can own portable memory/source identity, but concrete address legality and status/fallback strings remain blockers until route-native consumers exist. |
| `call_plans`, `variadic_entry_plans`, `intrinsic_carriers`, `inline_asm_carriers`, `i128_carriers`, `f128_carriers`, `atomic_operations`, `i128_runtime_helpers`, `f128_runtime_helpers` | Target-policy product plus compatibility adapter; selected call-use/source rows are BIR-owned candidates. | Call plans and special carriers encode ABI roles, helper operands, preservation, runtime helper names, atomic/intrinsic/inline-asm lowering, variadic homes, and target helper contracts consumed by x86/AArch64 and prepared helpers. | Route 6 can own call-use source identity only in selected agreement-gated rows. ABI binding, helper-oracle names/statuses, wrapper calls, and runtime-helper choices are target policy and string-sensitive. |
| `liveness` | Private pass context feeding target policy. | Regalloc consumes prepared liveness to allocate/registerize and then publishes regalloc/value-location products. | Keep private unless a later pass split proves no diagnostic or target consumer observes it directly. |

Blocked public authority cannot move yet because the remaining prepared surfaces
serve three roles at once: semantic identity cache, target lowering policy, and
public compatibility/status surface. BIR route records already cover selected
semantic families, but current x86/riscv/AArch64 consumers still read prepared
names, ids, homes, moves, status enums, helper-oracle strings, fallback rows,
printer output, route-debug rows, and target wrapper output. A safe follow-up
must first choose one consumer class, add or use a route-native fact under an
agreement gate, and prove the same public statuses/strings and fallback
behavior before any contraction. No whole-group deletion claim is supported by
this packet.

Deletion candidates found in this packet: none as whole groups. Possible future
deletion candidates are only narrow duplicate semantic indexes after each
consumer class above has a BIR-owned replacement and after string/fallback tests
are preserved.

## Suggested Next

Execute Step 4: split target policy from semantic authority in x86 and riscv
wrappers. Pick one prepared-shape semantic derivation in wrapper code, identify
the BIR route fact it should consume, and keep ABI/layout/register/stack/
emission/formatting policy target-owned.

## Watchouts

- Do not treat `PreparedFunctionLookups` or `PreparedBirModule` as whole-group
  deletion candidates.
- `call_plans`, `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` contain BIR semantic candidates, but each
  also carries compatibility/status or target-policy payload.
- `value_homes`, `move_bundles`, frame/stack/layout/regalloc/storage products,
  special carriers, runtime helpers, ABI decisions, register choices, and
  emitted instruction spelling are target policy.
- Preserve prepared printer output, helper-oracle names/statuses, x86/riscv
  move-intent statuses, supported-path contracts, fallback behavior,
  route-debug output, wrapper output, and baseline expectations.
- Current riscv evidence is prepared edge-publication consumption through
  `make_prepared_function_lookups(...)`, not direct Route 3/4/5/6 consumption.
- Current x86 evidence remains narrow Route 6 scalar `i32` call-use agreement;
  it does not justify broad call-plan, edge-publication, or value-home
  demotion.

## Proof

Analysis-only packet; no build or ctest required by delegated proof. Used
read-only inspection with `rg`, targeted source reads, and the clang-tool
queries listed above. No `test_after.log` was generated because this packet has
no compile/test proof requirement and changed only `todo.md`.
