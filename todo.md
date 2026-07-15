# Current Packet

Status: Active
Source Idea Path: ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select aggregate producer authority seams

## Just Finished

- Lifecycle switch from 754 Step 2: the formerly accepted direct-call-only
  contract cannot authorize local-load or constructed-insertvalue aggregate
  producer provenance. The two focused failures require a separate bounded
  prerequisite; no 754 repair is accepted.

## Suggested Next

- Step 1 only: trace the two selected aggregate producer paths from creation
  to `LirExtractValueOp.agg`, identify their exact existing IDs and the
  smallest checked carrier/verifier seams. Do not implement or widen scope.

## Watchouts

- Do not use display text or raw fallback, broaden to generic operand/
  expression provenance, add extractvalue index/layout/result rules, or touch
  Raw-BIR. Preserve 754 Step 2 for the exact post-handoff return.

## Proof

- Reproduce with fresh build then `ctest --test-dir build -R
  '^(positive_sema_ok_call_builtin_runtime_c|llvm_gcc_c_torture_src_complex_2_c|frontend_hir_tests$|backend_)' --output-on-failure`.
- Current named failures are the runtime and direct-complex aggregate paths;
  raw classification is explicitly rejected because it evades authority.
