Status: Active
Source Idea Path: ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Build the Fail-Closed Proof Matrix

# Current Packet

## Just Finished

Completed `plan.md` Step 5, "Build the Fail-Closed Proof Matrix", as an
analysis-only matrix for the selected Route 3 `LoadLocal`
result/source-memory identity fact.

Selected fact: Route 3 `LoadLocal` result/source-memory identity agreement for
a prepared dynamic source-memory row. Agreement means a same-function,
same-block `Route3MemoryAccessRecord` for `LoadLocal` identifies the same
loaded result value and source-memory fields as
`PreparedEdgePublication::source_memory_access`.

Carried-forward target classifications:

- Step 3 x86 remains blocked, not non-applicable. x86 has prepared memory
  consumers in `src/backend/mir/x86/module/module.cpp`, but no consumer that
  reads the selected Route 3 `LoadLocal` memory record or MIR query facade and
  compares it with prepared `source_memory_access`.
- Step 4 RISC-V remains diagnostic-only. `route3_source_memory_agrees` and the
  compared Route 3/prepared source-memory fields are semantic evidence; `lw`
  text, register choices, source-home policy, fallback names, status strings,
  helper/oracle names, and addressing/storage output remain prepared
  compatibility or target policy.

Fail-closed proof matrix:

| Case | Prepared compatibility surface that stays observable | Required Route 3/BIR behavior | Scope | Existing evidence or test |
| --- | --- | --- | --- | --- |
| Missing prepared source-memory row | `PreparedEdgePublicationSourceMemoryAccessStatus::MissingPreparedMemoryAccess`, public prepared publication lookup, and RISC-V `UnsupportedSourceHome` fallback/status rows remain observable. | Do not synthesize Route 3 agreement. A missing prepared row rejects semantic agreement even if a nearby Route 3 memory record exists. | Common prepared gate with RISC-V evidence; x86 remains blocked. | `apply_source_memory_access_fact(...)` reports missing prepared access; `backend_riscv_prepared_edge_publication_test.cpp:1465-1508` distinguishes missing prepared rows and keeps output fail-closed. |
| Invalid or incomplete prepared row | `IncompletePreparedMemoryAccess`, prepared source-memory result/base/size/alignment completeness checks, and unsupported prepared output rows stay observable. | Reject agreement unless prepared result, base identity, size, alignment, address space, volatility, and offset are complete enough to compare. | Common prepared gate with RISC-V evidence; x86 remains blocked. | `apply_source_memory_access_fact(...)` rejects incomplete prepared rows; `backend_riscv_prepared_edge_publication_test.cpp:1465-1508` checks incomplete rows reject as `UnsupportedSourceHome`. |
| Duplicate or conflict row | Prepared helper/oracle behavior for unique publication and memory lookup selection, duplicate lookup status, and conflict diagnostics stay compatibility-owned. | Reject agreement when more than one candidate prepared publication, prepared memory access, or Route 3 memory record could claim the same source identity without a unique same-function/same-block `LoadLocal` match. | Common rule; target-specific observability through current prepared helper names/statuses. | Existing helper names include `find_unique_indexed_prepared_edge_publication(...)`, `find_prepared_memory_access(...)`, and `route3_find_memory_access_record(...)`; no adapter may weaken duplicate/conflict expectations into first-match behavior. |
| Route/prepared mismatch | Prepared source-memory row remains authoritative for fallback/output; RISC-V diagnostic bit remains observable as false. | Reject agreement when Route 3 node kind, loaded result kind/type/name, base kind/identity, address space, volatility, byte offset, size, or alignment differs from the prepared source-memory row. | Common semantic rule with RISC-V evidence; x86 remains blocked. | `route3_source_memory_agrees_with_prepared_publication(...)` performs the comparison; `backend_riscv_prepared_edge_publication_test.cpp:1748-1762` mutates the Route 3 byte offset to `16` and expects `route3_source_memory_agrees == false` while prepared output remains stable. |
| Unsupported source-memory shape | Prepared `UnsupportedSourceHome`, `UnsupportedDestinationHome`, and `UnsupportedPublication` rows and exact status/fallback strings remain observable. | Reject Route 3 ownership for shapes outside the selected `LoadLocal` dynamic source-memory fact, including materialization-required rows, non-i32 dynamic loads, volatile or non-default address-space rows, and unsupported source/destination homes. | Common rejection with target-specific fallback/status spelling. | `backend_riscv_prepared_edge_publication_test.cpp:1555-1585` keeps unsupported source-memory shapes fail-closed; Step 4 classified fallback/status names as non-semantic. |
| Prepared-only consumer row | x86 prepared frame-slot/global memory rendering and prepared lookup/status rows remain compatibility-owned. | Do not count prepared-only consumers as Route 3 proof. Without a Route 3 memory record or MIR query facade comparison, the adapter must remain blocked for x86. | Target-specific x86 blocker. | Step 3 found x86 prepared consumers such as `render_prepared_frame_slot_memory_operand(...)`, `render_prepared_local_slot_statement_memory_operand(...)`, and `render_prepared_same_module_global_memory_operand(...)`, but no Route 3 identity consumer. |
| Fallback path | Prepared fallback names and statuses such as `MissingSharedLookups`, `MissingPublication`, `UnsupportedSourceHome`, `MissingPreparedMemoryAccess`, `IncompletePreparedMemoryAccess`, `available`, `memory_source`, `no_match`, and `no_source` remain observable. | Missing, rejected, or unavailable Route 3 diagnostics must preserve prepared-backed fallback behavior and must not invent Route 3-owned output. | Common compatibility rule; target-specific strings remain owned by current backends. | Step 4 recorded RISC-V fallback preservation when Route 3 diagnostics are absent, mismatched, or incomplete; `backend_riscv_prepared_edge_publication_test.cpp:1766-1781` clears Route 3 source-memory identity and expects agreement false with prepared output stable. |
| Policy-sensitive memory/output row | x86 operand spelling and RISC-V instruction/register/addressing rows remain target-owned; prepared helper/oracle names remain compatibility-owned. | Route 3/BIR may only prove source-memory identity, not addressing legality, register allocation, frame/global placement, source-home policy, size spelling, final assembly, or helper/status naming. | Target-specific policy. | Step 3 classified x86 stack/global operand text, frame-slot/global span policy, and prepared drift errors as compatibility-owned; Step 4 classified RISC-V `lw` text, registers, source homes, large-offset handling, and status strings as non-semantic. |

Reviewer rejection rules derived from the matrix:

- Reject expectation weakening that hides prepared missing, incomplete,
  unsupported, duplicate, fallback, or policy-sensitive rows.
- Reject named-case shortcuts that only make the known RISC-V diagnostic row
  agree without preserving the common reject cases above.
- Reject any claim of adapter readiness while x86 lacks a Route 3
  source-memory identity consumer or explicit agreement bridge.
- Reject any implementation that makes Route 3/BIR own target output policy
  instead of only the selected source-memory identity.

## Suggested Next

Execute Step 6 by deciding adapter readiness for idea 250. The next packet
should conclude from the matrix that the selected identity has RISC-V
diagnostic evidence but x86 remains blocked, then recommend closure or a split
without implementing an adapter.

## Watchouts

- The matrix supports a blocker-map conclusion, not adapter readiness: x86 has
  no selected Route 3 source-memory identity consumer.
- RISC-V agreement is still diagnostic-only; prepared output and fallback
  behavior must stay stable on missing, mismatched, or incomplete Route 3 rows.
- Do not broaden Step 6 into generic memory parity, expectation rewrites,
  target output ownership, or implementation planning.
- If a follow-up idea is created, keep it narrowly about adding or proving an
  x86 agreement bridge for this selected fact.

## Proof

No build/test proof required by the delegated packet. Analysis-only validation
used the existing Step 3 and Step 4 `todo.md` evidence plus focused `rg`/`sed`
inspection. No `test_after.log` produced because proof was explicitly not
required.
