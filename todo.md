Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Frontier And Prepare Handoff

# Current Packet

## Just Finished

Step 5 validated the completed source-to-regalloc-frontier chain for
`__c4c_builtin_vrm1`, `__c4c_builtin_vrm2`, `__c4c_builtin_vrm4`, and
`__c4c_builtin_vrm8` without changing implementation files.

Validated surfaces:
- Parser/type and sema preserve dedicated VRM carrier identity for the four
  builtin spellings instead of treating them as GCC vectors.
- HIR and LIR dumps carry stable `c4c.vrmN` type refs through direct and
  typedef spelling paths.
- LIR-to-BIR lowering preserves `c4c.vrmN` as dedicated BIR value metadata.
- Prepared inline asm validation accepts matching VRM carrier values for vector
  register constraints and derives vector register class plus group width from
  value type metadata.
- Scalar negative behavior remains intact: scalar `long`/`I64` with grouped
  vector constraints does not acquire VRM identity from constraint text alone
  and rejects as an incompatible register class.
- The delegated full build and full CTest suite completed successfully.

## Suggested Next

Supervisor should review and decide whether this runbook is ready for
lifecycle close or replacement by the next source idea. The natural follow-up
initiative is final VRM allocation/substitution/object-byte work after the
frontier metadata is accepted.

## Watchouts

- This runbook intentionally stops at the regalloc frontier. Final allocation,
  physical substitution, prepared-to-machine propagation, and object-byte
  emission remain follow-up work.
- ABI and storage semantics remain intentionally narrow: ordinary call/return
  ABI lowering, memory storage, address-taking, arrays/members, arithmetic,
  casts, comparisons, and lane operations remain future work.
- Manual `PreparedRegisterGroupOverride` fixtures can still exercise regalloc
  mechanics on scalar-shaped values; source inline asm metadata no longer uses
  constraint strings alone to create vector identity.
- Do not reuse `is_vector`, `vector_lanes`, or `vector_bytes`; those remain GCC
  vector storage metadata.

## Proof

Ran delegated full-suite proof successfully:
`bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure >> test_after.log 2>&1'`.

Result: `100% tests passed, 0 tests failed out of 3345`.
Proof log: `test_after.log`.

Supervisor acceptance also ran:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
