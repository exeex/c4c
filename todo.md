# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split Focused Repair Owners

# Current Packet

## Just Finished

Step 4 split the focused residual semantic `lir_to_bir`
global/pointer/aggregate projection owner into
`ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md`.

The new owner is ready for a lifecycle switch before implementation. Its
focused scope covers global scalar-array GEPs `00176` and `00181`,
pointer-value/parameter GEPs `00182` and `00209`, global dynamic aggregate
member GEPs `00195` and `00205`, bootstrap/global aggregate semantics `00204`,
and `00216` as a pointer-parameter/flexible-array aggregate projection
boundary case that must be included only if evidence shows the same semantic
projection rule.

Kept separate from the new owner:

- AArch64 machine-printer residuals, including scalar opcodes, integer casts,
  non-encodable immediates, stack-slot store source scratch printing, and
  `00140` call-boundary move printing.
- Runtime nonzero and runtime mismatch buckets, which still need generated
  assembly or narrower probes before they can become semantic owners.
- The standalone `00220` timeout/hang case.

## Suggested Next

Suggested lifecycle switch: deactivate or park umbrella inventory idea 295 and
activate `ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md`
before any implementation edits. Keep the current `test_before.log` baseline
as the accepted broad backend-regex reference: 352 selected, 291 passed, 61
failed.

## Watchouts

- This is an umbrella inventory. Do not implement fixes before a focused owner
  is split and activated.
- Do not reopen ideas 285 through 297 from failing counts or recurring testcase
  names alone. Reopen needs generated-code, proof, or diagnostic evidence that
  contradicts a specific closure boundary.
- `00159`, `00164`, `00169`, and `00089` appear again in runtime buckets, but
  the current packet only inspected logs and closed artifacts. They are not
  reopen evidence yet.
- Treat residual global/pointer/aggregate GEP work and `00216`
  pointer-parameter/flexible-array aggregate projection as the focused idea 298
  source scope, but do not absorb machine-printer residuals, runtime buckets,
  or `00220` timeout into that owner without new evidence.
- The runtime nonzero and runtime mismatch groups need generated assembly or
  narrower probes before a future split should promote them to semantic
  owners.
- `00220` timed out after `5.01 sec` in `test_before.log`; keep it quarantined
  from broad runtime proof until a timeout-specific owner exists or the
  supervisor delegates bounded hang inspection.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files, or
  canonical logs during lifecycle activation.
- `test_before.log` is the accepted focused baseline. `test_after.log` is
  absent at activation, and `test_baseline.log` is a pre-existing ignored root
  log.

## Proof

Lifecycle-only Step 4 packet. Created
`ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md` from the
Step 3 classification and recorded the suggested switch target. No tests were
run and no canonical logs were modified.
