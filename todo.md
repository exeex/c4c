Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Variadic Consumption Boundary

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run for Step 1 yet.

## Suggested Next

Executor should start Step 1, Inspect Prepared Variadic Consumption Boundary,
by tracing `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` from
prepared variadic-entry facts into AArch64 helper dispatch and recording the
exact consumption blockers or the first viable helper path.

## Watchouts

- Do not infer AAPCS64 variadic layout, register-save offsets, overflow-area
  offsets, named argument counts, or scratch policy inside AArch64 codegen.
- Do not weaken existing fail-closed diagnostics or backend expectations to
  claim support.
- Treat prepared dump presence as necessary context, not proof of selected
  machine-node consumption.

## Proof

Not run. Activation is lifecycle-only.
