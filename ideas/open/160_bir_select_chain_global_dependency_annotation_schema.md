# 160 BIR select-chain and direct-global dependency annotation schema

## Goal

Prototype BIR annotations for select-chain source identity, direct global-load
dependency, and scalar materialization eligibility established by Phase A Route
2.

## Why This Exists

Phase A proved target-neutral BIR answers for select-chain source producer,
root value/name, root-is-select, root instruction index, direct `LoadGlobalInst`
dependency, scalar materialization eligibility, and combined identity lookup.
Phase B classified these as value and instruction annotation candidates with
function-level indexes only for cheap lookup.

## In Scope

- Define value annotations for select-root identity, root-is-select, scalar
  eligibility, and direct-global dependency summary.
- Define instruction annotations for root producer instruction/index and direct
  `LoadGlobalInst` dependency.
- Preserve prepared select-chain queries as migration oracles until BIR-backed
  answers match accepted positive and negative cases.
- Keep lookup keys target-neutral and based on stable BIR ids.

## Out Of Scope

- Target materialization cost, hazard decisions, register availability,
  publication routing policy, call ABI behavior, and final AArch64 move or
  branch choices.
- Whole `PreparedDirectGlobalSelectChainDependency`,
  `PreparedSelectChainDependencyQuery`, or
  `PreparedScalarSelectChainMaterialization` records as annotation schema.

## Acceptance Criteria

- Select-chain and direct-global dependency consumers can read the accepted
  semantic facts from BIR-owned value/instruction annotations.
- Existing prepared query answers remain available as oracle checks during
  migration.
- Negative cases remain explicit and do not collapse into missing-cache
  behavior.

## Reviewer Reject Signals

- Reject if scalar eligibility is implemented by importing materialization cost,
  hazard, register availability, publication routing, or target move policy.
- Reject if the implementation depends on AArch64-specific instruction spelling
  or branch/move choices to identify select-chain facts.
- Reject if only direct-global success cases are proven while no-dependency and
  non-select-root cases are unexamined or weakened.
- Reject expectation downgrades that mark select-chain or direct-global cases
  unsupported without explicit user approval.
- Reject helper-only reshuffling where prepared queries still own the semantic
  answer and BIR has no typed annotation payload.
