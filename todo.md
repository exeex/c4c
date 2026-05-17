Status: Active
Source Idea Path: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Current Variadic Boundary

# Current Packet

## Just Finished

Activation created the runbook for Step 1; no implementation packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the current AArch64 variadic
call-boundary, direct extern wrapper, FPR argument count, variadic entry helper,
`va_start`, `va_arg`, `va_copy`, dispatch, instruction-record, and diagnostic
ownership, then decide whether `variadic.{hpp,cpp}` should own moved behavior,
an explicit deferred marker, narrow record builders, or a no-op boundary.

## Watchouts

Do not rebuild historical `va_list`, register-save-area, stack-overflow-area,
`va_start`, `va_arg`, or `va_copy` behavior from `variadic.md`. Preserve
ABI-visible behavior, diagnostics, emitted output, and unsupported contracts.

## Proof

Lifecycle-only activation. No build or backend proof was required.
