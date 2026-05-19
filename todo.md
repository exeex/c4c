Status: Active
Source Idea Path: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and classify focused cast diagnostics

# Current Packet

## Just Finished

Lifecycle switched from umbrella idea 295 to focused idea 300. No
implementation work has started under this active plan.

## Suggested Next

Execute `plan.md` Step 1: reproduce or inspect the scalar-cast diagnostics for
`00035`, `00105`, `00126`, `00134`, `00135`, `00151`, and `00208`, then record
each focused case by cast form and likely implementation surface.

## Watchouts

- Keep the owner limited to scalar-cast machine-printer forms.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not pull symbol-store value printing, other machine-printer/frontend,
  runtime nonzero, runtime mismatch, or timeout buckets into this owner without
  generated-code or diagnostic evidence.
- Do not reopen closed owners 285 through 299 from failure counts alone.

## Proof

Lifecycle-only switch. No build or runtime proof was required or run.
