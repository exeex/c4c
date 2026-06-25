Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Scalar Vararg and `va_copy` Boundary Facts

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 of the active
runbook. No implementation packet has run yet.

## Suggested Next

Delegate Step 1 to an executor: audit the remaining scalar `va_arg`, `va_copy`,
and broad RV64 variadic function admission diagnostics, then record the exact
first implementation boundary and proof command here.

## Watchouts

Do not reopen the completed idea 361 target-hook milestone unless the audit
finds a real regression in its original missing-fact acceptance criteria. Treat
`src/20030914-2.c` and `src/920908-1.c` as representatives, not implementation
keys.

## Proof

Lifecycle-only activation; no code validation was run.
