# Remaining Coverage First-Owner Matrix

Status: 793 Step 2 classification from post-792/post-7.30 evidence.

The accepted 792/734 stack-save row is historical only. A row marked
unreceived below is not receiver-ready merely because it is unreceived; it
remains fail closed until its named first owner publishes a checked structured
handoff.

| Remaining family | First owning layer | Structured evidence consulted | Dependency order | Evidence gap / successor readiness |
| --- | --- | --- | --- | --- |
| Local/VLA: stack restore, dynamic VLA allocation, VLA GEP, nonselected load/store/GEP, local temporaries, and lifetime consumers | Producer/schema/verifier | Step 1 baseline; `docs/lir_local_operation_authority/handoff_to_734.md`; closed 792; 734 post-7.30 exhaustion | Publish and verify one selected local/lifetime row, then its one-row Raw-BIR receipt | Gap: 792 selects only `LirStackSaveOp`'s saved-pointer result. The handoff explicitly excludes these rows, so none has receiver authorization. |
| Memory and `va_list`: memcpy-like residuals, memset, va-start/arg/end/copy, pointer/object lifetime | Producer/schema/verifier | Open 753; closed 752; 734 post-7.30 exhaustion | Closed 752 local-object substrate -> 753 structured memory/va pointer/lifetime authority -> one bounded receiver handoff | Gap: 753 says the broader operations still carry monostate/text operands. Accepted selected memcpy does not authorize the remaining family. |
| Aggregate/vector: object/value identity, field/member access, index/mask carriers | Producer/schema/verifier, with type-model prerequisite where composite type is needed | Open 754; open 763; 734 post-7.30 exhaustion | 763 composite carrier where needed -> 754 result/use and row-specific facts -> one bounded receiver handoff | Gap: 754 records aggregate/vector producers as largely text-based. No exact receiver row is named. |
| Body parameters: body-use identity, ABI-expanded/byval/aggregate/HFA/vector/variadic forms | Producer/schema/verifier | Closed 742 and its handoff record; 734 post-7.30 exhaustion | Publish body-use authority and classify logical-to-ABI-expanded forms -> bounded receiver handoff | Gap: 742 completed declaration/definition logical-parameter publication only; it expressly excludes body parameter identity and leaves complex ABI shapes classified, not received. |
| Module/type/global/metadata: declarations, extern/function/global shadows, struct declarations, initializers, metadata | Type-model first for composite forms; then producer/schema/verifier module convergence | Open 763, 761, and 762; 734 post-7.30 exhaustion | 763 composite `LirTypeRef` -> 761 call/signature type mirrors -> 762 module declaration shadows -> bounded module/global/metadata handoffs as structured evidence permits | Gap: 763 retains `runtime_text` for deferred forms; 761/762 are open convergence routes. No evidence names a receiver-ready global initializer or metadata row. |
| Instruction/terminator/inline-assembly residuals: ordinary calls/casts/binary/unary/intrinsics, PHI/CFG-adjacent rows, remaining terminators, inline-asm value/type bindings | Producer/schema/verifier; type-model for type-mirror blockers | 734 resumption/exhaustion records; open 761; closed 786 and later accepted bounded historical rows noted by 734 | For each residual, publish missing value/edge/type authority -> bounded Raw-BIR receipt; inline asm must retain opaque template/constraint text | Gap: 734 states only a bounded inline-asm path and selected earlier rows are accepted; it still lists other instruction/terminator/inline-asm rows as unreceived. 761 does not authorize parsing opaque templates or constraints. No combined successor is justified. |
| Final no-omission matrix, dispatcher, proof, and documentation convergence | Dispatcher/proof, followed by documentation convergence | 734 post-7.30 exhaustion; 793 scope and Step 1 baseline | Finish every preceding producer/type-model/receiver handoff -> enumerate each valid fact and typed disposition -> complete dispatcher/neighbors -> whole-module transactional proof -> documentation convergence | Gap: 734 explicitly records the matrix, per-row disposition, complete dispatcher, whole-module proof, and documentation convergence as unmet. This is terminal convergence work, not authority for an earlier receiver row. |

## Classification rules retained for Step 3

- A Raw-BIR receiver is never first owner for a row whose source authority is
  missing, text-only, monostate, or merely classified.
- Closed 792 has no authority beyond the accepted VLA stack-save handoff and
  does not change the local/VLA row classification.
- The table records existing scoped evidence, not successor ideas. Step 3 must
  choose a single bounded owner only after preserving these dependencies and
  explicit gaps.
