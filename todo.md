Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Resolve Scalar Dump Linkage Or Carry-Fact Model

# Current Packet

## Just Finished

Step 3 correction completed. The attempted
`prepared-select-chain-materializations` section was removed because the route
duplicated source-producer, direct-global, and scalar materialization lookup
logic in `prepared_printer.cpp`.

Retained Step 3 progress:

- `prepared-call-plans` argument lines now print carried call-argument
  direct-global dependency labels when
  `find_prepared_call_argument_direct_global_select_chain_dependency` returns a
  dependency:
  - `direct_global_select_chain=yes`
  - `direct_global_source=<value>`
  - `direct_global_root_is_select=yes|no`
  - `direct_global_root_inst=<index>`

Rejected route:

- Do not restore printer-local traversal for
  `make_prepared_edge_publication_source_producer_lookups`,
  `find_indexed_prepared_edge_publication_source_producer`, direct-global
  recursion, or scalar materialization lookup.
- Calling existing non-inline helpers directly from top-level
  `prepared_printer.cpp` is not currently linkable in printer-only backend
  fixtures that compile selected printer files without `prepared_lookups.cpp`
  and `publication_plans.cpp`.

Plan-owner repair rewrote the remaining runbook path:

- Step 4: Resolve Scalar Dump Linkage Or Carry-Fact Model
- Step 5: Implement Remaining Scalar And Source-Producer Dump Visibility
- Step 6: Add Focused Prepared-Printer Tests
- Step 7: Final Validation And Close Readiness

## Suggested Next

Execute Step 4. Determine a non-duplicative route for scalar select-chain
materialization dump visibility:

- include existing lookup/publication implementation objects in printer-only
  test/link contexts,
- expose a narrow adapter that reuses existing lookup authority without
  duplicating traversal logic, or
- carry scalar materialization facts in prepared-module data before the printer
  consumes them.

Keep the Step 3 call-argument direct-global dependency labels. Record the
chosen route and proof target here before moving to Step 5.

## Watchouts

- Source intent did not change; do not edit
  `ideas/open/108_prepared_select_chain_dump_contract_coverage.md` for this
  repair.
- A scalar select-chain section remains required for acceptance.
- Store-source visibility is not a shortcut unless a bounded carried fact
  already directly supports this contract.
- Do not restore printer-local source-producer/direct-global/scalar traversal
  in `prepared_printer.cpp`.
- Do not weaken prepared-printer expectations or mark supported dump paths
  unsupported.

## Proof

Lifecycle repair only. `git diff --check -- plan.md todo.md` passed.
