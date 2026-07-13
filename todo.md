# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Converge target layout and ordered preparation dependencies

## Just Finished

- Plan Step 7 is complete. Target-layout publication and all six preparation
  products are immutable, transactionally published, and keyed to the complete
  Canonical `PipelineStageStamp`, exact `TargetFingerprint`, layout/schema
  fingerprints, and ordered predecessor-product fingerprints.
- The dependency order is fixed as target layout, ABI, calls, variadic,
  address, inline-assembly target tables, runtime helpers, cumulative bundle,
  then register-constraint interpretation. Each product has one producer and
  declared immediate and later consumers.
- Target layout owns abstract categories/classes/groups/slots, aliases,
  capacities, ABI eligibility, and the private concrete-mapping domain.
  Preparation owns classification, vocabulary tables, and planning only; it
  keeps Canonical storage read-only and makes no general assignment decision.
- `regalloc/constraints` is the sole interpreter that parses, types, and binds
  `r`, `=r`, `VR`, `VRM2`, `VRM4`, `VRM8`, ties, early-clobbers, and clobbers
  to ordinary `InlineAsm` operands/results. Preparation publishes declarative
  vocabulary/eligibility tables and no instruction-specific bindings.

## Suggested Next

- Execute Plan Step 8 in order: converge generic pseudo lowering, the closed
  pseudo schema, the Pseudo verifier profile, and the optional target-pass
  gate. Bind the new immutable pseudo revision and all derived facts to the
  exact Step 7 products without letting pseudo lowering absorb allocation.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Preserve `C7 -> C8 -> C9`: inline-assembly target tables precede runtime
  helpers, the cumulative bundle publishes atomically, and only then may the
  constraint interpreter consume instruction descriptions and ordinals.
- Do not treat `VerifiedPreparationInput`, a target layout, any planner fact,
  or `VerifiedPreparationBundle` as `PreparedBir`; that name remains reserved
  for the later verified allocated revision.
- Step 8 must consume the exact Canonical stamp, target fingerprint, layout,
  cumulative preparation bundle, and `BoundConstraintSet`; compatible-looking
  or module-revision-only products are stale.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n '(preparation|inline.asm).*(parse|type|bind).*(constraint|=r|VRM)|constraint.*(parse|type|bind).*(preparation|inline.asm)|allocation home|physical register assignment|mutat.*Canonical' src/backend/bir/preparation/README.md src/backend/bir/preparation/inline_asm/README.md src/backend/bir/preparation/{abi,calls,variadic,address,runtime_helpers}/README.md src/backend/bir/target_layout/README.md && rg -n '(sole|only).*(constraint|interpreter)|parse|type|bind|revision|fingerprint|transaction' src/backend/bir/regalloc/constraints/README.md` — exit 0.
- ``git diff --check && rg -n '`E1`|`E2`|`E3`' src/backend/bir/LEGACY_COVERAGE.md && ! rg -n '`S23`|`S24`|`S25`' src/backend/bir/LEGACY_COVERAGE.md`` — exit 0; the legacy allocation rows now use the root-authoritative stage labels.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
