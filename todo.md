Status: Active
Source Idea Path: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Close the Proof Packet Notes

# Current Packet

## Just Finished

Completed Step 4 by recording closure-ready proof packet notes for the bounded
RISC-V same-consumer prepared-only `LoadLocal` row.

Reduced idea 265's prepared-only PO-family blocker only for the selected
RISC-V dynamic stack-source row: a normal, internally coherent
`PreparedFunctionLookups::memory_accesses` entry is no longer treated as
semantic authority by the same backend consumer when matching Route 3 / Route 5
memory-source authority is absent or non-agreeing. The accepted row proves the
consumer fails closed with `UnsupportedSourceHome` while the positive agreement
path remains byte-stable at `lw a1, 12(s2)`.

This reduces the idea 265 closure-note gap that said PO rows remained blocked
because no proof showed a still-public `memory_accesses` consumer rejecting a
self-consistent prepared-only row at the same Route 3 / Route 5 authority
boundary while preserving helper/status/fallback/debug/printer/wrapper/output
behavior. The reduction is narrow: it covers one RISC-V same-consumer
`LoadLocal` row and its adjacent helper/oracle/status/fallback/debug/output
contracts.

Remaining `memory_accesses` blockers:

- Other prepared-only PO rows and public consumers are not exhausted.
- X86 same-consumer prepared-only parity is not proven by this packet.
- Stale-publication rows remain blocked across producer block, instruction
  index, source value, base kind, wrong edge, duplicate, and obsolete Route 5
  owner cases.
- Byte-offset drift rows remain blocked for the full synthetic prepared offset
  matrix, public lookup consumer matrix, target operand surface, and
  wrapper/exact-output contracts.
- Cross-publication mismatch rows remain blocked for internally consistent but
  wrong prepared owners, duplicate/conflicting Route 5 memory-source rows,
  wrong-edge/wrong-destination memory-source rows, and cross-target
  status/fallback/output matrices.
- `PreparedFunctionLookups::memory_accesses`, `PreparedFunctionLookups`, and
  `PreparedBirModule` remain public compatibility surfaces; this packet does
  not authorize demotion, deletion, privatization, wrapper migration, accessor
  hiding, or whole-API retirement.

## Suggested Next

Supervisor should review the completed runbook and, if satisfied with the
existing focused and full-suite evidence, delegate plan-owner lifecycle closure
review for `ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md`.

## Watchouts

- Closure recommendation: close this runbook as a bounded proof packet for the
  RISC-V same-consumer prepared-only `LoadLocal` row, not as whole-API demotion
  readiness.
- The accepted baseline evidence is already full-suite green, but if the
  supervisor wants fresh closure proof after any additional dirty changes, run
  the matching regression guard or a broader CTest pass before asking the
  plan-owner to close.
- Do not use this packet to weaken expected output, status names,
  helper/oracle rows, fallback behavior, route/debug output, prepared-printer
  output, wrapper output, exact target output, or baselines.

## Proof

No new build or CTest was required for this summary/status packet.

Existing focused evidence:

```bash
{ cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure; } > test_after.log 2>&1
```

Step 3 notes record this focused proof as passed, with 2/2 CTests green:
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication`. Current `test_before.log` also
contains the same focused 2/2 green CTest result.

Existing accepted full-suite evidence: `test_baseline.log` records
`Baseline Commit: b5783084f4f409cdc05372293a4028350ad98588`,
`Baseline Regex: <full-suite>`, and `100% tests passed, 0 tests failed out of
3428`.

This evidence is sufficient for closure-ready notes. The lifecycle close
decision remains with the supervisor and plan-owner.
