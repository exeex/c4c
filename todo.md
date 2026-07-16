Status: Active
Source Idea Path: ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md
Source Plan Path: plan.md
Current Step ID: COMPLETE
Current Step Title: CFG/PHI raw-binding evidence runbook complete

# Current Packet

## Just Finished

Completed 850 evidence docs under `docs/lir_cfg_phi_raw_bindings_evidence/`.
The evidence maps `LirPhi` and terminator forms through producer fields,
verifier checks, Raw-BIR importer paths, and accepted receiver tests. It
concludes that current modeled CFG/PHI forms already carry native authority
and no new direct 734 handoff is authorized by 850.

## Suggested Next

Route the exhausted 850 runbook to plan-owner for close. The expected
disposition is documentation/evidence complete with no direct 734 receiver
handoff.

## Watchouts

This is documentation/evidence work only. Do not edit implementation files,
tests, expectations, unsupported markers, allowlists, runtime behavior, or
Raw-BIR receiver code. Do not recover block, value, type, owner, predecessor,
or edge facts from rendered text.

## Proof

Completed proof:

```sh
find docs/lir_cfg_phi_raw_bindings_evidence -maxdepth 1 -type f -printf '%f\n' | sort
git diff --check
```

Result: PASS. The directory contains exactly
`01_cfg_phi_raw_binding_route.md` and `index.md`.
