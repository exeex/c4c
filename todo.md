Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The FP Binary Owner

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the 570 FP binary evidence, reproduce or
confirm the `src/20000605-1.c` object-route failure if needed, and record the
current FP binary owner plus object-emission implementation surface here before
making code changes.

## Watchouts

- Do not match `src/20000605-1.c`, `render_image_rgb_a`, `%t5`, or exact value
  names.
- Do not mix FP casts, truncation, comparisons, pointer arithmetic, select, or
  inline asm work into this plan unless a separate follow-up idea is created.
- Diagnostic-only edits are not capability progress unless paired with real
  semantic lowering or a narrower fail-closed unsupported path.

## Proof

No validation run for lifecycle-only activation.
