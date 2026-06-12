Status: Active
Source Idea Path: ideas/open/213_route6_call_source_consumer.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Name the Consumer and Baseline Prepared Behavior

# Current Packet

## Just Finished

Step 1 selected one exact result-source consumer for idea 213:
`aarch64::codegen::record_call_result_source_register(...)` primary
call-result source-register publication, specifically the non-`result_lanes_only`
branch that reads `PreparedCallPlan::result` through
`prepare::find_prepared_call_result_late_publication(...)` and records the
call result value home in `BlockScalarLoweringState`.

Prepared baseline behavior:
- Prepared source data read: `call_plan.result` fields
  `destination_value_id`, `source_register_name`, `source_register_bank`,
  `source_register_placement`, `source_contiguous_width`, and
  `source_occupied_register_names`; value home comes from
  `context.function.value_home_lookups`/`value_locations`.
- Current consumer behavior: if prepared late publication says
  `source_register_publication_available`, the consumer converts the prepared
  ABI source register and records the destination value's emitted scalar
  register. It then separately records result-lane bindings from prepared
  after-call move-bundle ABI bindings.
- Fallback behavior today: when the prepared late-publication fact is absent,
  incomplete, has unparsable/mismatched register metadata, lacks a value home,
  or the BIR block/call is absent, the consumer returns without publishing a
  new scalar register rather than rederiving ABI source state.

Route 6 evidence source for later implementation:
- Use `bir::route6_find_call_result_source(...)` over a
  `bir::Route6CallUseSourceIndex`, keyed by block, call instruction index,
  callee, and the selected BIR result value.
- Agreement may validate only the same result value identity as the prepared
  destination value home. Route 6 must not supply or override source register,
  register bank/view, destination home, lane binding, after-call moves, stack
  stores, wrapper kind, aggregate transport, final call records, or emitted
  output.

Baseline/fallback/evidence matrix:
- Positive coverage exists at the helper/oracle layer:
  `backend_prepared_lookup_helper_test` covers `route6_call_result_source_record`
  and `route6_find_call_result_source` exposing result value provenance without
  ABI placement.
- Existing prepared consumer coverage:
  `backend_aarch64_call_boundary_owner_test` covers scalar/HFA/F128 call-result
  publication from prepared ABI source registers; `backend_aarch64_instruction_dispatch`
  covers direct call-result register moves, HFA result-lane moves, HFA stores
  consuming ABI result lanes, and after-call result publication ordering.
- Existing prepared printer surface:
  `backend_prepared_printer` prints prepared result source placement/register
  and late-publication flags from prepared call plans.
- Existing x86/wrapper surface:
  x86 `ConsumedPlans` carries Route 6 indexes, but x86 call-result consumers
  still use prepared result plans; `backend_x86_route_debug` is the byte-stable
  route-debug compatibility surface when enabled.
- Absent/invalid/duplicate/mismatch gaps for this selected consumer:
  no AArch64 consumer-level test currently proves absent Route 6 result facts,
  wrong-call/missing-call records, route/prepared result-value mismatch, or
  duplicate/conflicting Route 6 result evidence preserve the prepared
  source-register publication behavior. Helper/oracle coverage distinguishes
  missing result, wrong call, missing call, no-match lane, and duplicate lane,
  but not this consumer's prepared fallback.
- Fallback dimensions that must remain prepared/target-owned:
  ABI register placement and conversion, result value home lookup, result-lane
  bindings, after-call move bundles, stack-result stores, HFA/F128 carriers,
  aggregate transport, wrapper kind, final call records, printer strings, x86
  route-debug strings, and emitted assembly.

## Suggested Next

Step 2 should add an agreement-gated Route 6 result-source reader for only
`record_call_result_source_register(...)` primary result source-register
publication. The next packet should build a local Route 6 call-use source index
or pass one into the consumer, accept Route 6 only when
`route6_find_call_result_source(...)` matches the prepared destination value
identity, and fall back to the current prepared late-publication behavior for
absent, invalid, duplicate/conflicting, and mismatched Route 6 facts.

## Watchouts

- Keep the implementation to the selected result-source consumer; do not also
  migrate result-lane binding, after-call moves, result-store retargeting,
  wrappers, x86 result consumers, or prepared printer/debug rows.
- Route 6 result facts identify the BIR result value only. Any change that lets
  Route 6 choose ABI registers, register banks/views, destination homes,
  stack stores, aggregate lanes, wrapper behavior, or final call records is
  route drift.
- The positive test needs to prove route/prepared agreement is actually read,
  but the observable ABI/register output should remain byte-stable against
  the prepared baseline. Negative cases should mutate or remove Route 6 facts
  while preserving prepared call-plan fallback.

## Proof

Analysis-only packet; no build required and no `test_after.log` was produced.

Recommended narrow proof command for the later implementation packet:

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test backend_aarch64_call_boundary_owner_test backend_prepared_printer_test backend_x86_route_debug_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepared_printer|backend_x86_route_debug)$' --output-on-failure | tee test_after.log
```

If x86 backend tests are disabled in the local build, keep the same source
subset without `backend_x86_route_debug_test`/`backend_x86_route_debug` and
record that x86 route-debug byte-stability remains unproven for supervisor
follow-up.
