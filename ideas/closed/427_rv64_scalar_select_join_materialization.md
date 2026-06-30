# RV64 Scalar Select And Join Materialization

Status: Closed
Type: Implementation idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Source Evidence: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
Owning Layer: RV64 object lowering
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Add semantic RV64 object-lowering support for ordinary scalar select and
join-transfer fragments that are already represented in prepared BIR.

## Why This Exists

The regenerated RV64 gcc_torture scan classified 54
`unsupported_instruction_fragment` rows as select/join materialization, the
largest ordinary non-F128 owner bucket. Representative rows include
`src/pr43236.c`, `src/pr51933.c`, `src/pr68328.c`, `src/pr82954.c`, and
`src/pr84524.c`; `src/pr43236.c` exposes a prepared scalar `bir.select` row.

## In Scope

- Lower prepared scalar `bir.select` and related join-transfer fragments
  through RV64 object emission.
- Cover ordinary integer and pointer-sized scalar value merges when branch and
  value facts are coherent.
- Add focused tests that prove semantic select/join lowering across more than
  one representative shape.
- Preserve fail-closed diagnostics for select rows whose prepared control or
  value facts are incomplete.

## Out Of Scope

- F128 or long-double select handling.
- Inventing branch facts, dominance facts, value homes, or producer metadata in
  RV64 lowering.
- Aggregate `sret`/`byval`, call publication, inline asm, or pointer
  provenance repair.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Coherent prepared scalar select/join rows lower without
  `unsupported_instruction_fragment`.
- Nearby select forms use the same semantic lowering rule rather than
  file-specific paths.
- Incomplete prepared select facts still fail closed with a clear diagnostic.
- The proof includes focused select/join tests and the supervisor-delegated
  regression subset.

## Reviewer Reject Signals

- Reject filename-shaped handling for `pr43236.c`, `pr51933.c`, `pr68328.c`,
  or any other single gcc_torture row.
- Reject lowering that fabricates branch conditions, value joins, live ranges,
  or object homes not present in prepared facts.
- Reject expectation rewrites, allowlist edits, unsupported downgrades, or
  pass/fail accounting changes claimed as select/join progress.
- Reject mixing this route with call-adjacent publication, aggregate ABI, F128,
  or pointer-provenance repair.
- Reject helper renames or abstraction moves that leave prepared scalar
  `bir.select` rows failing with the same unsupported instruction-fragment
  mode.

## Closure Note

Closed after the active runbook implemented semantic RV64 object emission for
ordinary small-integer prepared scalar selects and stack-homed published
join-transfer select carriers using existing prepared facts. Focused backend
coverage now exercises multiple select/join shapes, including ordinary I8/I16
select materialization, normalized compare selects, join-transfer selects,
published-carrier handling, edge-compare-source cases, dependent edge sources,
and fail-closed ambiguous publication diagnostics.

Residual full gcc_torture representative failures are intentionally outside
this idea: current blockers are call-adjacent scalar/inline asm, frame-slot
address/value publication, global memory, or stack/publication follow-ups. No
expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
accounting changes were used as progress. The backend close gate passed with
matching `test_before.log` and `test_after.log` for
`{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; }`,
both reporting 327/327 passing tests.
