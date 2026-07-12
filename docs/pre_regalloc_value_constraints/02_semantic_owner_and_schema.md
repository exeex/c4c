# Semantic Owner and Schema Decision

## Decision

A bounded value-register constraint carrier is valid, but it must be a
target-aware **prepared semantic input** populated only from genuine semantic,
ABI, or machine-constraint producers. It must not be a general physical-register
annotation on BIR values, a liveness property, an allocator-output mutation, or
a fixture API for selecting convenient registers.

The carrier should live in `PreparedBirModule` beside
`PreparedRegisterGroupOverrides`, keyed by interned function/value identity and
available after liveness identity is established but before
`BirPreAlloc::run_regalloc`. Regalloc should normalize each admitted request
into the existing `PreparedAllocationConstraint` row and must then enforce that
row when constructing candidate spans. This keeps ingress/provenance separate
from the allocator’s normalized contract while making the published contract
an actual control surface rather than descriptive duplication.

This decision does not authorize an implementation or claim that the blocked
joined-branch program semantically requires two registers. A later producer
must derive requests from a real language, ABI, inline-assembly, or target
instruction constraint. Arbitrary value-name-to-register mapping is explicitly
invalid.

## Current ownership evidence

- `src/backend/prealloc/prealloc.cpp::BirPreAlloc::run` orders liveness,
  out-of-SSA, inline-assembly register-group override population, and regalloc.
  This is already a prepared semantic normalization seam before allocation.
- `src/backend/prealloc/module.hpp::PreparedBirModule` owns target profile,
  prepared names, liveness, `PreparedRegisterGroupOverrides`, and regalloc in
  one common target-aware module.
- `src/backend/prealloc/regalloc/classification.cpp` resolves class/width by
  stable function/value identity through
  `find_prepared_register_group_override`, demonstrating a pre-regalloc
  prepared carrier pattern without putting target policy into BIR.
- `src/backend/prealloc/regalloc.cpp::BirPreAlloc::run_regalloc` creates the
  sole `PreparedAllocationConstraint` row and performs assignment in the same
  phase, but assignment currently calls target pool helpers directly rather
  than consuming the row.
- `src/backend/prealloc/regalloc/assignment.cpp::choose_register_span` accepts
  ordered candidate spans and selects the first non-overlapping span. This is
  the narrow enforcement seam after a constraint-aware candidate list is
  validated and ordered.
- `src/backend/prealloc/target_register_profile.cpp` owns architectural pool
  membership and structured `PreparedRegisterPlacement` /
  `PreparedTargetRegisterIdentity` mapping for supported ABI identities.

## Comparison of plausible owners

| Candidate owner | Strengths | Blocking problems | Decision |
|---|---|---|---|
| BIR value/instruction schema | Earliest semantic source; naturally travels with definitions and phi inputs | BIR is target-independent semantic IR, while physical register names, pools, and target identities are target-specific. Adding a generic register field to `bir::Value`, `Param`, or every defining instruction would spread allocation policy through semantic IR and invite value-name-shaped controls. | Reject as carrier owner. BIR constructs may be evidence consumed by a prepared producer, but should not directly carry arbitrary physical assignments. |
| Prepared semantic module | Already combines interned identity, target profile, liveness results, inline-asm overrides, and phase ordering before regalloc. Can retain provenance and reject unsupported target identities before assignment. | Requires a new bounded carrier plus a producer/admission pass; must prevent callers and tests from injecting unproven arbitrary hints. | Accept as ingress owner, restricted to authenticated semantic/ABI/machine-constraint sources. |
| Liveness | Has stable `PreparedValueId`, function/value names, classifying type, interval, call crossing, and home-slot requirements immediately before regalloc. | Fixed/preferred physical placement is not a dataflow or lifetime fact. Mixing it into `PreparedLivenessValue` would make analysis output depend on target allocation policy and blur recomputation/validation boundaries. | Reject as owner. Liveness remains an input used to validate applicability and conflicts. |
| Regalloc / `PreparedAllocationConstraint` | Correct normalization and enforcement layer; already has class, width, fixed/preferred/forbidden fields and value IDs. | It is too late to invent semantic intent, currently lacks provenance, and its rows are outputs constructed during allocation. Direct fixture mutation or ad hoc row creation would bypass producer authentication. | Accept as normalized contract and enforcement owner, not as semantic ingress owner. |

## Bounded ingress schema

The exact C++ spelling is deferred, but the prepared input must be equivalent
to this contract:

```text
PreparedValueRegisterConstraintRequest
  function_name: FunctionNameId
  value_name: ValueNameId
  source_kind: semantic | abi | inline_asm | target_instruction
  source_reference: typed producer provenance
  strength: fixed | preferred | forbidden
  target_register: PreparedTargetRegisterIdentity
  register_class: PreparedRegisterClass
  contiguous_width: size_t
```

Rules:

1. Identity uses interned function/value IDs at ingress. `run_regalloc` resolves
   the unique `PreparedValueId` from liveness and records it in
   `PreparedAllocationConstraint`.
2. `PreparedTargetRegisterIdentity` is authoritative; raw spellings may be
   derived diagnostics only. Structured identity includes target architecture,
   bank, class, and physical index and prevents cross-target spelling reuse.
3. Class and width are explicit validation claims and must agree with
   `resolve_register_class`, `resolve_register_group_width`, and the candidate
   span represented by the target identity.
4. `source_kind` plus a typed reference is mandatory. A request with only a
   value name and register is incomplete and rejected.
5. Multiple preferred/forbidden requests may normalize into ordered vectors.
   At most one distinct fixed target span may survive for a value.
6. The carrier is internal prepared state. Public preparation options must not
   expose an unrestricted map from arbitrary value spellings to physical
   registers.

`PreparedRegisterPlacement` remains useful for pool/slot publication, but it
is not sufficient alone for fixed identity because caller/callee pool slots do
not represent every machine-constrained register family. Target identity plus
validated candidate-span membership is the stronger fixed claim.

## Admission and target legality

Before writing a normalized row, the prepared producer/admission pass must
reject:

- missing or ambiguous function/value identity;
- a request for a stack object, class `None`, or value with no allocatable live
  interval when the source contract requires a register;
- architecture mismatch between target profile and target identity;
- bank/class mismatch, zero width, or a width not supported by the value type
  and register-group override;
- a physical identity that cannot be materialized as a legal candidate span
  for the active target;
- fixed-register and `requires_home_slot` contradictions unless a separately
  defined semantic contract explicitly supports both storage obligations;
- missing, stale, or mismatched producer provenance.

Legality must be centralized in target-profile helpers. Inline assembly may
parse constraint spellings in `src/backend/prealloc/inline_asm.cpp`, but it
should publish the same structured identity rather than teaching general
regalloc about syntax. ABI helpers in
`src/backend/prealloc/target_register_profile.cpp` remain legitimate producers
for ABI-specific moves; they should feed general value allocation only when a
value truly has a live-range constraint, not merely because an edge move uses
an ABI register.

## Normalization into `PreparedAllocationConstraint`

For each allocatable liveness value, `run_regalloc` should combine:

- class/width and home requirements from liveness/classification;
- caller/callee saved default policy from the target profile;
- admitted prepared requests with producer provenance.

The normalized row should preserve current descriptive fields and add or link
provenance/status sufficient to distinguish default policy from semantic
requirements. Fixed identity narrows candidates to one validated span.
Forbidden identities remove spans. Preferred identities stably reorder legal
spans ahead of default policy; they do not make an illegal span legal.

Names and placements must be derived from the same validated spans, avoiding
independent string/placement lists that can contradict each other.

## Allocator enforcement

Enforcement belongs in common regalloc before
`regalloc_detail::choose_register_span`:

1. Look up exactly one normalized constraint by `PreparedValueId`.
2. Build target candidate spans once.
3. Filter forbidden spans.
4. If fixed, retain only the exact validated span.
5. Otherwise stably place preferred spans first and retain remaining defaults.
6. Pass that constrained ordered list to `choose_register_span` for both normal
   assignment and eviction replacement.

All three `assign_from_pool` passes in `run_regalloc` must use this same helper;
otherwise a fallback caller/callee pass could bypass a constraint. Published
assignment, placement identity, value homes, and later move records continue
to derive from the selected `PreparedPhysicalRegisterAssignment`.

## Conflict and failure behavior

- Two different fixed requests for one value: reject as ambiguous producer
  authority before allocation.
- Two interfering values fixed to overlapping spans: deterministic allocation
  failure; do not silently spill either fixed value or choose another register.
- Fixed span unavailable because an unconstrained value occupies it: normal
  eviction may relocate/spill the unconstrained value if existing rank rules
  permit; failure remains deterministic if the fixed requirement cannot be met.
- Preferred span unavailable: fall back to the remaining legal ordered spans;
  preference is not a requirement.
- Forbidden filtering empties the pool: spill only when the value is spillable
  and does not require a register; otherwise report allocation failure.
- Invalid class/width/target/provenance: reject during admission, not as an
  incidental spill.
- Constraint without matching liveness identity or normalized row without a
  matching regalloc value: fail closed as stale/incomplete prepared state.

Failure must be represented as a typed prepared/regalloc status or invariant,
not only a note or changed test expectation. Downstream targets must consume
the resulting assignment/status and must not reinterpret or repair constraints.

## Why this is not a joined-branch control

The blocked publication fixture supplies no semantic reason for its incoming
and phi destination to occupy distinct registers. This schema does not permit a
test to add such a reason by value name. A valid positive case must originate
from an existing or newly specified semantic/ABI/machine constraint whose
meaning independently requires or prefers the register identity. Whether such
a program can prove idea 722, and what deterministic negative matrix it needs,
is reserved for Step 3.

## Step 2 completion statement

The ownership ambiguity is resolved: prepared semantic state owns authenticated
ingress and target validation; `PreparedAllocationConstraint` owns normalized
per-value publication; common regalloc owns enforcement. BIR and liveness remain
evidence sources, not physical-register carrier owners. The carrier is valid
only with typed provenance and must not expose unrestricted named-value
allocation control.
