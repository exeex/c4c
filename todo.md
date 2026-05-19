Status: Active
Source Idea Path: ideas/open/322_aarch64_va_start_destination_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Va Start Destination Publication

# Current Packet

## Just Finished

Lifecycle activated idea 322 after idea 321 closed. Ready to start Step 1:
localize AArch64 `va_start` destination address publication.

## Suggested Next

Trace the generated `myprintf` `va_start` stores back through
`homes.destination_va_list`, `make_variadic_va_start_record()`, and
`print_va_start_lowering_lines()` to identify where the writable local
`va_list` address should be materialized.

## Watchouts

- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- The external representative now reaches runtime, so the prior raw helper
  text assembler blocker is gone.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not reopen aggregate `va_arg` helper lowering; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `x21`, one local
  variable, one frame slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, and 321.
- Keep global initializer emission and later runtime mismatches separate unless
  they become the first bad fact after `va_start` destination publication is
  repaired.

## Proof

No Step 1 proof has run yet for idea 322. Use the focused 11-test subset from
the idea 321 closure as the initial proof scope unless the supervisor selects a
different subset.
