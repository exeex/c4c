Status: Active
Source Idea Path: ideas/open/113_bir_prealloc_aggregate_call_boundary_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Create Bounded Follow-Up Ideas

# Current Packet

## Just Finished

Step 3 `Compare Nearby Closed Ideas` reviewed closed ideas 97, 101, 102, 104,
106, 110, and 112 for overlap with the live candidate gap from Step 2:
whether the total outgoing stack argument reservation/address area needs an
explicit prepared ownership/visibility contract.

Overlap decisions:

| Closed idea | Decision | Overlap result |
| --- | --- | --- |
| 97 `bir_prealloc_call_abi_authority_boundary_audit` | Partially covered | Settles the broad split: BIR publishes semantic call ABI facts, while prealloc owns physical arg/result register and stack placement, byval chunks, moves, clobbers, and boundary effects. It does not name a first-class prepared fact for the total outgoing stack reservation/address area. |
| 101 `bir_prealloc_missing_call_abi_fallback_boundary` | Covered for ABI carriers; no new idea | Settles missing call-ABI carrier fallback authority. Ordinary calls depend on explicit BIR ABI carriers; target-sensitive placement and movement remain prealloc authority. This covers source ABI selection/fallback questions but not the aggregate outgoing-reservation total. |
| 101 `closure_note_followup_recovery_audit` | Covered as procedural history; no new idea | Confirms prior follow-ups 100-108 were completed or no-action. It does not add a separate aggregate call-boundary contract. |
| 102 `aapcs64_va_arg_payload_shape_authority` | Covered; no new idea for aggregate `va_arg` homes | Settles AAPCS64 `va_arg` payload ABI/HFA shape authority and leaves only constrained destination-home mapping inside the aggregate `va_arg` access-plan route. Current prepared variadic aggregate access plans already expose payload/source/progression/overflow and destination-home facts. |
| 104 `bir_prealloc_pointer_carrier_provenance_contract` | Covered for pointer/source provenance; no new idea | Settles local-slot pointer preservation, pointer-symbol seeding, and explicit pointer add/sub carrier authority. This supports local aggregate pointer operand source selection without duplicating a new source-selection idea. |
| 106 `prealloc_stack_layout_slice_family_fact_contract` | Covered for frame-slot/source selection; no new idea | Settles structured slice-family coverage and frame-address publication through prepared frame-slot address materialization. Physical frame placement remains prealloc authority. This covers aggregate source selection and frame-slot-backed address materialization, not outgoing call stack reservation totals. |
| 110 `call_planning_frame_address_materialization_authority` | Covered for call argument source selection; no new idea | Removes/narrows local frame-address name fallback as ordinary call-argument authority and proves supported aggregate call argument shapes select from explicit prepared frame-slot materialization facts. No duplicate source-selection follow-up is needed. |
| 110 `aarch64_remaining_baseline_failure_recovery` | Partially covered as recovery context | Recovers the baseline failure family and related AArch64 call-boundary tests, but it is validation/recovery history rather than a prepared ownership contract for total outgoing stack reservation. |
| 112 `aarch64_00216_00204_post_closure_regression` | Partially covered; unresolved shared-contract gap remains | Repairs the concrete AArch64 failures. Its close note explicitly says `00204` required outgoing AArch64 stack argument reservation before `x16`-relative stores, plus stable callee-side `va_list` home preservation. The repair proves the target route, but current prepared plans still expose only per-argument destinations and not the total outgoing reservation/address area as a named prepared fact. |

Step 3 conclusions:

- `covered`: local aggregate pointer/source selection, frame-slot-backed
  aggregate address materialization, byval aggregate transport details, and
  aggregate `va_arg` payload/home preservation are already covered by closed
  work plus current prepared visibility.
- `no new idea`: source selection is covered by ideas 104, 106, and
  110-call-planning plus current prepared frame-slot materialization/source
  selection dumps.
- `no new idea`: byval aggregate transport is covered by idea 97's retained
  prealloc authority for byval chunks/placement and current
  `PreparedAggregateTransportPlan`/prepared-printer visibility.
- `no new idea`: aggregate `va_arg` homes are covered by idea 102 and current
  prepared variadic aggregate access-plan dumps.
- `unresolved`: explicit prepared visibility/ownership for the total outgoing
  stack argument reservation/address area remains a bounded candidate. Existing
  work proves AArch64 needs the reservation before outgoing stack stores, but
  no reviewed closed idea settles whether shared prepared call plans should
  publish that total directly rather than leaving it derivable from
  per-argument destination offsets/sizes.

## Suggested Next

Proceed to Step 4 by creating one bounded follow-up idea for the unresolved
prepared contract: explicit total outgoing stack argument reservation/address
area ownership and dump visibility. Keep it separate from AArch64's `x16`
scratch-base convention and concrete instruction ordering.

## Watchouts

- Do not create duplicate ideas for aggregate source selection, byval
  transport, or aggregate `va_arg` homes; Step 3 found those covered.
- The unresolved gap should be framed as a shared prepared call-plan contract
  only if Step 4 can name the owner boundary, likely files/modules, proof route,
  and overfit reject signals.
- AArch64 still owns physical stack adjustment emission, scratch register
  choice, store instruction sequence, and stack restoration.

## Proof

analysis-only; no build/test run. Historical comparison used closed ideas
97, both numbered 101 notes, 102, 104, 106, both numbered 110 notes, and 112.
No build or test command was run, and no new `test_after.log` was produced for
this analysis-only packet.
