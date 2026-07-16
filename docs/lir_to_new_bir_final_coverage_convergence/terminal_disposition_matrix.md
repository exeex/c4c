# 797 Terminal Disposition Matrix

Status: Step 1 matrix for `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`

## Inputs

- `plan.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/813_lir_string_semantic_authority_completion_umbrella.md`
- `ideas/closed/847_lir_universal_model_string_escape_hatch_deletion.md`
- `ideas/closed/866_lir_remaining_authority_owner_triage.md`
- `docs/lir_string_semantic_authority_completion/handoff_to_734_and_797.md`
- `docs/lir_string_semantic_authority_completion/row_to_owner_map.md`
- `docs/lir_string_semantic_authority_completion/closure_trace.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`
- `docs/lir_memory_va_object_lifetime_authority/handoff_to_734.md`

## Disposition Vocabulary

- `accepted terminal input`: 797 may use the row as terminal matrix input.
- `accepted no-change/evidence disposition`: accepted evidence says no new
  direct 734 receipt is authorized, or the row is intentional text/no-change.
- `blocked prerequisite`: 797 must stop before Step 2 until the named
  prerequisite closes outside 797.

## Matrix

| Current-LIR semantic family | Owner disposition | Receiver disposition | Proof status | 797 disposition |
| --- | --- | --- | --- | --- |
| Accepted historical 734 bounded receipts through Step 7.51 | 734 records accepted Raw-BIR receiver/import/verifier work through commit `750b6b3ba`; accepted history also includes the earlier selected store/load/GEP/return, direct-call, arithmetic/intrinsic, CFG/PHI, local-object, VLA, memcpy, and other bounded receiver rows named in the 734 source history. | Received by 734 where explicitly accepted. No row is reopened or generalized by 797. | Accepted 734 commits and their focused/broader proof remain historical evidence. | accepted terminal input |
| Compact scalar and ABI-leaf authority | Closed 841 completed bounded producer/schema migration for compact scalar and ABI-leaf ownership. | No direct 734 receiver handoff was accepted from 841; future receiver work still requires one exact typed scalar or ABI-leaf handoff naming a bounded Raw-BIR row. | Closed 841 records accepted commits and proof. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Restricted first-class value unions: call, PHI, select, return boundaries | Closed 842 completed bounded producer/schema work for named first-class boundary alternatives. | No direct 734 receiver handoff was accepted from 842; future Raw-BIR receipt requires one exact typed row accepted by a named boundary owner. | Closed 842 records accepted commits and proof. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Direct HIR family construction and recursive array composition | Closed 843 completed bounded producer construction and recursive array composition. | No direct 734 receiver handoff was accepted from 843; future 734 return requires one exact aggregate/vector/array row accepted by a successor. | Closed 843 records accepted commits and proof. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Global and extern type facts | Closed 844 completed global/extern type facts and explicitly excludes initializer-text semantics and non-type global policy. | Return to 734 only after one exact typed global or extern handoff is accepted; no direct receiver receipt is recorded by 844. | Closed 844 records accepted commits and proof. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Typed reference carriers and collector migration | Closed 845 completed collector/import-preparation after producer carriers. It is not first owner for missing producer facts. | 734 may receive only after a carrier migration accepts an exact field that a bounded Raw-BIR row can consume; no direct receiver receipt is recorded here. | Closed 845 records accepted route evidence. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Family overloaded verifier, dispatch, and printer migration | Closed 846 completed verifier/dispatch/printer migration for already-published native facts. | Any later 734 receipt still requires a selected accepted handoff for one already published native fact; 846 does not create producer state or direct receiver work. | Closed 846 records accepted route evidence. | accepted no-change/evidence disposition for 797; no Step 2 receiver work authorized |
| Global policy and symbol identity evidence (`global.policy-identity-evidence`) | Closed 848 is evidence-complete through 813. It found Raw BIR already stores and validates global object identity/policy facts, while LIR still carries weak/visibility policy in `linkage_vis` and const/global policy in `qualifier` without a complete global-family verifier contract. | 813 authorizes no new direct 734 receiver handoff. Future work requires a later exact structured producer/verifier route or evidence-backed no-change disposition before any receipt. | Closed 848 and 813 row-to-owner/closure docs are accepted evidence. | accepted no-change/evidence disposition |
| Intrinsic and inline-assembly binding evidence (`instruction.intrinsic-binding-evidence`) | Closed 849 is evidence-complete through 813. Selected inline-assembly value bindings have native structured routes; templates, constraints, and clobbers remain opaque payload. The evidence does not prove a complete inventory for every builtin helper form. | 813 authorizes no new direct 734 receiver handoff. Future singular residual instruction, terminator, or inline-assembly handoffs must return first to exact 796 or 846 scope before any 734 receipt. | Closed 849 and 813 row-to-owner/closure docs are accepted evidence. | accepted no-change/evidence disposition |
| CFG/PHI raw binding evidence (`cfg.phi-raw-bindings-evidence`) | Closed 850 is evidence-complete through 813. Current modeled PHI and terminator forms carry native block/value/type/edge authority. | 813 authorizes no new direct 734 row because accepted bounded 734 receipts already cover the modeled CFG/PHI Raw-BIR receiver rows described by the evidence. Future raw seams need separate producer/verifier ownership before 734. | Closed 850 and 813 row-to-owner/closure docs are accepted evidence. | accepted terminal input for modeled rows; accepted no-change/evidence disposition for no new receiver work |
| Memory/VA/object/lifetime authority: selected direct-local `LirVaStartOp` destination `va_list` pointer authority | Closed 867 accepted the producer-side handoff for exactly one row, carried by `LirVaStartOp.ap_authority` with `requires_native_memory_va_authority`. | 734 records accepted Step 7.52 receiver commit `a680b50e8`, which receives the selected direct-local `LirVaStartOp` destination `va_list` authority into typed Raw BIR and verifies the handed-off pointer authority tuple. | Closed 867 focused proof accepted `backend_lir_selected_pointer_authority`; 734 Step 7.52 accepted matching focused before/after proof over `backend_lir_to_bir_interface` and `backend_lir_selected_pointer_authority`, with a non-decreasing regression guard. | accepted terminal input |
| Universal model and string escape-hatch deletion | Closed 847 deleted or fenced terminal universal-model and string escape-hatch surfaces in scope, including implicit conversions, mutable string accessors, selected semantic string authority, and expired adapters. | No 734 return is required by 847. Its return is terminal valid-LIR disposition handoff to 797, while 734 receipts remain downstream only where applicable. | Closed 847 records full-suite proof: build plus full ctest, 3038/3038 passing. | accepted terminal input |
| Body-parameter authority beyond accepted bounded rows | Open 795 preserves its own bounded body-parameter scope; 866 classifies it as stale for current post-Step-7.51 successor selection and says accepted body-parameter/direct-call parameter rows must not be reopened here. | 797 has no authority to absorb 795 or create receiver rows from it. | Open 795 remains outside this packet; accepted bounded rows are historical evidence only. | accepted no-change/evidence disposition for 797; preserve open 795 |
| Residual instruction/terminator authority beyond accepted bounded rows | Open 796 preserves only selected residual producer-family scope. 866 says accepted cast-result and pointer-subtraction records are evidence, not blanket ownership of every residual instruction, terminator, or inline-assembly seam. | 734 is downstream only after exact accepted 796 or 846 handoff. No new receiver row is authorized by 797. | Open 796 remains outside this packet; accepted bounded rows are historical evidence only. | accepted no-change/evidence disposition for 797; preserve open 796 |
| Switch selector authority | Open 821 and 822 retain switch selector routes under their own return chain. 847 explicitly leaves switch selector surfaces outside its handoff. | 797 has no authority to absorb 821/822 or fabricate a 734 receipt. | Open 821/822 remain outside this packet. | accepted no-change/evidence disposition for 797; preserve open 821 and 822 |
| Intentional opaque/render/presentation text | 813 and 866 preserve templates, constraints, labels, predecessor/case/printer text, names, rendered operands, and other display-only strings as non-authoritative unless a separate source proves otherwise. | No receiver route. 734 and 797 must reject reconstruction from text, `monostate`, compatibility mirrors, printer output, or testcase identity. | Accepted 813/866 documentation evidence. | accepted no-change/evidence disposition |

## Step 1 Result

Step 1 finds no missing first-owner handoff or missing 734 receipt that blocks
797 from continuing.

The 867 producer handoff initially requires a 734 receiver packet, but
`ideas/open/734_lir_to_new_bir_container_completeness.md` records accepted
post-Step 7.52 receipt commit `a680b50e8` for exactly that selected
direct-local `LirVaStartOp` destination `va_list` authority. That row is
therefore terminal input to 797, not an outstanding prerequisite.

The next 797 action is Step 2: compare dispatcher, importer destination
containers, and reachable verifier behavior against this matrix and repair
only matrix-proven gaps.
