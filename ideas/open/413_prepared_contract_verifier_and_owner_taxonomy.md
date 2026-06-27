# Prepared Contract Verifier And Owner Taxonomy

Status: Open
Type: Follow-up contract refactor idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`

## Goal

Introduce a prepared contract verifier and owner taxonomy that classifies
prepared facts before MIR/RV64/AArch64 consumers run.

## Why This Exists

Idea 412 confirmed that missing producer facts are currently discovered across
many local status enums and target rejection paths. Recent RV64 work repeatedly
had to split producer defects out of target buckets: I16 ABI homes, local
aggregate slot extents, variadic helper operand homes, global initializer
admission, and wide rematerialized immediates.

The compiler needs one prepared verifier vocabulary that can say whether a fact
family is coherent, producer-missing, producer-incoherent,
target-unsupported-but-coherent, or pre-prepared semantic failure.

## In Scope

- Add or stage a verifier/report pass after prepared publication and before
  target prepared consumers.
- Start with a small family set: value homes, call argument plans, memory
  accesses, helper operand homes, and publication facts.
- Reuse existing missing-fact/status language where possible.
- Make diagnostics precise enough to route work to BIR/prepared or target
  lowering.
- Add focused backend/prepared tests for verifier categories.

## Out Of Scope

- Rewriting all prepared structs in this slice.
- Implementing broad RV64 object lowering.
- Changing gcc_torture allowlists, runtime comparison, or unsupported markers.
- Treating RV64 gcc_torture pass count as the close gate.

## Acceptance Criteria

- A verifier/report surface exists and runs before target prepared consumers in
  the selected route.
- At least three selected fact families produce owner-classified diagnostics.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- RV64 gcc_torture representatives may be used as evidence, but temporary RV64
  gcc_torture pass-count regression is acceptable if default CTest is stable
  and the regression is a fail-closed contract classification.

## Reviewer Reject Signals

- Reject diagnostics-only churn if malformed prepared facts can still reach
  target consumers unchanged.
- Reject verifier categories that blur producer-missing facts with target
  unsupported coherent facts.
- Reject MIR/RV64 changes that infer ABI homes, aggregate layout, helper
  operand addresses, global initializer bytes, or BIR constant semantics.
- Reject broad BIR rewrites claimed as verifier work.
- Reject proof that only checks one named gcc_torture case without focused
  contract tests or default CTest stability.
