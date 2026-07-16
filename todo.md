# Current Packet

Status: Active
Source Idea Path: ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish first-owner decomposition from exact reproductions

## Just Finished

- Step 1 established two separate pre-existing owners from the exact 14-case
  reproduction. `frontend_hir_tests` still segfaults; a narrow backtrace puts
  its first owning seam at `typespec_aggregate_owner_key`, reached from
  `lir_owned_type_spec` / `populate_lir_function_params` in
  `test_hir_to_lir_object_helper_callees_prefer_link_name_ids`. The clean,
  backend-enabled `f0fc85e4f^` reproduction independently segfaults there.
- The 13 named GCC torture tests all instead stop at the unchanged-before-
  `f0fc85e4f` `verify_truthiness_lhs_parameter_authority` seam, which rejects
  missing `LirCmpOp.truthiness_lhs_parameter_authority` for a native
  direct-scalar truthiness LHS. These are separate first-owner families; no
  repair or baseline clearance is claimed.

## Suggested Next

- Plan-owner: create ordered separate successors before code changes: first,
  a narrowly scoped `typespec_aggregate_owner_key` / LIR function-parameter
  ownership crash diagnosis and repair for the named HIR test; second, a
  native `LirCmpOp.truthiness_lhs_parameter_authority` producer/verification
  completion route for the 13 GCC torture cases. Do not combine the routes.

## Watchouts

- Preserve the accepted `f0fc85e4f` focused 1/1 result and do not change 830.
  No expectation downgrade, unsupported marker, allowlist, test filter, or
  weaker harness contract can clear this baseline gate.

## Proof

- Baseline provenance: `test_baseline.log` at `8418036b` accepted 3038/3038;
  `test_baseline.new.log` at `f0fc85e4f` rejected 3024/3038. Step 1 starts
  with the exact 14-failure reproduction command recorded in the source idea.
- Fresh Step 1 reproduction: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^(frontend_hir_tests|llvm_gcc_c_torture_src_(20090113_2|930719_1|931012_1|950512_1|961112_1|comp_goto_1|pr23604|pr28289|pr37780|pr43385|pr46909_2|pr51323|pr88714)_c)$'`
  failed 14/14: one HIR segfault and the 13 matching truthiness-authority
  frontend rejections.
