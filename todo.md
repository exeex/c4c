# Current Packet

Status: Active
Source Idea Path: ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the manual switch authority seam

## Just Finished

- Lifecycle switch from 820 Step 3: its accepted DirectScalar producer/verifier
  work remains preserved in the 820 resumption record; no 820 broader-checkpoint
  completion is claimed.

## Suggested Next

- Trace only the manual `frontend_lir_call_type_ref` switch fixture and the
  existing structured modelled-result authority seam named by Step 1.

## Watchouts

- Do not modify DirectScalar, Raw-BIR, importer, generic scalar receipt, or
  generic switch behavior. The prior failure is a fixture/modelled-result
  authority blocker, not evidence against `4bcc7c8ff`.

## Proof

- Initial blocker evidence: `ctest --test-dir build --output-on-failure -R
  '^frontend_lir_call_type_ref$'` stopped at `LirSwitch.selector: must identify
  a current-function integer value definition` before DirectScalar tests. This
  is diagnosis-only evidence; Step 3 owns fresh acceptance proof.
