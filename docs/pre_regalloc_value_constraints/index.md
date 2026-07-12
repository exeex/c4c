# Pre-Regalloc Value Constraint Research

## Research set

1. [Existing constraint flow](01_existing_constraint_flow.md) traces named BIR
   identity through liveness and regalloc, classifies supported inputs versus
   derived policy, and locates the two current discontinuities: no general
   fixed/preferred request reaches `run_regalloc`, and allocation does not
   consume the constraint rows it publishes.
2. [Semantic owner and schema](02_semantic_owner_and_schema.md) compares BIR,
   prepared semantic, liveness, and regalloc ownership. It assigns authenticated
   ingress and target validation to prepared semantic state, normalization to
   `PreparedAllocationConstraint`, and enforcement to common regalloc while
   rejecting unrestricted value-name register controls.
3. [Deterministic proof and follow-up boundary](03_proof_and_followup_boundary.md)
   defines an independently meaningful explicit-register inline-assembly
   positive, adjacent fail-closed negatives, deterministic acceptance rules,
   forbidden shortcuts, and a narrow implementation boundary with a stop
   condition.

## Coherent conclusion

The existing `PreparedAllocationConstraint` surface is descriptive rather than
causal: target/liveness policy populates its preference fields, fixed fields are
absent, and common allocation independently rebuilds candidate pools. A valid
future carrier therefore needs both authenticated semantic ingress and
allocator enforcement.

The bounded ownership split is:

- prepared semantic state owns typed producer provenance, stable interned
  function/value identity, structured target identity, and target legality;
- `PreparedAllocationConstraint` owns the normalized per-`PreparedValueId`
  contract, including class, width, fixed/preferred/forbidden facts, and status;
- common regalloc owns constraint-aware candidate filtering, ordering,
  conflicts, and deterministic failure;
- BIR and liveness provide semantic/dataflow evidence but do not own arbitrary
  physical-register controls; targets consume results and do not repair them.

The research identifies explicit-register inline-assembly operands as the
narrow legitimate first producer family. Their placement meaning exists
independently of direct-edge publication, and current common code already has
structured operand metadata and AArch64/RV64 register-identity validation. A
semantic program can constrain a predecessor output and phi use to different
register identities, making a genuine edge move a deterministic consequence
rather than allocator luck.

## Follow-up boundary

A follow-up implementation idea may cover only prepared inline-assembly
explicit-register allocation constraints:

- preserve supported explicit-register operand identity before regalloc;
- publish authenticated prepared requests with typed operand provenance;
- normalize and enforce them in every common allocation/eviction pool;
- prove target legality, conflict behavior, a semantic non-coalesced phi move,
  typed direct-edge availability, and the documented negative matrix.

It must not expose a fixture or preparation map from arbitrary function/value
names to registers, broaden into x86 emission/ABI policy, mutate allocator
outputs or prepared homes, or weaken publication admission.

Stop instead of opening that implementation route if the frontend/lowering
cannot preserve one supported explicit-register operand family without a broad
inline-assembly redesign. In that case idea 722 still lacks a legitimate
deterministic register-source producer; allocator pressure or fixture injection
is not an acceptable substitute.

## Cross-document consistency review

| Check | Result |
|---|---|
| Earliest boundary | Consistent: answer 01 identifies missing pre-regalloc ingress plus missing enforcement; answers 02 and 03 address both rather than treating the existing row as active. |
| Ownership | Consistent: answer 01 defers architecture; answer 02 chooses prepared ingress / normalized regalloc contract / common enforcement; answer 03 stays within that split. |
| Identity and legality | Consistent: answers 02 and 03 require interned value identity, typed provenance, structured target identity, class/width agreement, and target validation. |
| Failure model | Consistent: invalid, missing, stale, ambiguous, mismatched, unsupported, and conflicting facts fail before or during common allocation; preferences alone may fall back. |
| Positive proof | Consistent: answer 03 uses semantic inline-assembly constraints whose meaning survives removal of the publication assertion; it does not contradict answer 01's finding that this ingress is absent today. |
| Scope | Consistent: all answers are documentation-only and leave implementation, tests, target emission, and idea 722 acceptance unchanged. |

No cross-document contradiction was found.

## Acceptance and reject-signal review

- Exact output contract: this directory contains `index.md` plus exactly the
  three required numbered answer files.
- Direct answers: answer 01 provides the requested trace/classification and
  boundary; answer 02 compares all four owners and decides a bounded schema;
  answer 03 supplies the semantic positive, negative matrix, deterministic
  rules, follow-up outline, and stop decision.
- Evidence: every answer cites concrete repository types, functions, and paths;
  architecture conclusions are separated from established current behavior.
- No testcase shaping: no fixture flag, block/value-name match, forced pair, or
  arbitrary named-value register API is recommended.
- No expectation-only progress: no unsupported reclassification, expectation
  downgrade, helper rename, or status relabel is claimed as capability repair.
- Complete schema criteria: identity, provenance, class/width, target legality,
  conflict behavior, failure status, ingress, and enforcement are explicit.
- No downstream reconstruction: x86 synthesis, post-prepare home mutation,
  allocator-output rewriting, and publication fallback are expressly forbidden.
- Narrow scope: the follow-up is limited to an authenticated inline-assembly
  producer family plus common normalization/enforcement; broad ABI, scheduling,
  publication, and emission work remains out of scope.
- Original failure is not renamed: the documents explicitly state that
  publishing rows without consuming them preserves the gap and must be rejected.

The research set satisfies idea 723's documentation acceptance criteria and
triggers none of its reviewer reject signals.
