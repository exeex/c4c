Status: Active
Source Idea Path: ideas/open/345_builtin_vrm_register_carrier_types_to_regalloc_frontier.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reject Non-Expanded VRM Call Boundaries

# Current Packet

## Just Finished

Step 6 rejected non-expanded bare VRM carriers at real function-call ABI
boundaries while preserving declaration-only signature carrier identity.

Completed work:
- Direct calls with bare `__c4c_builtin_vrm1/2/4/8` parameters now diagnose
  during HIR call lowering before LIR/BIR call ABI lowering can invent a
  convention.
- Direct call expressions whose result type is bare `c4c.vrmN` now diagnose
  before the result is materialized as an ordinary call value.
- Explicit returns from functions returning bare `c4c.vrmN` now diagnose during
  HIR statement lowering.
- Function pointer calls use tracked local/parameter/global `FnPtrSig`
  metadata to reject bare VRM parameters through the same call-boundary path.
- Focused frontend LIR tests cover direct parameter, direct result, explicit
  return, and reachable function-pointer parameter diagnostics while retaining
  declaration-only VRM signature preservation and verifier drift checks.

## Suggested Next

Ask the plan owner to decide whether the active source idea can close or
whether final allocation/substitution/object-byte follow-up should remain as a
separate open initiative.

## Watchouts

- This runbook intentionally stops at the regalloc frontier. Final allocation,
  physical substitution, prepared-to-machine propagation, and object-byte
  emission remain follow-up work.
- ABI and storage semantics remain intentionally narrow: ordinary call/return
  ABI lowering, memory storage, address-taking, arrays/members, arithmetic,
  casts, comparisons, and lane operations remain future work.
- Step 6 intentionally rejects only bare VRM carriers with no pointer, array,
  or reference layer; it does not implement VRM storage, memory, or ordinary C
  ABI semantics.
- Manual `PreparedRegisterGroupOverride` fixtures can still exercise regalloc
  mechanics on scalar-shaped values; source inline asm metadata no longer uses
  constraint strings alone to create vector identity.
- Do not reuse `is_vector`, `vector_lanes`, or `vector_bytes`; those remain GCC
  vector storage metadata.

## Proof

Ran delegated Step 6 proof successfully:
`bash -lc 'cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R "^(negative_tests_|cpp_negative_tests_|frontend_lir_|backend_)" >> test_after.log 2>&1'`.

Result: passed.
Proof log: `test_after.log`.

Supervisor acceptance also ran:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_)'`
