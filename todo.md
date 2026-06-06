Status: Active
Source Idea Path: ideas/open/113_bir_prealloc_aggregate_call_boundary_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Compare Nearby Closed Ideas

# Current Packet

## Just Finished

Step 2 `Classify Current Contract Ownership` audited current BIR,
prealloc/planning, prepared dump, and AArch64 lowering surfaces for aggregate
call-boundary ownership.

Current owner-layer classifications:

| Target | Primary owner layer | Shared fact currently present | AArch64 ABI/codegen details kept separate | Prepared dump visibility | Concrete unresolved gap |
| --- | --- | --- | --- | --- | --- |
| Frame-slot-backed aggregate address materialization | Shared prealloc/planning fact. | `PreparedCallArgumentSourceSelectionKind::FrameSlotAddress` carries selected frame slot, stack offset, size/align, and materialization provenance from addressing/prealloc. | AArch64 only consumes the selected memory/address fact to form target operands or materialization instructions. | Visible in prepared call plans through `arg.source_selection=frame_slot_address` plus selection slot/offset/materialization fields. | No concrete Step 2 gap. |
| Local aggregate pointer operand selection | Shared prealloc/planning fact. | `call_argument_allows_local_aggregate_address_publication`, `local_frame_address_name_matches`, derived pointer base/delta lookup, and `LocalFrameAddressMaterialization` selection identify local aggregate address operands before target lowering. | AArch64 may decide how to place the pointer register, but should not infer aggregate identity from instruction shape. | Visible as `arg.source_selection=local_frame_address_materialization`, including source value/base, pointer delta, materialization block/inst, frame slot, and byte offset. | No concrete Step 2 gap. |
| Outgoing stack argument area reservation/address ownership | Split owner: target ABI placement plus target codegen emission; shared prealloc owns per-argument destination offsets/sizes. | `PreparedCallArgumentPlan::destination_stack_offset_bytes` and `destination_stack_size_bytes` expose stack-argument destinations; regalloc/call plans compute stack offsets from `passed_on_stack`/byval ABI facts. | AArch64 derives total aligned outgoing bytes, emits `sub sp`, publishes `x16` from adjusted `sp`, adjusts source memory after reservation, stores via the outgoing base, and restores `sp`. | Per-argument destinations are visible as `dest_stack_offset`/`dest_stack_size` and byval transport destination fields. The aggregate reservation total is currently only derivable, not an explicit prepared call-plan field. | Candidate gap: shared prepared call plans do not name the total outgoing stack reservation/address area as a first-class fact; only AArch64 computes it from argument destinations. |
| Byval aggregate stack routes | Split owner: target ABI placement decision plus shared prealloc transport plan. | BIR ABI records `byval_copy`, size/align, register-vs-stack placement, and HFA lane facts; prealloc emits `PreparedAggregateTransportPlan` with payload/copy sizes, source slot/offset, destination stack offset/size, chunks, lanes, and scratch requirements. | AArch64 chooses exact load/store chunks, scratch registers, outgoing base register, and instruction order for stack/register byval traffic. | Visible in prepared dumps as `arg.aggregate_transport=byval_register_lanes`, `transport_source_stack_offset`, `transport_dest_stack_offset`, chunks, lanes, and scratch requirements. | No concrete Step 2 gap beyond the reservation total above. |
| Variadic aggregate `va_arg` payload/home preservation | Split owner: shared BIR/prealloc metadata plus AArch64 ABI lowering. | BIR `CallInst` carries `va_arg_payload_abi`, HFA lane count/size; prealloc `PreparedVariadicAggregateVaArgAccessPlan` preserves source class, payload size/align, destination payload home, source/progression fields, source slot size, copy size/align, overflow stride, and register-save lane homes. `source_va_list` and matching `va_start` destination/home relations are also prepared facts. | AArch64 owns AAPCS64 `va_list` field layout use, register-save/overflow progression, scratch selection, copy loops, and stable va-list base rematerialization from the prepared stack home. | Visible in prepared variadic dumps as `helper_operand kind=va_arg_aggregate`, `aggregate_dst`, `src_va_list`, and `aggregate_access_plan=...` with payload/source/progression/overflow fields. | No concrete Step 2 gap. |
| Prepared dump visibility for aggregate call-boundary facts | Shared proof/inspection surface. | Prepared printers currently expose call argument source selection, byval aggregate transport, per-argument stack destinations, variadic helper homes, and aggregate va_arg access plans. | AArch64 emission-only details are intentionally visible through target instruction/route tests, not as shared prepared facts. | Strong for source selections, byval transport, and variadic aggregate access plans; weaker for an explicit outgoing stack area total/base ownership fact. | Candidate gap: add/compare an explicit prepared-dump fact for total outgoing stack argument reservation only if nearby closed ideas do not already settle it. |

## Suggested Next

Proceed to Step 3 by reviewing closed ideas 97, 101, 102, 104, 106, 110, and
112 for overlap with the one live candidate gap: whether total outgoing stack
argument reservation/address-area ownership needs a first-class prepared
contract/dump fact, or whether current per-argument destination facts plus
AArch64 route tests are already sufficient.

## Watchouts

- Do not collapse AArch64's `x16` scratch convention into a shared contract;
  only the total reservation/address-area fact is a possible shared contract.
- Current prepared dumps already expose most aggregate call-boundary facts.
  Step 3 should reject duplicate follow-up ideas for source selection, byval
  transport, or aggregate `va_arg` homes unless a closed-idea comparison finds
  an uncovered current hole.
- AArch64 ABI details to keep separate: AAPCS64 byval/HFA placement,
  register-save/overflow `va_list` fields, `fp_offset` progression, `x16`
  outgoing base choice, and concrete `sub`/`mov`/store/`add` instruction order.

## Proof

analysis-only; no build/test run. Evidence came from current source inspection
of BIR call ABI metadata, `src/backend/prealloc/call_plans.cpp`,
`src/backend/prealloc/calls.hpp`,
`src/backend/prealloc/variadic_entry_plans.cpp`,
`src/backend/prealloc/variadic.hpp`,
`src/backend/prealloc/prepared_printer/calls.cpp`,
`src/backend/prealloc/prepared_printer/variadic.cpp`, and AArch64
`calls.cpp`/`variadic.cpp`/`machine_printer.cpp`. No new `test_after.log` was
produced for this analysis-only packet.
