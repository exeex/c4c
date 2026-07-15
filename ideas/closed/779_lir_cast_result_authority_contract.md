# LIR Standalone Cast Result Authority Contract

Status: Closed — capability complete
Type: bounded verifier/IR authority prerequisite
Unblocks: `ideas/open/778_lir_logical_rhs_result_authority_publication.md`

## Goal

Make the LIR verifier and result-ownership model fail closed for standalone
`LirCastOp` result authority: missing, invalid, duplicate, and foreign result
IDs must be rejected without relying on display text.

## Why This Exists

778 can allocate a native ID at its logical RHS `zext` producer, but its
required malformed-result proof cannot be expressed against the current
contract. Result ownership collection sees only `result.value_id()`, while
`verify_cast_op_authority` accepts an absent cast result. That allows the
missing-result case through and prevents a complete fail-closed matrix for a
standalone cast. This is an IR/verifier prerequisite, not a logical-RHS
producer change.

## In Scope

- define the minimal native `LirCastOp.result` authority requirement for a
  standalone, result-producing cast;
- align verifier result ownership collection and cast verification so missing,
  invalid, duplicate, and foreign cast result IDs fail closed;
- add focused structural positive and malformed coverage for that standalone
  cast contract; and
- publish the exact contract and proof needed for 778 to retry its bounded
  logical RHS producer step.

## Out Of Scope

- `emit_logical`, logical RHS producer allocation, PHI result/incoming entries,
  PHI verification, predecessor/edge work, and generic logical migration;
- ternary, coerce, vaarg, calls, ordinary instruction families, or a generic
  expression-result API migration;
- Raw-BIR/importer, backend, target lowering, MIR, emission, result-name maps,
  side tables, synthetic values, display-text parsing, and testcase-specific
  branches.

## Acceptance Criteria

- A standalone result-producing `LirCastOp` has a valid owning
  current-function `LirValueId` before any rendered spelling is considered.
- The verifier rejects missing, invalid, duplicate, and foreign standalone cast
  result authority through the native ownership contract.
- Focused coverage proves the positive path and every malformed case without
  PHI fixtures or logical-family behavior.
- The accepted handoff names the verifier/IR contract, focused proof, and the
  exact 778 return point; it does not claim logical-RHS or PHI progress.

## Reviewer Reject Signals

- Reject a logical RHS producer edit, PHI carrier/verifier change, or generic
  expression-result migration claimed as progress for this standalone cast
  contract.
- Reject accepting absent cast result authority, weakening malformed
  expectations, or changing a supported case to unsupported to make tests
  pass.
- Reject text-derived IDs, result-name maps, side tables, synthetic values, or
  named-testcase branches in place of native ownership verification.
- Reject Raw-BIR/importer, backend, target lowering, MIR, emission, or unrelated
  operation-family rewrites as part of this idea.

## Closure Record

Disposition: capability complete.

Accepted verifier/IR contract: selected standalone result-producing
`LirCastOp` instances opt into native result authority. The verifier requires a
present, valid, current-function-owned `LirValueId` and rejects missing,
invalid, same-function duplicate, and cross-function foreign result IDs. The
ownership index records instruction-result provenance across functions; it does
not use rendered text, result-name maps, side tables, PHI, logical lowering, or
generic expression APIs.

Accepted implementation and coverage: `5a9888938` selects standalone casts and
rejects missing result authority; `1e48ea6ab` adds cross-function result
ownership; `29f4adb6b` adds focused positive, missing, invalid, duplicate, and
foreign-ID coverage. Accepted focused proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
The supervisor also accepted the full-suite baseline candidate after
`1e48ea6ab`.

Successor resumption: `ideas/open/778_lir_logical_rhs_result_authority_publication.md`
resumes at Step 1, `Publish the logical RHS conversion result`, as a clean
producer reattempt. Its prior local `binary.cpp` diff remains unaccepted;
restart from the clean producer change, then perform its Step 2 against this
accepted contract. PHI result/incoming and generic logical work remain
excluded.
