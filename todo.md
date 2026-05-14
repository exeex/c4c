Status: Active
Source Idea Path: ideas/open/232_aarch64_variadic_function_entry_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Variadic Entry Inputs And Existing Prepared Boundaries

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run yet.

## Suggested Next

Start Step 1, Inspect Variadic Entry Inputs And Existing Prepared Boundaries.

## Watchouts

- Keep call-boundary variadic metadata separate from full variadic function
  entry carriers.
- Do not infer `va_list` layout, register-save areas, named GP/FP counts,
  negative offsets, overflow-area bases, or helper scratch policy inside
  AArch64 target codegen.
- Reject expectation downgrades, unsupported-path weakening, named-case
  shortcuts, and text-only printer patches as progress.

## Proof

Lifecycle-only activation; no build or test proof required.
