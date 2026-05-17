Status: Active
Source Idea Path: ideas/open/270_aarch64_prologue_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Current Prologue Boundary

# Current Packet

## Just Finished

Activation created the runbook for Step 1; no implementation packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the current AArch64 frame-entry,
callee-save, parameter-home, call, return, and module ownership, then decide
whether `prologue.{hpp,cpp}` should own moved behavior, a deferred marker, a
narrow frame-record API, or a no-op boundary.

## Watchouts

Do not rebuild historical frame layout, register allocation, parameter storage,
variadic save areas, or callee-save instruction selection from `prologue.md`.
Preserve ABI-visible behavior and emitted output.

## Proof

Lifecycle-only activation. No build or backend proof was required.
