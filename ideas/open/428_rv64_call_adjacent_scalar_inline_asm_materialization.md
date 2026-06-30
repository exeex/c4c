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

## Lifecycle Disposition

The active runbook for this idea was exhausted on 2026-06-30 and is parked,
not closed. The completed packets validated coherent scalar RV64 object
lowering for prepared GPR call-result publication, existing frame-slot scalar
value call-argument support, and complete-carrier scalar `.insn r` / `.insn d`
inline-asm materialization.

Remaining representative rows still stop at `unsupported_instruction_fragment`
because the prepared inline-asm carrier facts are incomplete or outside scalar
GPR input/output scope:

- `pr38533`: repeated `missing_operand0_home` for tied `=r,0` inline asm.
- `pr45695` and `pr49279`: `tied_input_output_home_mismatch`.
- `pr40657`: unsupported `=*m` memory constraint.
- `pr56982`: clobber-only `~{memory}` carrier with no scalar publication.

Follow-up source idea:
`ideas/open/432_prepared_inline_asm_operand_home_carriers.md`.
Do not reactivate this RV64 object-lowering plan until the prepared
producer/carrier blocker rows have a coherent carrier contract or are
explicitly excluded by a separate lifecycle decision.

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
