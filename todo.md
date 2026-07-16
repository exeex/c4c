Status: Active
Source Idea Path: ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Decompose The Remaining Aggregate-Owner Rejection Families

# Current Packet

## Just Finished

Completed Step 1 current-evidence decomposition after the closed 837
architecture initiative. The historical residual aggregate-owner subset no
longer reproduces: the fresh build plus focused reproduction covering
`frontend_hir_tests`, `frontend_lir_global_type_ref`, and the 13 historical
`llvm_gcc_c_torture` cases passed 15/15.

No structured-key, matching-module-owner, or no-owner compatibility residual
group is currently visible on the representative evidence. Because the source
resume condition required fresh current evidence to reproduce an independently
owned residual family before further repair, there is no Step 2 code packet to
execute inside 836.

## Suggested Next

Send the exhausted/no-current-failure runbook to lifecycle review. The
appropriate disposition is an intentional conclusion, not a capability repair:
close 836 with the current 15/15 reproduction evidence and return
`ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
to unchanged Step 4 for its comparable full-suite gate.

## Watchouts

- Do not assume the old 3026/3038 rejected candidate still reproduces after
  the architecture work.
- Do not edit 831, 830, 829, Raw-BIR, importer, or generic call code.
- Do not change expectations, filters, allowlists, unsupported markers, or
  diagnostics to clear the route.
- Preserve malformed, foreign, wrong-namespace, and genuinely ownerless
  rejection.

## Proof

Passed current focused reproduction:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_lir_global_type_ref|llvm_gcc_c_torture_src_(20090113_2|930719_1|931012_1|950512_1|961112_1|comp_goto_1|pr23604|pr28289|pr37780|pr43385|pr46909_2|pr51323|pr88714)_c)$' ) > test_after.log 2>&1`

Result: 15/15 tests passed. No old `LIR-owned aggregate function type
requires ...` diagnostic appears in `test_after.log`.
