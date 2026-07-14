# Current Packet

Status: Active
Source Idea Path: ideas/open/758_lir_rvalue_value_identity_preservation_for_computed_goto.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Preserve typed local and parameter rvalue identity
你該做code review了

## Just Finished

- Switched from paused 757 Step 1 to its separately scoped rvalue-identity
  preservation prerequisite; 757's source resumption record owns the exact
  computed-goto return point.

## Suggested Next

- Execute Step 1 only: preserve existing typed local and parameter rvalue
  identity through the expression/operand route without text recovery.

## Watchouts

- Do not add the `LirIndirectBrOp` address field/verifier, alter computed-goto
  successor authority, or change Raw-BIR work; those remain outside this
  prerequisite.

## Proof

- Before the interrupted 757 prototype, a fresh build and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed 1/1 in `test_before.log`.
- Executor: run a fresh build and the same focused proof after this bounded
  route change.
- Supervisor: select broader/full acceptance and manage canonical regression
  logs separately.
