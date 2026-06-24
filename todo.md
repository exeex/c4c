Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve VRM Identity Through BIR Lowering And Publish Pre-Regalloc Classification Metadata

# Current Packet

## Just Finished

Step 3/4 completed the BIR/prealloc carrier substep for
`__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and
`__c4c_builtin_vrm8`.

Implemented details:
- Added dedicated BIR `TypeKind::Vrm1`, `Vrm2`, `Vrm4`, and `Vrm8` carrier
  kinds with stable `c4c.vrmN` rendering.
- Taught LIR-to-BIR scalar/function-pointer type lowering and signature
  parameter lowering to preserve `c4c.vrmN` instead of decaying to scalar,
  pointer, aggregate, or GCC vector storage.
- Kept ordinary call argument/return ABI classification and inline-asm integer
  immediate handling explicitly unsupported for VRM carriers.
- Taught prealloc regalloc classification to derive vector register class and
  group width directly from BIR VRM value type metadata.
- Gated inline asm vector register-group override publication so `VR`/`VRM2`/
  `VRM4` constraints reinforce only matching VRM value types; scalar `long`
  with `VRM2` remains a general width-1 value and rejects as incompatible.
- Added backend lowering, liveness/regalloc, and prepared-printer coverage for
  positive VRM carriers and scalar negative behavior.

## Suggested Next

Delegate Step 5 frontier validation to `c4c-executor`: prove the source-to-
regalloc-frontier chain together across frontend HIR/LIR, BIR lowering,
prepared inline asm carrier validation, scalar negative diagnostics, and the
supervisor-selected broader subset.

## Watchouts

- ABI and storage semantics remain intentionally narrow: ordinary ABI lowering,
  memory storage, address-taking, arrays/members, arithmetic, casts,
  comparisons, lane operations, final register allocation substitution, and
  object-byte emission remain future work.
- Manual `PreparedRegisterGroupOverride` fixtures can still exercise regalloc
  mechanics on scalar-shaped values; source inline asm metadata no longer uses
  constraint strings alone to create vector identity.
- Do not reuse `is_vector`, `vector_lanes`, or `vector_bytes`; those remain GCC
  vector storage metadata.

## Proof

Ran delegated proof successfully:
`bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^backend_" >> test_after.log 2>&1'`.

Proof log: `test_after.log`.

Supervisor acceptance also ran:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
