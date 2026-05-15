Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add ABI Binding And Marshaling Authority

# Current Packet

## Just Finished

Executed Step 4 by adding prepared/shared ABI binding and marshal/unmarshal
facts for supported i128 float-conversion helper mappings.

FP-to-I128 helpers now expose scalar FP argument ABI binding in the target call
argument FP bank, direct low/high i128 result ABI bindings in the call-result
GPR bank, scalar-value-to-ABI argument marshaling, and ABI-result-to-carrier
unmarshaling. I128-to-FP helpers now expose low/high i128 source ABI argument
bindings in the call-argument GPR bank, scalar FP result ABI binding in the
call-result FP bank, carrier-lane-to-ABI argument marshaling, and
ABI-result-to-scalar-value unmarshaling.

Unsupported F128/binary128 conversion mappings remain explicitly deferred, and
memory-return-required helper ownership remains fail-closed because this packet
does not add destination slot/offset producer authority. The prepared printer
dumps the new scalar ABI bindings and scalar marshaling facts beside existing
lane bindings without touching AArch64 selected-record or terminal printer
consumption.

## Suggested Next

Execute Step 5 as the clobber/resource, live-preservation, and selected-call
ownership packet for supported i128 conversion helper families. Use the Step 4
ABI and marshaling facts to decide when a conversion helper boundary has enough
call policy to own a later selected terminal call, while keeping memory-return
and F128/binary128 families fail-closed.

Suggested focused proof for Step 5:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reopen direct-result div/rem helper-call printing unless a concrete
  regression is found.
- Conversion helpers now have source-operation/helper mapping records,
  lane/scalar ownership facts, ABI register-bank transitions, and
  marshal/unmarshal moves. Clobber/resource policy, live-preservation, and
  selected-call ownership remain incomplete by design.
- Do not treat the existing memory-return vocabulary as ownership. I128 helper
  memory-return support still lacks destination/slot/offset producer authority
  and helper ABI policy tying an sret/memory destination to the helper record.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Preserve existing supported div/rem helper-call behavior while adding
  deferred helper authority.

## Proof

Focused proof passed:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: 3/3 tests passed. `git diff --check` also passed.
