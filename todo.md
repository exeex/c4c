# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Call-Boundary Record Surfaces

## Just Finished

Lifecycle activation created this audit runbook from
`ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md`.

## Suggested Next

Begin Step 1, "Inventory Call-Boundary Record Surfaces", by tracing
call-boundary move records from `calls.cpp` through `instruction.*` and
`machine_printer.*`.

## Watchouts

- This is audit-only; do not edit implementation files while executing the
  current plan.
- Do not claim printer cleanup by deleting validation or moving ABI-specific
  call-boundary construction into shared authority.
- Treat unclear duplication as `missing-evidence` until the record path and
  owner boundary are traced.

## Proof

Activation only. No build or backend test proof is required until execution
touches implementation files.
