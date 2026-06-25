Status: Active
Source Idea Path: ideas/open/365_rv64_va_start_overflow_base_state_production.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Overflow Base-State Producer Boundary

# Current Packet

## Just Finished

Activation created this executor-compatible scratchpad for `plan.md` Step 1.
No implementation packet has run yet.

## Suggested Next

Start Step 1 by auditing the prepared variadic-entry producer, prepared
printer/debug output, and RV64 object consumer path that currently reports the
missing overflow-area initial base-state diagnostic. Record the first
implementation boundary and narrow proof command here before changing code.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-364 object-emission helper lowering except for narrow
  consumer adjustments required by a finalized producer contract.
- Do not broaden into external variadic call lowering, general parameter-home
  expansion, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- Progress must publish explicit prepared overflow-area base state; a
  diagnostic rename, expectation rewrite, or object-emission guess is not
  enough.

## Proof

No proof run for activation-only lifecycle work.
