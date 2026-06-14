Status: Active
Source Idea Path: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Audit x86/riscv BIR Fact Parity

# Current Packet

## Just Finished

Completed plan.md Step 2 parity audit for x86/riscv BIR semantic facts.

Inspected source/symbol surfaces:

- `src/backend/bir/bir.cpp` and `src/backend/bir/bir.hpp`: Route 3 memory
  access records and same-block global/load-local helpers; Route 4 publication
  availability indexes and reference validators; Route 5 edge/join-source
  records; Route 6 call-use source indexes, argument/result records, status
  enums, and lookup helpers.
- `src/backend/prealloc/module.hpp`: `PreparedBirModule` still owns BIR module
  plus names, control-flow, value locations, stack/layout/addressing/liveness,
  regalloc, frame/dynamic-stack, call/store-source/variadic/storage/carrier,
  atomic/intrinsic/inline-asm/runtime-helper, phase, and note fields.
- `src/backend/prealloc/prepared_lookups.hpp` and
  `src/backend/prealloc/prepared_lookups.cpp`: `PreparedFunctionLookups`,
  `make_prepared_function_lookups(...)`, memory-access lookups, edge
  publication lookups, source-producer lookups, Route 4 block-entry attribution,
  and same-block global/load-local prepared helpers.
- `src/backend/prealloc/publication_plans.cpp` and
  `src/backend/prealloc/publication_plans.hpp`: Route 5 join-source agreement
  attachment for current-block parallel-copy source facts and prepared status
  surfaces.
- `src/backend/mir/x86/x86.hpp`: `ConsumedPlans`,
  `consume_prepared_function_lookups(...)`,
  `route6_build_call_use_source_index(...)`, and
  `find_consumed_scalar_i32_call_argument_source(...)`.
- `src/backend/mir/x86/module/module.cpp`: x86 scalar direct extern-call
  argument consumers pass an optional Route 6 record into
  `append_prepared_direct_extern_call_argument(...)`.
- `src/backend/mir/x86/prepared/dispatch.cpp` and
  `src/backend/mir/x86/prepared/prepared.hpp`: x86 edge-publication move intent
  still consumes prepared lookup/value-home facts.
- `src/backend/mir/x86/debug/debug.cpp` and x86 route-debug surfaces: Route 6
  is visible in x86 diagnostics only through agreement-gated scalar `i32`
  helper rows.
- `src/backend/mir/riscv/codegen/emit.cpp` and
  `src/backend/mir/riscv/codegen/emit.hpp`: riscv codegen builds
  `PreparedFunctionLookups` per function, iterates
  `lookups.edge_publications.publications`, and consumes prepared publication,
  source-home, value-home, move, stack, register, pointer, and status facts.

x86/riscv parity classification:

| Fact family | Classification | Evidence | Follow-up/blocker disposition |
| --- | --- | --- | --- |
| Route 3 memory/source identity | Unconsumed BIR fact | BIR has portable Route 3 access records and same-block global/load-local helpers. E5/240 proved one AArch64 global-load agreement-gated adapter. x86 inspection found no Route 3 consumer. riscv `render_edge_publication_source_operand(...)` reads prepared `PreparedEdgePublication` memory fields such as source producer kind, memory status, base kind, byte offset, size, alignment, volatile/address-space, and materialization flags. | Follow-up should target a selected riscv or x86 memory/publication reader that can compare Route 3 with prepared memory access before consuming it. No riscv parity claim yet. |
| Route 4/5 publication and edge/join identity | Unconsumed BIR fact | BIR Route 4 and Route 5 indexes exist. Prepared has Route 4 block-entry attribution and Route 5 current-block join-source agreement attachment. E5/241 proved one AArch64 Route 5 current-block join-source adapter. x86 prepared dispatch and riscv emit still consume `PreparedFunctionLookups::edge_publications` and prepared move/value-home policy. | Follow-up should split semantic publication/source identity from target move emission for one x86 or riscv wrapper reader. Whole `edge_publications`/`edge_publication_source_producers` demotion is blocked. |
| Route 6 scalar call-use source identity | Unconsumed BIR fact | BIR Route 6 builds argument, producer, direct-global dependency, publication-source, result, and lane records. x86 `ConsumedPlans` builds the index and the scalar `i32` direct-call path uses `find_consumed_scalar_i32_call_argument_source(...)` only when Route 6 source value id agrees with prepared call-plan `source_value_id`. E5/242 is AArch64 call-result evidence. riscv has no Route 6 reference. | Follow-up can extend the x86 scalar agreement boundary or add a riscv selected call-use consumer, but must not treat x86-only or AArch64-only Route 6 proof as riscv parity. |
| Route/source/block/value identity in `PreparedBirModule` | Missing BIR fact | The BIR module has intrinsic function/block/value structure, but `PreparedBirModule` still owns the durable cross-pass name tables, prepared ids, control-flow lookup state, value-home ids, and target-prepared value-location products used by wrappers and diagnostics. No portable BIR index currently replaces these prepared identity tables across x86/riscv. | Follow-up should classify individual name/control-flow/value-id surfaces in Step 3. Broad aggregate retirement is blocked. |
| Producer/consumer relationships | Portable BIR fact, partly unconsumed | Route 1 producer identity underlies Route 3/4/5/6 records and Route 5/6 include source-producer relationships. Consumers remain mostly prepared/AArch64-selected; x86 consumes Route 6 scalar `i32` only, riscv consumes prepared edge-publication source producers. | Treat route records as portable semantic candidates, but require target-specific adapter follow-ups before claiming x86/riscv parity. |
| Diagnostics, oracle, fallback, and expected-string surfaces | Blocked by diagnostics/fallback/tests | Prepared printer, x86 route debug, helper-oracle/status names, riscv `EdgePublicationMoveIntentStatus`, prepared lookup statuses, fallback behavior, wrapper output, and expected strings still observe prepared structure. Route 4/5/6 status enums exist but do not replace public prepared diagnostics across both targets. | Step 5 must define string-stable route-native replacements and fallback tests before any public prepared diagnostic/oracle contraction. |
| Target wrapper consumers | Target-policy fact | x86 and riscv wrappers consume prepared facts for ABI registers, stack/frame layout, value homes, move scheduling, scratch registers, concrete load/store/move spelling, offsets, pointer-base policy, and emitted assembly. riscv has no BIR route consumers; x86 has only a Route 6 scalar agreement assist while still retaining prepared wrapper policy. | Keep ABI/layout/register/stack/emission/formatting/wrapper policy target-owned. Follow-ups should make target wrappers consume BIR semantic facts only behind agreement gates. |

## Suggested Next

Execute Step 3: classify `PreparedFunctionLookups` and `PreparedBirModule`
field groups by destination: private pass context, BIR-owned index,
target-policy product, compatibility adapter, deletion candidate, or blocker.
Start from `PreparedFunctionLookups::{call_plans,address_materializations,
memory_accesses,move_bundles,value_homes,edge_publications,
edge_publication_source_producers}` and the `PreparedBirModule` field groups
named above.

## Watchouts

- This is analysis-only; do not implement adapter, demotion, or deletion work.
- Do not claim riscv readiness from x86-only evidence.
- Keep ABI, layout, register, stack, emission, formatting, and wrapper policy
  target-owned.
- Preserve strings, helper-oracle statuses, supported-path contracts, fallback
  behavior, route-debug output, wrapper output, and baseline expectations.
- Current riscv evidence is prepared edge-publication consumption, not BIR
  Route 3/4/5/6 consumption. Do not infer riscv parity from it.
- Current x86 route evidence is narrow Route 6 scalar `i32` call-use identity,
  not Route 3/4/5 parity and not broad Route 6 wrapper readiness.
- Closed 240-242 are AArch64-selected adapters. They provide agreement-gated
  migration patterns and blocker categories, not x86/riscv parity.
- `PreparedFunctionLookups` likely contains mixed destinations: semantic route
  candidates, transient pass context, target-policy products, compatibility
  adapters, and diagnostic/oracle blockers. Step 3 should avoid whole-group
  deletion claims unless every consumer class is accounted for.

## Proof

Analysis-only packet; no build or ctest required by delegated proof. Used
read-only inspection with `rg` and targeted source reads. No `test_after.log`
was generated because this packet has no compile/test proof requirement and
changed only `todo.md`.
