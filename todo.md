Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Clobber, Resource, Live-Preservation, And Selected-Call Facts

# Current Packet

## Just Finished

Executed Step 5 by adding clobber/resource, live-preservation, and
selected-call ownership facts for supported i128 float-conversion helper
families.

Supported FP-to-I128 and I128-to-FP helpers now publish runtime-helper call
resources, caller-saved clobber policy, live-preservation evaluation, and
selected-call ownership when callee identity, resource/clobber policy, ABI
bindings, marshaling/unmarshaling, and live preservation are complete.
Selected-call completeness now uses the conversion helper's scalar/pair shape
instead of the div/rem six-lane shape. F128/binary128 conversion mappings
remain explicitly deferred, and memory-return-required ownership remains
fail-closed because destination/slot/offset producer authority is still absent.

## Suggested Next

Execute Step 6 as the validation and summary packet. Confirm the prepared/shared
deferred helper authority is sufficient for supported conversion helpers,
summarize remaining unsupported memory-return and F128/binary128 shapes, and
ask the supervisor whether to close this prerequisite or activate a later
selected AArch64 consumer route.

Suggested focused proof for Step 6:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reopen direct-result div/rem helper-call printing unless a concrete
  regression is found.
- Conversion helpers now have source-operation/helper mapping records,
  lane/scalar ownership facts, ABI register-bank transitions,
  marshal/unmarshal moves, clobber/resource policy, live-preservation
  evaluation, and selected-call ownership facts.
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
