Status: Active
Source Idea Path: ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close the Proof Packet Notes

# Current Packet

## Just Finished

Step 4: recorded closure-ready notes for the RISC-V byte-offset drift proof
packet.

This row reduces the BO-family byte-offset drift blocker from idea 265 for one
RISC-V same-consumer dynamic stack-source `LoadLocal` row. The prepared/public
`memory_accesses` row remains built by normal prepared lookup construction at
byte offset `12`, the agreeing Route 3/Route 5 path remains byte-stable at
`lw a1, 12(s2)`, and the drift row changes only Route 3 source-memory byte
offset authority to `16`. That mismatch reaches the real RISC-V consumer and
fails closed through `UnsupportedSourceHome`, empty instruction text,
`MemorySource`, `route5_edge_source_agrees=false`, and
`route3_source_memory_agrees=false`.

Remaining `memory_accesses` blockers are not cleared by this row:

- BO-family coverage still is not exhaustive for every synthetic prepared
  offset drift row, public lookup consumer, target operand surface, or
  wrapper/exact-output contract; this packet proves only the selected RISC-V
  same-consumer dynamic stack-source `LoadLocal` row.
- Prepared-only, stale-publication, and cross-publication mismatch families
  remain bounded proof-map blockers from idea 265 except where separately
  reduced by prior accepted proof packets such as idea 272.
- This packet does not authorize demotion, deletion, privatization, accessor
  wrapping, public API hiding, expectation weakening, or broad
  `PreparedFunctionLookups` / `PreparedBirModule` retirement.

The accepted validation evidence is the narrow RISC-V prepared edge
publication plus prepared lookup helper proof, regression-guarded against
`test_before.log`. After acceptance, `test_after.log` was rolled forward to
`test_before.log`. No broader validation is required for this todo-only
closure-note packet. Broader validation is recommended before final lifecycle
closure if the supervisor treats closure as a milestone decision.

## Suggested Next

Supervisor/plan-owner lifecycle closure review for the active runbook and
source idea. Do not schedule more executor work unless that review finds a
missing closure criterion.

## Watchouts

- This packet only records closure notes; it does not perform lifecycle
  closure, commit readiness review, or source-idea edits.
- Keep the closure claim scoped to one RISC-V same-consumer byte-offset drift
  row. Do not treat this as whole-API demotion readiness or as x86 parity.
- Do not weaken the public `memory_accesses` lookup requirement, positive
  `lw a1, 12(s2)` output, missing/incomplete source-memory fail-closed cases,
  status names, fallback behavior, route/debug output, prepared-printer output,
  wrapper output, or baseline-visible exact output.

## Proof

No new build/test was required for this todo-only closure-note packet. The
accepted proof result for the completed code/test slice was:

```sh
cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: passed `2/2` tests. The result was regression-guarded against
`test_before.log`; after acceptance, `test_after.log` was rolled forward to
`test_before.log`.
