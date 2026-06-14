Status: Active
Source Idea Path: ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prepare Closure Evidence

# Current Packet

## Just Finished

Step 2 classification is complete by read-only inspection.

Deletion/demotion proof gate used for every possible candidate:
cross-target route-native positive, missing, mismatch, duplicate, fallback,
wrapper-output, and baseline proof must exist unless the group is strictly
target-local private pass context. No inventoried group satisfies that full
gate. Existing proof is narrower:

- x86 Route 6 proof covers the selected scalar `i32` direct-call argument path
  with positive, absent, invalid, duplicate, prepared-mismatch,
  source-name-mismatch, fallback, `ConsumedPlans`, and wrapper-output coverage.
  It is x86-local and call-argument-specific.
- riscv Route 5/Route 3 proof covers edge/source and memory/source agreement,
  missing, mismatch, duplicate/conflict, incomplete source-memory, fallback,
  and exact wrapper instruction text for the selected edge-publication adapter.
  It is riscv-local and edge-publication-specific.
- Compatibility-retention proof keeps prepared status strings, helper-oracle
  rows, route-debug names, fallback names, and wrapper-output baselines
  explicit beside route-native rows. It preserves public compatibility; it is
  not deletion authority.

Public `PreparedFunctionLookups` classification map:

| Field group | Classification | Evidence | Status |
| --- | --- | --- | --- |
| `call_plans` | blocked public authority | x86 Route 6 now has route-native rows for the selected scalar call-argument path, but public `PreparedCallPlanLookups`, `ConsumedPlans`, helper-oracle statuses, preserved values, outgoing stack arguments, result lanes, and wrapper assembly remain observable compatibility/wrapper authority. | Blocked; proof is x86-local and path-specific, not cross-target call-plan retirement proof. |
| `address_materializations` | target-policy product | Carries frame-slot, global, string, label, GOT/TLS, byte-offset, relocation, and base-plus-offset materialization policy consumed by addressing, calls, variadic entry, and target emission. | Not a deletion candidate; target ABI/layout/address policy must stay outside BIR. |
| `memory_accesses` | blocked public authority | Route 3 source-memory agreement exists for the riscv edge-publication adapter, and prepared helper/status names such as `missing_prepared_memory_access` and `incomplete_prepared_memory_access` remain retained compatibility. | Blocked; proof is riscv-local/adapter-local and compatibility-only outside that route. |
| `move_bundles` | target-policy product | Publishes block-entry, before-call, after-call, before-return, ABI binding, register-bank, lane, and parallel-copy move facts consumed by wrappers and publication helpers. | Not a deletion candidate; no cross-target route-native replacement proves ABI moves, wrapper output, and fallback behavior. |
| `value_homes` | target-policy product | Maps prepared values to registers, stack slots, immediates, and pointer-base-plus-offset homes used by wrappers, edge publications, stack-source publications, and fallback helpers. | Not a deletion candidate; register/stack home policy remains target/prepared-owned. |
| `edge_publications` | blocked public authority | riscv Route 5/Route 3 consumes route facts only when they agree with prepared publication/source-memory authority; prepared publication statuses and fallback remain explicit compatibility. | Blocked; proof is riscv-local and does not prove x86, Route 4/current-block, aggregate-stack, wrapper-output, and baseline parity for deletion. |
| `edge_publication_source_producers` | blocked public authority | Indexes source producer identity for immediates, loads, casts, binaries, and select materialization; riscv adapter uses this only as agreement input beside prepared source-memory authority. | Blocked; source-producer route ownership is not proven cross-target with the full positive/negative/fallback matrix. |

`PreparedBirModule` field-group classification map:

| Field group | Classification | Evidence | Status |
| --- | --- | --- | --- |
| `module` | blocked public authority | Owns the BIR module that route-native facts are derived from and remains the root input for prepared and target consumers. | Not a demotion/deletion candidate in this gate. |
| `route`, `invariants`, `completed_phases`, `notes` | private pass context | These record prepare-run mode, invariant diagnostics, phase markers, and notes rather than public route/source authority. | No implementation idea needed; safe only as private context, not deletion work. |
| `names` | compatibility adapter | Interned function/value/block/link/text IDs are required to bridge BIR spellings to prepared lookup/status helpers and target diagnostics. | Blocked; public status/debug rows still depend on stable prepared naming. |
| `control_flow` | blocked public authority | Provides block labels, dominance, join transfers, branch/edge facts, and function/block mapping used by edge publications, call plans, and wrappers. | Blocked; Route 4/5/6 subsets do not prove whole control-flow-publication replacement. |
| `target_profile` | target-policy product | Drives ABI, register classes, relocation model, legalization, call lowering, and target wrapper choices. | Not a deletion candidate; must remain outside BIR. |
| `stack_layout` | target-policy product | Owns frame objects, slots, offsets, alignment, and frame-size facts used by calls, addressing, variadic entry, helpers, and wrappers. | Not a deletion candidate; layout/stack policy remains target/prepared-owned. |
| `register_group_overrides` | target-policy product | Publishes target register grouping overrides that feed register placement and wrapper/helper policy. | Not a deletion candidate without separate target-policy proof. |
| `regalloc` | target-policy product | Owns assigned registers, register banks/classes, spill/reload, and placement identity used by wrappers, call plans, storage, and helpers. | Not a deletion candidate; register allocation remains target/prepared-owned. |
| `frame_plan` | target-policy product | Owns prologue/epilogue, saved registers, frame-pointer, and callee-save policy. | Not a deletion candidate; ABI/frame policy remains target/prepared-owned. |
| `dynamic_stack_plan` | target-policy product | Publishes dynamic stack and frame-pointer interactions derived from frame policy. | Not a deletion candidate; no route-native replacement proof exists. |
| `storage_plans` | target-policy product | Owns storage and ABI placement plans for special carriers and helper paths. | Not a deletion candidate; target storage policy remains prepared-owned. |
| `value_locations` | target-policy product | Backing store for `value_homes` and `move_bundles`; consumed by calls, edge publications, helpers, wrappers, and fallback. | Not a deletion candidate; no cross-target replacement proves register/stack/move behavior. |
| `addressing` | target-policy product | Backing store for `address_materializations` and `memory_accesses`; carries address base, size, align, volatility, and materialization policy. | Not a deletion candidate; Route 3 proof is riscv-local and address policy remains target/prepared-owned. |
| `liveness` | private pass context | Feeds regalloc, call preservation, and runtime-helper ownership but is not itself a public route diagnostic surface. | Demotion can only be considered as private pass cleanup; no deletion candidate is proven here. |
| `call_plans` | blocked public authority | Backing state for public call lookup group and x86 wrapper compatibility. | Blocked for the same reasons as `PreparedFunctionLookups::call_plans`. |
| `store_source_publications` | blocked public authority | Publishes store-source relationships used by prepared memory/source reasoning and compatibility helpers. | Blocked; no cross-target route-native positive/negative/fallback matrix proves replacement. |
| `variadic_entry_plans` | target-policy product | Uses stack layout, addressing, value homes, and target ABI for variadic entry setup. | Not a deletion candidate; ABI/layout policy remains target/prepared-owned. |
| `i128_carriers`, `f128_carriers` | target-policy product | Carry target storage/register policy for wide scalar lowering and helper ownership. | Not a deletion candidate; no cross-target route-native carrier replacement proof. |
| `atomic_operations` | target-policy product | Publishes target-sensitive atomic lowering facts. | Not a deletion candidate without separate atomic route/emission proof. |
| `intrinsic_carriers` | target-policy product | Carries target-sensitive intrinsic lowering metadata. | Not a deletion candidate without separate intrinsic route/emission proof. |
| `inline_asm_carriers` | target-policy product | Owns inline-asm operand/result homes, register identities, and target constraints. | Not a deletion candidate; formatting/register policy must stay target-local. |
| `f128_runtime_helpers`, `i128_runtime_helpers` | target-policy product | Own runtime-helper ABI bindings, clobber sets, scalar ownership, call preservation, and helper protocol. | Not a deletion candidate; helper ABI/wrapper behavior lacks route-native replacement proof. |

Candidate outcome:

- Deletion candidates: none.
- Demotion candidates ready for implementation ideas: none.
- Private pass context only: `route`, `invariants`, `completed_phases`,
  `notes`, and possibly `liveness` for a future cleanup discussion, but no
  deletion packet is justified by Step 2 because Step 3 owns draft 155 and
  follow-up disposition.
- Blocked public authority: all public lookup groups that expose route/source,
  publication, call, memory, or status semantics remain blocked until full
  cross-target route-native proof replaces or explicitly preserves each public
  behavior.
- Target-policy products: ABI, layout, register, stack, storage, addressing,
  carrier, helper, emission, formatting, and wrapper policy groups remain out
  of BIR ownership and should not be proposed for demotion by this gate.

Step 3 disposition is complete by comparing the Step 2 blocker map with
`ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`.

Draft 155 disposition: keep blocked with named blockers.

Rationale: draft 155 asks for broad `PreparedBirModule` retirement/demotion
analysis after earlier BIR/prealloc phases, but the current field-by-field gate
finds no safe final deletion or demotion candidate. The useful successor work
from ideas 243-246 proves selected x86/riscv route-native diagnostics,
adapters, compatibility-retention rows, and wrapper-output baselines; it does
not prove whole prepared aggregate retirement.

Named blockers for draft 155:

- Public `PreparedFunctionLookups` groups remain observable prepared authority:
  `call_plans`, `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` still lack full cross-target
  route-native positive, missing, mismatch, duplicate, fallback,
  wrapper-output, and baseline proof.
- Public compatibility/status surfaces remain intentionally retained:
  prepared helper/oracle statuses, route-debug names, fallback names, and
  wrapper-output compatibility rows still need stable prepared names and
  status families.
- Target-policy field groups are not BIR retirement candidates:
  address materialization, ABI moves, value homes, stack layout, register
  allocation, frame plans, dynamic stack plans, storage plans, value
  locations, addressing, variadic entry, wide-scalar carriers, atomics,
  intrinsics, inline asm, and runtime helper protocol remain target/prepared
  policy.
- `PreparedBirModule` public/control fields remain blocked:
  `module`, `names`, `control_flow`, `call_plans`, and
  `store_source_publications` lack whole-route replacement proof and cannot be
  hidden behind private wrappers.
- Private pass context groups (`route`, `invariants`, `completed_phases`,
  `notes`, and possibly `liveness`) are not enough to justify an
  implementation idea from this gate because draft 155's broad aggregate
  retirement scope remains blocked.

Follow-up ideas created: none.

No field group is safe for final demotion or deletion. Do not open broad
aggregate retirement from draft 155, do not claim `PreparedBirModule` or
`PreparedFunctionLookups` retirement, do not move target policy into BIR, and
do not weaken expectations or baselines to make a field appear removable.

Step 4 closure evidence is prepared.

Lifecycle pointer check:

- `plan.md` source idea:
  `ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md`.
- `todo.md` source idea:
  `ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md`.
- The field-group inventory, classification map, draft 155 disposition, and
  no-follow-up outcome remain recorded above.

Closure evidence:

- Safe deletion candidates: none.
- Safe demotion candidates: none.
- Draft 155 disposition: keep blocked with the named blockers above.
- Follow-up ideas created: none.
- Closure is ready for plan-owner decision unless lifecycle proof or
  regression comparison finds a blocker.

## Suggested Next

Ask the plan owner to decide closure for idea 247 using the recorded
field-group blocker map, draft 155 disposition, no-follow-up outcome, and
matching lifecycle proof logs.

## Watchouts

Do not convert `target-policy product` into a BIR migration request. The gate
requires preserving target-local ABI, layout, register, stack, storage,
emission, formatting, and wrapper policy outside BIR. Also do not treat
compatibility-retention proof as deletion authority; it is evidence that public
prepared names still matter.

## Proof

Delegated proof completed and preserved in `test_after.log`:

```bash
cmake --build build-x86 --target backend_prepared_lookup_helper_test backend_x86_route_debug_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_x86_route_debug|backend_riscv_prepared_edge_publication)$' | tee test_after.log
```

Result: passed 3/3
(`backend_prepared_lookup_helper`, `backend_x86_route_debug`, and
`backend_riscv_prepared_edge_publication`).

The proof is sufficient for this closure-evidence packet because it uses the
supervisor-selected lifecycle close subset against the prepared compatibility,
x86 Route 6 diagnostic, and riscv Route 5/Route 3 publication surfaces.
