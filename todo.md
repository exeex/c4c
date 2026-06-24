Status: Active
Source Idea Path: ideas/open/328_rv64_prepared_edge_publication_module_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The Missing Publication Move

# Current Packet

## Just Finished

No executor packet has run for this active plan yet.

## Suggested Next

Start Step 1 from `plan.md`: reproduce
`backend_riscv_prepared_edge_publication`, inspect the final emitted RV64
assembly, and localize where available edge-publication moves are skipped.

## Watchouts

- Do not special-case fixture names, value ids, registers, or the literal
  expected move string as the implementation mechanism.
- Do not rediscover edge moves by scanning BIR predecessor/successor blocks.
- Preserve fail-closed behavior for unsupported homes and missing shared lookup
  authority.
- Keep idea 327 out of scope.

## Proof

Pending. Executor should write focused build/test proof here after running the
delegated packet.
