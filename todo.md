Status: Active
Source Idea Path: ideas/open/364_rv64_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared `va_start` Helper Boundary

# Current Packet

## Just Finished

Activation created this executor-compatible scratchpad for `plan.md` Step 1.
No implementation packet has run yet.

## Suggested Next

Start Step 1 by auditing the prepared `va_start` helper facts and the RV64
object consumer that currently reports `unsupported_variadic_helper_lowering`.
Record the first implementation boundary and narrow proof command here before
changing code.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen scalar `va_arg`, `va_copy`, external variadic call lowering, or
  general parameter-home expansion as part of this packet.
- Progress must consume prepared helper facts or emit precise diagnostics; a
  helper rename or expectation rewrite is not enough.

## Proof

No proof run for activation-only lifecycle work.
