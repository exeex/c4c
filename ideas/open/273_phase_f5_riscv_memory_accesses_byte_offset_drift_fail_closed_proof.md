# 273 Phase F5 riscv memory_accesses byte-offset drift fail-closed proof

## Goal

Use the supported RISC-V dynamic stack-source `LoadLocal` memory-access
consumer path to prove one byte-offset drift fail-closed row for
`PreparedFunctionLookups::memory_accesses`.

This is a bounded proof packet. It must not demote, delete, privatize, wrap,
hide, or rename `memory_accesses`.

## Why This Exists

Closed idea
`ideas/closed/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md`
kept the byte-offset drift family blocked. It found:

- BO-1 has selected x86 guardrail proof for reachable selected `LoadLocal`
  offset mutation;
- BO-2 has RISC-V consumer proof that an embedded Route 3 offset mutation
  preserves prepared output while agreement booleans become false;
- BO-3 and BO-4 have helper-level offset mismatch proof;
- BO-5 has partial x86/riscv guardrail evidence.

Closed idea
`ideas/closed/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md`
then proved one RISC-V same-consumer prepared-only `LoadLocal` row and
confirmed the RISC-V consumer directly reads
`PreparedFunctionLookups::memory_accesses`.

The next useful packet is therefore one RISC-V same-consumer byte-offset drift
proof: the prepared memory row remains present, but the Route 3 memory/source
authority disagrees on byte offset and must not be accepted as semantic
agreement.

## Required First Step

Before editing code or tests, identify and record:

- the exact RISC-V backend consumer that reads
  `PreparedFunctionLookups::memory_accesses`;
- the concrete prepared memory row from the dynamic stack-source `LoadLocal`
  fixture;
- the matching positive Route 3 memory/source authority fact;
- the byte offset carried by the prepared row;
- the byte offset carried by the Route 3 fact in the drift row;
- the exact expected fail-closed status or agreement flag;
- the public compatibility output, especially the positive-path
  `lw a1, 12(s2)` output, that must remain stable.

If the byte-offset drift row cannot be expressed through supported fixture
construction without hand-built stale prepared state, close with a blocker note
and produce a narrower fixture/testability idea instead.

## In Scope

- One RISC-V same-consumer byte-offset drift row for the dynamic stack-source
  `LoadLocal` path.
- Fail-closed behavior when the prepared memory row and Route 3 memory/source
  fact disagree only on byte offset.
- Preservation of the positive agreement path and exact RISC-V output.
- Preservation of prepared lookup/status compatibility, helper/oracle names,
  fallback names, route/debug output, prepared-printer output, wrapper output,
  and baseline behavior.
- Recording whether this proof reduces the BO-family blocker from idea 265.

## Out Of Scope

- Prepared-only proof already covered by idea 272.
- Stale-publication or cross-publication mismatch proof unless it naturally
  appears as the minimal way to express the byte-offset drift row.
- X86 memory-access proof.
- Demotion, deletion, privatization, wrapper creation, accessor migration, or
  public API hiding for `memory_accesses`.
- Broad `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155
  retirement.
- Moving target-owned stack, storage, addressing, source-home, register,
  layout, wrapper, formatting, emission, instruction spelling, or exact output
  policy into BIR.
- Weakening expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.

## Acceptance Criteria

- The selected RISC-V backend consumer, prepared memory row, prepared byte
  offset, and Route 3 drift byte offset are named.
- The proof shows byte-offset drift fails closed and is not accepted as
  Route 3 semantic agreement.
- The positive agreement path remains stable, including `lw a1, 12(s2)`.
- Public prepared compatibility output remains observable and byte-stable.
- The closure note states exactly which BO-family blocker from idea 265 is
  reduced and which `memory_accesses` blockers remain.

## Suggested Proof Scope

Use a matching before/after proof around the RISC-V fixture and helper/oracle
surfaces touched by the packet. Candidate tests:

```bash
cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test
ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure
```

Adjust the build directory or target names only if the current repo setup
requires it, and record the exact command used in `todo.md`.

## Reviewer Reject Signals

- The proof does not reach the real RISC-V backend consumer that reads
  `PreparedFunctionLookups::memory_accesses`.
- The byte-offset drift row is hand-built stale state that normal prepared
  lookup construction would reject.
- The patch accepts mismatched byte offsets as Route 3 semantic agreement.
- The patch proves helper/oracle behavior only while claiming backend
  same-consumer coverage.
- The patch hides, demotes, deletes, privatizes, wraps, or renames
  `memory_accesses`.
- The patch weakens exact RISC-V output, status names, helper/oracle rows,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, or baselines.
- The patch claims whole `memory_accesses`, `PreparedFunctionLookups`, or
  `PreparedBirModule` demotion readiness from this one proof row.
