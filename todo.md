Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Consume Prepared I128 Runtime Helper Boundaries

# Current Packet

## Just Finished

Lifecycle handoff completed: idea 248 is closed for the supported i128 div/rem
prepared runtime-helper authority prerequisite, and idea 236 is active again.

Prepared helper authority now available for Step 6:

- i128 `SDiv`, `UDiv`, `SRem`, and `URem` source-operation mapping to helper
  kind and callee
- structural helper callees: `__divti3`, `__udivti3`, `__modti3`, and
  `__umodti3`
- low/high argument lane bindings and direct low/high result lane bindings
  from `PreparedI128Carrier`
- helper-specific call-boundary, caller-saved clobber, resource, ABI, and GPR
  argument/result bank facts

## Suggested Next

Execute Step 6 from `plan.md`: consume the prepared div/rem helper records
into selected AArch64 helper-boundary records, preserving fail-closed
diagnostics for float/i128 conversions and memory-return helper families.

## Watchouts

- Do not synthesize helper calls from `BinaryInst` or `CastInst` opcodes in
  AArch64 dispatch.
- Do not hard-code helper callees, fixed `x0`/`x1` lane ownership, register
  adjacency, or clobber policy inside target lowering.
- Float/i128 conversion helper mapping remains deferred.
- Memory-return helper families remain deferred unless a separate
  prepared/shared authority route supplies destination identity, storage
  extent, alignment, slot, and offset ownership.
- Step 7 printer work should wait until Step 6 selected helper-boundary records
  are complete.

## Proof

Lifecycle-only close and reactivation. No build or test proof was required,
and proof logs were not modified.
