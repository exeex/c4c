# Prepared Inline-Assembly Explicit-Register Allocation Constraints

Status: Open
Type: Common prepared semantic and regalloc capability
Derived from: `ideas/closed/723_pre_regalloc_value_constraint_carrier_research.md`
Unblocks: `ideas/open/722_direct_edge_publication_available_move_contract.md`

## Parked Route Note (2026-07-12)

Step 1 triggered this idea's stop condition. `make_inline_asm_metadata` accepts
class-only register constraints, vector classes, ties, immediates,
memory/address operands, and clobbers, but `{x10}` reaches
`unsupported_constraint`. The later RV64 physical-name parser validates
already-assigned homes and is not semantic ingress. Therefore no
already-supported explicit-register operand family exists to preserve.

Idea 724 remains open and incomplete. It is parked behind
`ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md`, which
must determine whether a legitimate source/LIR syntax exists and whether
support can remain narrow. Do not resume this idea by treating clobbers as
value constraints or by adding fixture allocation controls.

## Goal

Preserve one supported RV64 explicit-register inline-assembly operand family as
authenticated prepared value constraints, normalize it into
`PreparedAllocationConstraint`, and enforce it in common regalloc so assignment
and downstream move publication are deterministic consequences of semantic
operand meaning.

## Why This Exists

Research under `docs/pre_regalloc_value_constraints/` proved that current
constraint rows are descriptive: fixed fields are never populated and the
allocator independently rebuilds candidate pools. It also identified explicit
inline-assembly register operands as a legitimate producer whose placement
meaning exists independently of the direct-edge testcase. A narrow end-to-end
implementation can establish both missing ingress and enforcement without
adding arbitrary value-name hints or target-consumer fallback.

## In Scope

- Localize one already representable RV64 explicit-register input/output
  operand spelling from semantic lowering into `InlineAsmOperandMetadata`.
- If that spelling is not currently preserved structurally, add only the
  minimal metadata needed to retain its explicit target identity; stop if this
  requires broad inline-assembly parsing redesign.
- Publish authenticated prepared requests before `run_regalloc`, keyed by
  interned function/value identity and typed inline-assembly operand provenance.
- Validate target architecture, bank/class, group width, candidate-span
  membership, value identity, and contradictory/ambiguous authority.
- Normalize the admitted fixed identity into `PreparedAllocationConstraint`
  with coherent names, placements, structured identity, and typed status.
- Enforce the normalized row in every normal and eviction candidate-pool pass.
- Prove a semantic predecessor-output / phi-use pair constrained to distinct
  registers creates a genuine non-coalesced out-of-SSA move and reaches
  `current_block_direct_edge_publication_sources` as `Available`.
- Cover the focused missing, stale, ambiguous, mismatch, unsupported,
  class/width, home, interference, publication, and freshness negatives from
  the research matrix.

## Out Of Scope

- Arbitrary function/value-name register maps, fixture controls, or preparation
  options for selecting physical registers.
- Preferred-register policy without an authenticated semantic producer.
- AArch64 or x86 expansion in the initial slice.
- Broad inline-assembly syntax work, ABI policy, target scheduling/emission,
  Route 3/Route 5, or joined-branch-specific lowering.
- Post-regalloc assignment rewriting, post-prepare home mutation, typed-view
  inference, downstream synchronization, or expectation downgrades.

## Required Invariants

- The semantic constraint remains meaningful when publication assertions are
  removed from the test.
- Structured target identity is authoritative; raw spelling is derived and
  cannot bypass target validation.
- Every normalized row has unique value identity and typed producer provenance.
- Fixed constraints are requirements: illegal, ambiguous, conflicting, or
  unenforceable authority fails deterministically rather than spilling or
  selecting another register silently.
- All assignment and eviction routes consume the same constrained candidate
  helper.
- Natural coalescing remains no-move behavior and is not reclassified.

## Acceptance Criteria

- One supported RV64 explicit-register operand family survives semantic
  lowering as structured authority and is admitted before regalloc.
- Common regalloc assigns the required identity independent of allocator order,
  unrelated pressure, or candidate-pool iteration.
- Distinct genuine inline-assembly constraints on a predecessor value and phi
  use produce a real edge move without injected prepared facts.
- The typed direct-edge query reports coherent register-source `Available`
  authority with selected freshness.
- Focused negative states remain precise and fail closed.
- Matching focused before/after regression proof passes; broader backend proof
  is required before close because common regalloc changes affect shared code.

## Stop Condition

Stop and return to lifecycle review if the semantic frontend/lowering cannot
preserve one supported explicit-register operand family without broad parser or
inline-assembly redesign. Do not replace the semantic producer with a fixture
map or allocator pressure.

## Reviewer Reject Signals

- Reject block-label, value-name, known-register-pair, or testcase-shaped
  matching presented as constraint production.
- Reject manual positive `PreparedAllocationConstraint`, assignment, value-home,
  move, publication, or freshness injection.
- Reject post-regalloc mutation, x86/target consumer synthesis, Route 5
  restoration, or typed-view inference.
- Reject expectation downgrades, unsupported reclassification, helper renames,
  status relabeling, or diagnostics-only changes claimed as capability repair.
- Reject constraint publication without enforcement in every normal and
  eviction candidate-pool route.
- Reject raw register spelling accepted without architecture, bank/class,
  width, span, identity, provenance, conflict, and failure validation.
- Reject broad ABI, emission, scheduling, multi-target, or inline-assembly
  syntax rewrites outside the initial RV64 operand family.
- Reject retaining the exact absent-ingress or ignored-constraint failure behind
  a new carrier name.
