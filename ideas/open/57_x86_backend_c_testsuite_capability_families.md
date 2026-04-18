# Prepared-BIR-To-X86 Route Decomposition Umbrella

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-18

## Intent

Idea 57 is the umbrella source note for removing x86 prepared-emitter matcher
growth and replacing it with explicit upstream contracts.

It is intentionally not the next execution target.

It exists to keep the decomposition coherent and to stop future plans from
silently collapsing several separate ownership problems back into one vague
"make x86 pass more tests" initiative.

## The Decomposition

The route is split into four concrete source ideas:

1. [58_bir_cfg_and_join_materialization_for_x86.md](/workspaces/c4c/ideas/open/58_bir_cfg_and_join_materialization_for_x86.md)
   Shared control-flow meaning: branches, joins, short-circuit flow, loop
   carries.
2. [60_prepared_value_location_consumption.md](/workspaces/c4c/ideas/open/60_prepared_value_location_consumption.md)
   Canonical value homes and move obligations: register homes, stack homes,
   join/call/return copy plans.
3. [61_stack_frame_and_addressing_consumption.md](/workspaces/c4c/ideas/open/61_stack_frame_and_addressing_consumption.md)
   Canonical frame and memory-address consumption: frame slots, stack-relative
   addressing, local/global/pointer provenance.
4. [59_generic_scalar_instruction_selection_for_x86.md](/workspaces/c4c/ideas/open/59_generic_scalar_instruction_selection_for_x86.md)
   Generic x86 scalar lowering over the contracts produced by 58, 60, and 61.

## Required Ordering

The expected dependency order is:

1. idea 58
2. idea 60
3. idea 61
4. idea 59

The exact execution slicing may interleave 60 and 61, but idea 59 should not
be activated before x86 has canonical value-home and memory-address inputs.

## Concrete Boundary Statement

The public x86 prepared route should eventually consume a handoff shaped more
like this:

```cpp
struct PreparedBirModule {
  bir::Module module;
  PreparedControlFlow control_flow;    // idea 58
  PreparedValueLocationMap locations;  // idea 60
  PreparedAddressingMap addressing;    // idea 61
  PreparedRegalloc regalloc;           // idea 60 producer data
  PreparedStackLayout stack_layout;    // idea 61 producer data
};
```

Then x86 scalar lowering should look like this:

```cpp
emit_prepared_function(function, prepared);
emit_prepared_block(block, function_ctx);
emit_prepared_inst(inst, block_ctx);
emit_prepared_terminator(term, block_ctx);
```

The emitter should no longer need to answer:

- "what does this CFG shape mean"
- "where does this value live"
- "what stack slot is this local"
- "what address form is this load/store really using"

Those answers must already be upstream-owned.

## What This Umbrella Does Not Own

This idea does not define exact struct fields by itself.
That detail belongs in 58, 59, 60, and 61.

This idea does not justify one more x86 `try_*` helper.

This idea does not define a runbook.

## Recommendation For Future Activation

Activate one of the leaf ideas, not this umbrella.

The best next activation target is usually idea 60 or idea 61 if the current
x86 emitter is still rebuilding value homes or frame addresses locally.
