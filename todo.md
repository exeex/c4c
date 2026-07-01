Status: Active
Source Idea Path: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Infrastructure Bucket Evidence

# Current Packet

## Just Finished

Activation initialized this execution state for `plan.md` Step 1.

## Suggested Next

Execute Step 1: rebuild the current `unsupported_stack_frame`,
`unsupported_global_data`, and `unsupported_param_home` evidence from the RV64
gcc_torture handoff artifacts, then identify representative rows and dump paths
for classification.

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

## Proof

Lifecycle activation only. No build or test proof required yet.
