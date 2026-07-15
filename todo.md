# Current Packet

Status: Active
Source Idea Path: ideas/open/820_lir_directscalar_parameter_producer_verifier_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the boundary and record the 734 handoff

## Just Finished

- Step 3 causal probe reproduced `frontend_lir_call_type_ref` failing at
  `LirSwitch.selector: must identify a current-function integer value definition`.
  It is independent of Idea 820: the failure is in that test's manually-built
  switch fixture, which is exercised before every DirectScalar test, while
  820 changed only `init_fn_ctx` body-parameter publication and the native
  DirectScalar verifier predicate.
- `git show 4bcc7c8ff^` confirms the switch fixture and the switch-selector
  verifier branch are byte-for-byte outside the 820 commit. The smallest
  repair is a separate frontend LIR switch-fixture/modelled-result-authority
  packet; it is out of scope for 820 and does not authorize Raw-BIR receipt.

## Suggested Next

- Return to 734 only with its selected `LirBinOp.lhs` receiver authorization
  after the separate frontend switch blocker is resolved or explicitly routed;
  retain the DirectScalar boundary and do not broaden it into generic scalar
  receipt.

## Watchouts

- The focused failure occurs before the test's DirectScalar boundary tests and
  cannot be attributed to the producer/verifier changes in 4bcc7c8ff. No
  implementation or Raw-BIR files were changed by this probe.

## Proof

- Focused causal probe failed as expected:
  `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  It aborts at `LirSwitch.selector: must identify a current-function integer
  value definition`. This diagnosis-only packet intentionally did not write a
  root proof log and does not establish full acceptance.
