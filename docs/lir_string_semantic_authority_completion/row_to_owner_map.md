# 813 Row To Owner Map

## Source Evidence

This map consumes the accepted 812 input validation in
`input_validation.md`. The complete active Step 2 input is exactly three
stable insufficient-evidence keys:

1. `global.policy-identity-evidence`
2. `instruction.intrinsic-binding-evidence`
3. `cfg.phi-raw-bindings-evidence`

No unowned semantic route rows remain in the 812 input. Closed ideas 848, 849,
and 850 answer the three evidence questions. Closed idea 847 is terminal
deletion evidence for 797 only; it does not make 797 a repair owner and does
not close 797.

## Stable Key Dispositions

| Stable key | Evidence source | Disposition | First owner / downstream relation |
| --- | --- | --- | --- |
| `global.policy-identity-evidence` | `ideas/closed/848_lir_global_policy_identity_evidence.md` and `docs/lir_global_policy_identity_evidence/` | Evidence complete; no direct 734 receiver handoff. Raw BIR stores and validates global object identity and policy facts, but LIR still carries weak/visibility policy in `linkage_vis` and const/global policy in `qualifier` without a complete global-family verifier contract. | 844 may reuse the evidence as non-type global policy context. A later exact producer/verifier route or evidence-backed no-change disposition is required before 734 receives anything. 797 waits for that downstream disposition. |
| `instruction.intrinsic-binding-evidence` | `ideas/closed/849_lir_intrinsic_binding_evidence.md` and `docs/lir_intrinsic_binding_evidence/` | Evidence complete; no direct 734 receiver handoff. Selected inline-assembly value bindings have native structured routes, and templates/constraints/clobbers remain opaque payload. This evidence does not prove a complete binding inventory for every builtin helper form. | Future singular residual inline-assembly or instruction binding handoffs belong first to 796. Verifier/dispatch/printer migration for already-published native facts belongs first to 846. 734 and 797 remain downstream until one exact 796 or 846 handoff is accepted. |
| `cfg.phi-raw-bindings-evidence` | `ideas/closed/850_lir_cfg_phi_raw_bindings_evidence.md` and `docs/lir_cfg_phi_raw_bindings_evidence/` | Evidence complete; no new direct 734 receiver handoff. Current modeled PHI and terminator forms carry native block/value/type/edge authority, and accepted 734 bounded receipts already cover the described Raw-BIR receiver rows. | If a future raw CFG/PHI seam appears, a separate producer/verifier owner must first publish exact native block/value/type/edge facts and malformed-boundary proof. 734 remains downstream with no new row from 850. 797 remains downstream of any future 734 disposition. |

## 847 Terminal Deletion Evidence

Closed idea 847 deleted or fenced the terminal universal-model and string
escape-hatch surfaces in its scope and recorded a full-suite proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1
```

Result: `100% tests passed, 0 tests failed out of 3038`.

This evidence is terminal input to 797 only. It does not absorb residual
non-type/string semantic routes owned by 812/813, switch selector surfaces
owned by 821/822, receiver rows owned by 734, or final 797 coverage
convergence.

## Step 2 Decision

No new open first-owner successor is created in Step 2. The three stable keys
are now mapped exactly once to completed-evidence dispositions and downstream
relations. 797 is not assigned repair ownership for any key.
