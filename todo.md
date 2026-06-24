Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve VRM Identity Through LIR/BIR Lowering

# Current Packet

## Just Finished

Step 3 completed the LIR carrier substep for `__c4c_builtin_vrm1`,
`__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and `__c4c_builtin_vrm8`.

Implemented details:
- Added dedicated `LirTypeKind::VrmRegister` plus structured `LirTypeRef`
  width metadata for the stable `c4c.vrm1`, `c4c.vrm2`, `c4c.vrm4`, and
  `c4c.vrm8` spellings.
- Taught HIR-to-LIR value type spelling to preserve bare VRM carriers as
  `c4c.vrmN` instead of scalar, pointer, aggregate, or GCC vector LLVM storage.
- Extended LIR verifier signature checks so VRM return and parameter mirrors
  must keep the dedicated kind and exact width.
- Added frontend LIR coverage for direct and typedef spellings across all four
  widths, including printer proof and verifier rejection for scalar, GCC vector,
  and wrong-width drift.

## Suggested Next

Delegate the BIR/prealloc substep of Step 3/4 to `c4c-executor`: carry the
structured LIR `c4c.vrmN` carrier into backend-facing BIR type/value metadata
and publish vector register class plus group width at the pre-regalloc
frontier.

## Watchouts

- ABI and storage semantics are still intentionally unsupported after this
  slice: ordinary function parameter/return ABI, globals, static storage,
  alloca-backed storage, loads/stores, address-taking, arrays/members,
  initializers, arithmetic, casts, comparisons, lane operations, and final
  register allocation remain future work.
- The Step 3 LIR proof uses function declarations as a type-ref/signature probe.
  Do not treat it as ordinary ABI, storage, or executable LLVM IR support.
- Backend BIR/prealloc surfaces still need the next packet before VRM carriers
  can reach prepared inline asm/register-class metadata.
- Do not reuse `is_vector`, `vector_lanes`, or `vector_bytes`; those remain GCC
  vector storage metadata.

## Proof

Ran delegated proof successfully:
`bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^frontend_lir_" >> test_after.log 2>&1'`.

Proof log: `test_after.log`.

Supervisor acceptance also ran:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
