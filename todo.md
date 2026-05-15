Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Summarize

# Current Packet

## Just Finished

Executed Step 6 by running the delegated full-suite proof and summarizing the
prepared/shared deferred i128 helper authority completed by Steps 2-5.

Supported deferred i128 conversion helpers now have structured prepared/shared
authority for source-operation mapping, helper family/kind, callee identity,
direct result/argument lane ownership, ABI/register-bank bindings,
marshal/unmarshal moves, runtime-helper resources, caller-saved clobber policy,
live-preservation evaluation, and selected-call ownership. The supported set is
the direct-result F32/F64 conversion family:

- FP-to-I128: `__fixsfti`, `__fixdfti`, `__fixunssfti`, `__fixunsdfti`.
- I128-to-FP: `__floattisf`, `__floattidf`, `__floatuntisf`,
  `__floatuntidf`.

Remaining unsupported shapes are explicit blockers, not target-local lowering
gaps: F128/binary128 conversion mappings remain deferred, and i128 helper
memory-return ownership remains fail-closed because destination identity,
storage extent/alignment, slot, offset, and producer authority are not complete
for helper-selected memory results.

## Suggested Next

Ask the supervisor to route this exhausted runbook to plan-owner
close/deactivate review. If accepted, the next separate initiative should be a
selected AArch64 consumer route that consumes the prepared helper authority
without synthesizing helper ABI facts locally.

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

Full-suite proof passed:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Result: 3167/3167 tests passed. `test_after.log` is the preserved proof log.
