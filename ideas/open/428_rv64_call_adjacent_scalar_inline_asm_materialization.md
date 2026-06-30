# RV64 Call-Adjacent Scalar And Inline-Asm Materialization

Status: Open
Type: Implementation idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Source Evidence: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
Owning Layer: RV64 call and inline-asm object lowering
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Materialize ordinary scalar fragments around calls and inline asm in RV64
object lowering without absorbing aggregate ABI or F128 helper-call work.

## Why This Exists

The regenerated RV64 gcc_torture scan classified 38
`unsupported_instruction_fragment` rows as call-adjacent scalar publication and
inline-asm materialization. Representative rows include `src/pr38533.c`,
`src/pr40657.c`, `src/pr45695.c`, `src/pr49279.c`, and `src/pr56982.c`;
`src/pr38533.c` contains a prepared scalar inline-asm call fragment.

## In Scope

- Publish scalar call results and scalar call arguments when prepared call
  facts and object homes are coherent.
- Materialize scalar inline-asm input/output fragments using the prepared call
  surface.
- Add focused tests for scalar call results, scalar call arguments, and scalar
  inline-asm fragments.
- Keep diagnostics fail-closed for missing call homes, unsupported constraints,
  or non-scalar call storage.

## Out Of Scope

- Aggregate `sret`/`byval` call-storage lowering.
- F128 helper calls, long-double ABI, or F128 runtime support.
- Reconstructing missing call metadata in RV64 from raw BIR or source shape.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Coherent scalar call-result and call-argument rows no longer stop at
  `unsupported_instruction_fragment`.
- Coherent scalar inline-asm fragments lower through a general scalar
  call-adjacent route.
- Aggregate ABI and F128 rows remain explicitly out of this route unless a
  separate idea owns them.
- The proof includes focused call/inline-asm tests and the
  supervisor-delegated regression subset.

## Reviewer Reject Signals

- Reject treating aggregate `sret` or `byval` rows as scalar call publication.
- Reject inline-asm handling keyed to representative filenames such as
  `pr38533.c` or to one exact constraint spelling without a semantic rule.
- Reject RV64 inference of missing call homes, argument metadata, return homes,
  or inline-asm operands.
- Reject expectation rewrites, unsupported downgrades, or pass/fail accounting
  changes as evidence of call-adjacent progress.
- Reject a route that pulls F128 helper-call behavior into ordinary scalar
  call materialization.
