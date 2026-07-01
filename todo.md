Status: Active
Source Idea Path: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Infrastructure Bucket Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by rebuilding the infrastructure bucket inventory
from the fresh RV64 gcc_torture backend-object handoff artifacts. Direct TSV
and per-case-log extraction confirms:

- `unsupported_stack_frame`: 84 rows
- `unsupported_global_data`: 40 rows
- `unsupported_param_home`: 4 rows

Representative rows and final diagnostics are recorded in
`docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`.
Local row evidence was also written under
`build/agent_state/424_step1_infrastructure_bucket_evidence/`.

## Suggested Next

Execute Step 2: create focused BIR, prepared-BIR, MIR, and object-emission
dumps for representative rows from each target bucket, then classify each as a
producer-contract gap, coherent RV64 emission gap, or intentionally parked
scope.

## Watchouts

- Do not implement global-data, stack-frame, or parameter-home lowering inside
  this triage route.
- Split missing or incoherent prepared facts from coherent RV64 emission gaps.
- Do not infer labels, relocations, zero-fill ranges, frame slots, offsets,
  widths, or parameter homes in RV64 from testcase names, source shape, value
  names, object labels, or final unsupported buckets.
- Keep F128 local-memory, F128 parameter homes, and F128 ABI plumbing out of
  this review.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- The full-scan target row directories only preserve `case.log`; Step 2 needs
  focused dumps before making owner decisions.
- Keep runtime mismatch rows and the older instruction-fragment residual plan
  separate from this infrastructure inventory.

## Proof

Evidence-only proof was written to `test_after.log`:

- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Rebuild Infrastructure Bucket Evidence'`
- `awk`/`rg` extraction over
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv` and
  per-case logs
- `git diff --check -- todo.md docs/rv64_gcc_torture_post_contract`
