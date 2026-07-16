Status: Active
Source Idea Path: ideas/open/849_lir_intrinsic_binding_evidence.md
Source Plan Path: plan.md
Current Step ID: COMPLETE
Current Step Title: Intrinsic binding evidence runbook complete

# Current Packet

## Just Finished

Completed 849 evidence docs under `docs/lir_intrinsic_binding_evidence/`.
The evidence traces `LirInlineAsmValueBinding` producers, verifier checks,
Raw-BIR inline-asm import, and selected builtin/intrinsic receiver families.
It concludes that inline-asm value bindings have a native structured route,
templates/constraints remain opaque, and no direct 734 handoff is authorized
without a future singular 796 or 846 handoff.

## Suggested Next

Route the exhausted 849 runbook to plan-owner for close. The expected
disposition is documentation/evidence complete with no direct 734 receiver
handoff.

## Watchouts

This is documentation/evidence work only. Do not edit implementation files,
tests, expectations, unsupported markers, allowlists, runtime behavior, or
Raw-BIR receiver code. Do not parse inline-assembly templates or constraints
for binding, type, value, ABI, or dispatch facts.

## Proof

Completed proof:

```sh
find docs/lir_intrinsic_binding_evidence -maxdepth 1 -type f -printf '%f\n' | sort
git diff --check
```

Result: PASS. The directory contains exactly
`01_intrinsic_inline_asm_binding_route.md` and `index.md`.
