# Current Packet

Status: Active
Source Idea Path: ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the automatic local initializer authority handoff

## Just Finished

- Plan switch completed from 768 Step 5; no 768 Step 5 implementation is
  accepted or committed.

## Suggested Next

- Trace the nested automatic local label-address table initializer through its
  generic coordinator/initializer-list consumer and define the focused
  structured-authority contract for Step 1.

## Watchouts

- Do not add automatic-table `DeclRef` decay, carrier/verifier/backend work,
  synthetic bridges, raw-text recovery, testcase routing, or expectation
  downgrades.

## Proof

- Step 1 is planning/ownership mapping only. Before any code acceptance, use a
  fresh build, the dedicated focused frontend-LIR proof, and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_'`.
