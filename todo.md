Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Print I128 Runtime Helper Boundary Calls

# Current Packet

## Just Finished

Lifecycle handoff completed: idea 249 is closed for supported direct-result
i128 div/rem helper marshaling and ABI register-binding authority, and idea
236 is active again.

Prepared/helper facts now available for Step 7:

- structured ABI argument and direct-result bindings for low/high lanes
- marshal/unmarshal facts connecting `PreparedI128Carrier` lanes to helper ABI
  argument/result bindings
- helper resource and clobber policy
- live-preservation and `PreparedCallPreservedValue` facts for values live
  across supported div/rem helper boundaries
- selected-call ownership that becomes true only when callee identity,
  resources, clobbers, ABI bindings, marshaling/unmarshaling, and live
  preservation are structurally complete

## Suggested Next

Execute Step 7 from `plan.md`: print supported direct-result i128 div/rem
helper-boundary calls from structured selected records and prepared
marshaling/ABI facts.

## Watchouts

- Do not recover operands from fixed `x0..x5` conventions, register adjacency,
  rendered names, opcodes, or helper callee strings.
- Keep incomplete live-preservation states fail-closed.
- Keep float/i128 conversion helpers and memory-return helper families
  deferred.
- `machine_printer.cpp` may only stop rejecting `I128RuntimeHelperBoundaryRecord`
  terminal output when it emits moves and `bl` from structured record fields.

## Proof

Lifecycle-only close and reactivation. No build or test proof was required,
and proof logs were not modified.
