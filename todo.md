Status: Active
Source Idea Path: ideas/open/111_lir_struct_decl_printer_authority_switch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Declaration Emission And Parity Paths

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Delegate Step 1 to inspect the LLVM struct declaration emission path,
`struct_decls` rendering, legacy `type_decls` population, and verifier parity
checks before implementation edits.

## Watchouts

- Keep `type_decls` populated as legacy proof/backcompat data.
- Leave verifier shadow/parity checks active.
- Do not broaden the switch to global, function, extern, call, backend, BIR,
  MIR, or layout lookup authority.
- Do not rewrite expectations or downgrade tests as proof.

## Proof

Lifecycle-only activation; no build or test proof required.
