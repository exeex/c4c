Status: Active
Source Idea Path: ideas/open/05_prepared_call_argument_source_selection_completeness.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Selection Kinds And Consumers

# Current Packet

## Just Finished

Completed Step 1 audit for `plan.md`: enumerated
`PreparedCallArgumentSourceSelectionKind` variants, current payload facts,
AArch64 consumers, and print/diagnostic visibility.

| Selection kind | Prepared payload currently carried | AArch64 facts required | Completeness |
| --- | --- | --- | --- |
| `None` | no payload; default/absence sentinel | no emission source should be selected from this kind | Complete as sentinel only. |
| `FrameSlotValue` | source value id/name/home kind, source slot, stack offset, size, align, base/delta when available | `make_selected_frame_slot_source(... FrameSlotValue ...)` requires slot, offset, size, align, source id/name, and stack-slot home kind | Complete when produced; selected consumers fail closed if any required field is absent. Older lookup/rederivation remains only on the no-selection path. |
| `FrameSlotAddress` | source slot, stack offset, size, align from sret memory return or frame-slot address materialization; materialization block/index/slot/offset when available | `make_selected_frame_slot_source(... FrameSlotAddress ...)` requires slot, offset, size, align and uses materialization coordinates for the memory operand | Complete for selected sret/frame-slot-address sources. No-selection path still searches materializations for legacy compatibility. |
| `LocalFrameAddressMaterialization` | source identity, materialization block/index/frame slot/byte offset, source slot/offset when derivable, size, align | `make_selected_local_frame_address_source` requires size, align, frame slot, and byte offset | Complete for a selected local-frame address source. Watch the current `calls_moves.cpp` fallback that can still treat this kind as permission to use a callee-saved prior record when the local-frame source did not produce an operand. |
| `FrameSlotValue` local aggregate address fallback | not a distinct enum kind; consumers admit `FrameSlotValue` and `PriorPreservation` in `make_call_move_local_aggregate_frame_address_source` / dispatch bridge helper | local object and lane stack slot are discovered from prepared stack layout by name | Not a selection payload family; this is a consumer fallback path and should not be expanded as source-selection completion work. |
| `ByvalRegisterLane` | source identity/home fields, lane extent, lane source instruction index, and frame-slot facts copied from memory access when available | `make_byval_register_lane_prepared_source` requires extent, slot, offset, source size, align, and size coverage; `selected_byval_lane_extent_bytes` consumes extent | Mostly complete when memory-access facts exist. Visibility gap remains: if selected but extent is absent, `selected_byval_lane_extent_bytes` may still rederive from payload stores rather than exposing a missing prepared fact. |
| `PriorPreservation` / stack-slot route | source value id/name, route, prior call block/instruction, preserved stack slot, offset, size, align | `make_prior_preserved_call_argument_source(selection)` requires route `StackSlot`, source id/name, stack slot, offset, size, align, and nonzero size | Complete for stack-slot prior preservation; incomplete selections fail closed and the existing test expects no target-local rederivation. |
| `PriorPreservation` / callee-saved-register route | source value id/name, route, prior call block/instruction; no register name, register bank, typed placement, contiguous width, or occupied register list | AArch64 register source construction requires `register_name`, `register_placement`, `register_bank`, `contiguous_width`, `occupied_register_names`, plus value id/name/view for `make_register_operand_from_prepared_authority` | First incomplete selection family. The selection names a callee-saved prior-preservation source but does not carry the register operand facts needed to emit it. |

First incomplete selection family: `PriorPreservation` with
`PreparedCallPreservationRoute::CalleeSavedRegister`.

Narrow consumer path proving the missing fact is required:
`lower_before_call_move` sees an explicit `argument->source_selection`,
calls `make_prior_preserved_call_argument_source(context, selection, ...)`,
and that overload only accepts `PriorPreservation` with route `StackSlot`.
For a callee-saved route it returns `nullopt`, so `calls_moves.cpp` falls back
through `find_prior_preserved_value_for_call_argument(...)` and then calls the
`PreparedCallPreservedValue` overload, which has the register facts needed by
`make_register_operand_from_prepared_authority`. This proves the prepared
selection itself is missing the callee-saved register source facts.

Printer/diagnostic visibility gaps:

- `prepared_printer/calls.cpp` prints call argument source/destination fields
  and preserved values, but does not print `arg.source_selection`, its kind, or
  any selection payload fields. The enum-name helper exists in `calls.hpp` but
  is not used on this surface.
- Incomplete selected sources usually fail by returning `std::nullopt`.
  The selection overload of `make_prior_preserved_call_argument_source` ignores
  diagnostics, so missing route/payload fields are not reported as prepared
  selection incompleteness.
- Existing tests assert the fail-closed behavior for an incomplete stack prior
  selection with no diagnostics; there is no printer/diagnostic check that
  would make the missing callee-saved selection facts visible.

## Suggested Next

Execute Step 2 by extending `PreparedCallArgumentSourceSelection` for the
callee-saved `PriorPreservation` route with ABI-neutral register source facts
already present on `PreparedCallPreservedValue`, then populate those fields in
`select_prepared_call_argument_source` without changing AArch64 emission yet.

## Watchouts

Keep the Step 2 payload scoped to source-selection completeness. Do not move
preservation source authority wholesale into this plan; that is reserved for
idea `06`.

Do not repair missing selection facts with AArch64 target-local fallback
rederivation or named-test shortcuts. The current callee-saved path already
falls back through `find_prior_preserved_value_for_call_argument`; the next
slice should make that fallback unnecessary for explicit callee-saved
`PriorPreservation` selections.

## Proof

No build or test was run. Proof was not applicable because this was an
audit-only packet and no implementation changed. No `test_after.log` was
created.
