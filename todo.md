# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair structured result and aggregate operand authority

## Just Finished

- Lifecycle switch from 801: its Step 1 remains accepted in `827dae5bd3` and
  its Step 2 repair remains preserved but unaccepted. The first full-baseline
  failure is aggregate SSA authority in `LirExtractValueOp.agg`, which is 754
  scope; historical 754 Step 2 acceptance cannot make Step 3 runnable.

## Suggested Next

- Step 2 only: trace and repair the supported aggregate SSA path that reaches
  `LirExtractValueOp.agg` without a valid `LirValueId`. Preserve structured
  authority and add nearby same-feature coverage; do not weaken the verifier.

## Watchouts

- Do not parse display text, repeat Step 1, claim historical Step 2 as current
  acceptance, begin index/result-type layout validation, touch Raw-BIR, or
  absorb 801's anonymous-layout work. Preserve 801's unaccepted repair and
  802's parked unaccepted hunk.

## Proof

- Blocker evidence: fresh `cmake --build --preset default`, then
  `ctest --test-dir build -R '^positive_sema_ok_call_builtin_runtime_c$'
  --output-on-failure` and
  `ctest --test-dir build -R '^llvm_gcc_c_torture_src_complex_2_c$'
  --output-on-failure` each fail at `LirExtractValueOp.agg: aggregate SSA
  operand requires valid LirValueId authority`.
- Before accepting Step 2: fresh build, focused aggregate/frontend/backend
  proof, and supervisor-owned 100% full baseline acceptance.
