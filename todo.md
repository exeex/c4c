Status: Active
Source Idea Path: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Intrinsic Memory Admission Boundary

# Current Packet

## Just Finished

Activated Step 1 from `plan.md` for the runtime/intrinsic memory producer
admission idea.

## Suggested Next

Inspect the `memcpy` and `memset` representative rows, capture the current
semantic admission boundary, and record whether they share one producer repair
path.

## Watchouts

- Do not replace intrinsic rows with runtime call substitutions or
  target-specific named-case lowering.
- Do not weaken expectations, unsupported markers, allowlists, or semantic
  admission checks.
- If `memcpy` and `memset` do not share a producer boundary, request a
  lifecycle split before implementation.

## Proof

- Not run for activation-only lifecycle setup.
