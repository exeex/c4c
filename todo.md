# Current Packet

Status: Active
Source Idea Path: ideas/open/818_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one body-parameter use authority row

## Just Finished

- Lifecycle resumption: 819 is capability-complete. Its accepted
  `DirectScalar` native definition and `LirBinOp.lhs` role binding are now
  available solely for 818 Step 1 re-evaluation; 818 has not selected a row or
  advanced Steps 2–3.

## Suggested Next

- Execute Plan Step 1 only: determine whether the accepted 819 scalar
  `LirBinOp.lhs` tuple is receiver-ready, select/trace it if so, otherwise
  return a separately scoped blocker. Do not advance Steps 2–3.

## Watchouts

- `param_slots`, rendered names/signatures, `LirOperand::raw`, diagnostics,
  and testcase identity are forbidden authority.
- Do not touch Raw-BIR/importer/dispatcher/receiver work or reuse accepted
  817 / separate 795 authority as this row.
- 819 permits only the exact plain fixed scalar `LirBinOp.lhs` tuple; do not
  generalize to rhs, another operator, or another parameter form.

## Proof

- 819 accepted proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
  passed 6/6, with matching `^backend_` before/after regression guard and no
  new failures. Step 1 is selection/trace only; no new receiver proof is
  implied.
