Status: Active
Source Idea Path: ideas/open/279_phase_f5_x86_memory_accesses_public_field_parity_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove Or Block The x86 Public-Field Row

# Current Packet

## Just Finished

Completed Step 2 by recording bounded non-applicability for the x86
`memory_accesses` public-field parity row. No x86 parity proof is claimed:
Step 1 found no real x86 backend consumer boundary that reads
`PreparedFunctionLookups::memory_accesses`, so there is no current path through
which a public-field row can be proven or failed closed.

The absent boundary is specifically an x86 backend consumer of
`PreparedFunctionLookups::memory_accesses`. Existing x86 source-memory coverage
is real backend coverage, but it flows through `edge_publications`,
`PreparedEdgePublication::source_memory_access`, and
`PreparedAddressingFunction` / `find_prepared_memory_access(addressing, ...)`
instead of the public `memory_accesses` lookup field.

## Suggested Next

Return to the supervisor for route selection. The smallest prerequisite for any
future x86 public-field parity proof is to add or expose a real x86 backend
consumer boundary that reads `PreparedFunctionLookups::memory_accesses`, then
prove one bounded public row through that path.

## Watchouts

Do not claim x86 parity from the existing x86 Route 3 / Route 5 source-memory
agreement tests: they are real backend consumers, but they consume
`edge_publications`, `PreparedEdgePublication::source_memory_access`, and
`PreparedAddressingFunction`, not the public `memory_accesses` lookup field.
Do not use helper-only lookup tests, prepared-printer output, wrappers,
copied-publication paths, or renamed helpers as substitutes for the missing
boundary. This step is a scoped block/non-applicability record, not an x86
capability repair.

## Proof

Todo-only packet. No build or tests were run, and `test_before.log` /
`test_after.log` were not touched.
