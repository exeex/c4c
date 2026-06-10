Status: Active
Source Idea Path: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize Selected F64 Readback Failure

# Current Packet

## Just Finished

Lifecycle activation only: created the active runbook from
`ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md`.

## Suggested Next

Execute Step 1 (`Reproduce And Localize Selected F64 Readback Failure`): build
`backend_aarch64_instruction_dispatch_test`, run the monolithic dispatch CTest,
confirm the `expected selected f64 global readback to feed call ABI move`
failure, and identify the source-selection or call ABI routing owner before
implementation begins.

## Watchouts

- Keep this route limited to the ambient selected f64 global readback dispatch
  debt.
- Do not weaken, remove, downgrade, or mark unsupported the monolithic dispatch
  expectation.
- Do not use Route 5 join-source records to carry call ABI or layout policy.
- Do not special-case the exact test name, exact emitted assembly snippet, or a
  single named global instead of repairing source selection or call ABI move
  routing.

## Proof

Not run for lifecycle activation.
