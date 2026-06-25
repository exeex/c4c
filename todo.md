Status: Active
Source Idea Path: ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Destination Address Producer Boundary

# Current Packet

## Just Finished

Activation created this executor-compatible scratchpad for `plan.md` Step 1.
No implementation packet has run yet.

## Suggested Next

Start Step 1 by auditing the prepared variadic-helper destination `va_list`
address facts, the available value-home or GPR-home producer state, and the
RV64 object consumer path that currently reports the prepared-GPR-home
diagnostic. Record the first implementation boundary and narrow proof command
here before changing code.

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-365 overflow-area base-state production.
- Do not broaden into general parameter-home expansion, external variadic call
  lowering, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- Progress must publish or prove an explicit prepared destination-address
  contract; a diagnostic rename, expectation rewrite, or object-emission guess
  is not enough.

## Proof

No proof run for activation-only lifecycle work.
