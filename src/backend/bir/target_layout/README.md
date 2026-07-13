# Verified Target Layout Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: product
Phase-ID: C2
Upstream: exact C1 validated `TargetProfile` and complete `TargetFingerprint` plus verifier-bound `VerifiedPreparationInput`
Downstream: one immutable exact-key `VerifiedTargetLayout` consumed first by C3 ABI preparation
Owner-Path: `src/backend/bir/target_layout/README.md`
Last-Reconciled-Commit: `eab55fde0`

## Purpose

C2 is the sole derivation and verification authority for the finite abstract
target layout used by BIR preparation and allocation. It consumes the exact C1
validated profile/fingerprint and the verifier-bound exact Canonical input,
selects one closed layout table under exact schema/options versions, verifies
every finite relation, and atomically publishes one immutable
`VerifiedTargetLayout`. It never reconstructs target identity, inspects
Canonical instructions, or mutates Raw or Canonical BIR.

## Owns

- stable target-layout category, class, group, slot, alias-unit and mapping-rule
  ID schemas;
- the exact per-profile finite slot domains, reserved/eligible sets, alias
  relations, capacities, group legality and ABI eligibility tables;
- the private abstract-slot-to-concrete-unit mapping domain and its version;
- complete C2 product keys, deterministic derivation/verification, structured
  failures, atomic publication and transitive invalidation; and
- the sole immutable `VerifiedTargetLayout` capability accepted by C3.

## Does Not Own

- C1 target request selection, normalization, defaulting, validation,
  `TargetProfile` or `TargetFingerprint` formation;
- Canonical verification or the verifier's `VerifiedPreparationInput` binding;
- parsing rendered `data_layout`, architecture/triple-only reconstruction,
  target facts imported from Raw/Canonical or allocator/prepared state;
- semantic type layout, per-value ABI classification, selected argument/result
  slots, call/variadic/address/helper plans or constraint interpretation;
- liveness, assignment, eviction, homes, spills, frame placement, machine
  opcodes, rendering or emission; or
- publication/value-flow or any other analysis as layout identity or
  publication authority.

## Inputs

C2 receives one exact mutually bound Canonical/target capability set. All
inputs are immutable. Equality of architecture, triple, register counts or
rendered strings cannot substitute for any capability or complete key.

### Exact C2 input matrix

| Input / product | Exact required state | Optional / empty form | Failure / forbidden substitution |
|---|---|---|---|
| validated profile | the one immutable C1 `TargetProfile` accepted for this request | none | directly constructed, mutable, partial or merely compatible profile is `TargetLayoutProfileInvalid` |
| target fingerprint | complete C1 `TargetFingerprint` covering every request/default/capability/registry axis | none | architecture/triple-only hash, omitted axis or reconstructed equality is `TargetLayoutTargetKeyInvalid` |
| preparation input | one borrowing verifier-issued `VerifiedPreparationInput` over the same profile/fingerprint and Canonical owner | none | report, copied token, foreign target or unbound profile is `TargetLayoutBindingInvalid` |
| Canonical stamp | exact B8 `PipelineStageStamp`: epoch, module revision, ordered function digest, plan/options lineage, ordinal 7 and all P01-P07 properties | empty module retains the exact empty function digest | Raw/view/candidate, stale/partial/mixed stamp or semantic hash is `TargetLayoutCanonicalInvalid` |
| target-profile schema | exact accepted `TargetProfile`/capability-registry versions named by C1 | none | missing or different version is `TargetLayoutProfileSchemaInvalid` |
| layout schema | exact `TargetLayoutSchemaFingerprint` naming all ID/table/verifier rules | none | caller-selected or unknown schema is `TargetLayoutSchemaInvalid` |
| layout options | normalized semantic `TargetLayoutOptionsFingerprint`; v1 has no free option and uses the canonical empty value | exact canonical empty options only | worker/cache/environment/pointer state or target-changing option is `TargetLayoutOptionsInvalid` |
| mapping registry | exact concrete-mapping-table version named by the target fingerprint and layout schema | none | backend-local names, stale table or compatible version is `TargetLayoutMappingVersionInvalid` |
| execution control | deterministic cancellation/resource bound for finite validation | observer absent is valid | unbounded/nondeterministic policy is `TargetLayoutResourceInvalid` |

## Outputs

Success publishes exactly one immutable `VerifiedTargetLayout` containing all
tables below under one product key. Embedded tables and fingerprints are
read-only views, never independent capabilities. Failure publishes no layout,
table prefix, capacity, mapping, cache entry or successor fact.

### Exact C2 output matrix

| Embedded output / result | Exact consumer | Required binding | Optional / error form |
|---|---|---|---|
| `VerifiedTargetLayout` capability | C3 ABI preparation, later declared consumers | exact C1 profile/fingerprint, Canonical stamp, schema/options/mapping versions and verifier token | no partial/compatible capability; failure returns none |
| category table | C3 classification and later allocation | closed stable `PseudoCategoryId` rows and supported-class references | unsupported category is explicit `Absent`, never inferred |
| class table | C3 eligibility and later constraints/allocation | closed stable `PseudoClassId`, category, widths and feature predicate | disabled class is explicit `Absent` with capacity zero |
| group table | C3 aggregate requirements and later allocation | stable `PseudoGroupId`, class, width, alignment, contiguity and legal bases | unsupported width is `Absent`; invalid request is failure |
| abstract slot table | later constraints/allocation | every stable `PseudoSlotId`, class views and exactly one alias unit | empty class has an empty table, never an unknown count |
| alias relation | later constraint/allocation validation | stable `AliasUnitId`, reflexive/symmetric/transitive closure and cross-view aliases | distinct units are known non-alias; incomplete/asymmetric relation fails |
| reserved/eligible sets | C3 ABI eligibility and later allocation | exact disjoint fixed-reserved, ABI-only and allocatable memberships | optional capability-disabled set is known empty |
| capacity table | C3 feasibility and later allocation | exact eligible alias-unit count per class/group after reservations | zero is a valid known capacity; guessed architectural count forbidden |
| ABI eligibility table | C3 sole per-value classifier | exact calling role -> legal class/group/slot-set relation only | stack-required/unsupported relation is explicit; no selected slot |
| private concrete-mapping domain | E4 target-realizability and F1 apply-only mapping after allocation | stable mapping-rule ID and total slot/alias-unit -> target-unit record | not public naming/assignment authority; missing required mapping fails C2 |
| structured `TargetLayoutFailure` | caller/diagnostics | stable rule/table/ID/key/cause and deterministic order | report grants no layout, cache or preparation capability |

## Adjacent-Stage Contract

The [external C1 owner](../../../target_profile/README.md) alone supplies the
validated immutable profile and complete target fingerprint. The [shared
verifier](../verify/README.md) alone binds that fingerprint to the exact
unchanged Canonical `PipelineStageStamp` in `VerifiedPreparationInput`. C2
consumes both authorities without recreating either, verifies one finite
layout, and publishes the only `VerifiedTargetLayout` capability.

[C3 ABI preparation](../preparation/abi/README.md) accepts only that capability
with the same target fingerprint and Canonical stamp. C3 may classify semantic
parameters/results against layout-owned eligibility and class/group facts, but
cannot change a table, choose a concrete unit, infer a missing class or use
architecture/triple similarity. C2 never requests publication/value-flow;
that analysis begins at C3, its first real semantic classification consumer.

## Stable ID and Product-Key Schema

IDs are typed stable values scoped by the exact target/layout schema. Dense
indices may exist inside one verified table but cannot escape or become keys.

| Stable ID / key | Exact identity | Optional / failure rule |
|---|---|---|
| `PseudoCategoryId` | closed v1 category ordinal | unknown ID is `TargetLayoutCategoryInvalid` |
| `PseudoClassId` | category plus closed class ordinal and feature predicate | disabled class is `Absent`; unknown ID fails |
| `PseudoGroupId` | class plus width/alignment/contiguity rule ordinal | unsupported group is `Absent`; malformed group fails |
| `PseudoSlotId` | target fingerprint, category storage domain and stable physical-unit ordinal | no pointer/name identity; missing ordinal fails |
| `AliasUnitId` | exact underlying storage-unit ordinal under one target fingerprint | every slot has exactly one; missing/duplicate unit fails |
| `MappingRuleId` | mapping-table version plus target-unit rule ordinal | private until allocated/E4/F1 use; missing rule fails |
| `TargetLayoutKey` | Canonical `PipelineStageStamp`, complete `TargetFingerprint`, profile/layout/mapping schemas and normalized options | mismatch/staleness is `TargetLayoutKeyMismatch` |
| `TargetLayoutFingerprint` | deterministic digest of the complete key and every verified table row in stable-ID order | subset/table-count/architecture hash is never equality |

Every downstream handle retains the complete `TargetLayoutKey` and
fingerprint. A changed Canonical stamp, target fingerprint, profile/capability
registry, layout/mapping schema or semantic option invalidates the layout and
every C3-C9 successor; no old handle retargets.

## Closed Category, Class, and Group Matrix

The v1 vocabulary is finite. `AggregateAddress` is a GPR class view and adds no
slot or capacity. Float/vector views may share alias units as specified below;
their capacities are never added together.

| Category | Closed class | Legal semantic carrier | Closed groups | Optional / failure form |
|---|---|---|---|---|
| `Gpr` | `General` | integer, pointer and address-sized scalar | `Single(1,align1)`, `Pair(2,align1)`, `AlignedPair(2,align2)`, `Quad(4,align1)` | unavailable group is `Absent`; illegal width/alignment fails |
| `Gpr` | `AggregateAddress` | address of stack-required/by-value aggregate | `Single(1,align1)` only; aliases `General` slot-for-slot | no separate slot/capacity; non-address use fails |
| `Fpr` | `Float32` | IEEE binary32 scalar/lane | `Single`, `Pair`, `Quad` with target ABI legal bases | absent without required float ABI/capability |
| `Fpr` | `Float64` | IEEE binary64 scalar/lane | `Single`, `Pair`, `Quad` with target ABI legal bases | absent without required float ABI/capability |
| `Fpr` | `Float128` | semantic binary128 carrier only where one registered group exists | exact registered `Single` or `Pair`; otherwise `Absent` | C3 must choose stack/helper requirement, never synthesize slots |
| `Vreg` | `Vector64` | admitted 64-bit vector carrier | `Single`, `Pair`, `Quad` under capability table | absent when vector capability disabled |
| `Vreg` | `Vector128` | admitted 128-bit vector/aggregate carrier | `Single`, `Pair`, `Quad` under capability table | absent when vector capability disabled |
| `Vreg` | `Vector256` | admitted 256-bit vector carrier | `Single` and registered contiguous groups only | absent without exact wide-vector capability |

Group legality is computed from explicit legal-base tables, never from
`slot_index + width <= count` alone. Every group expands to exact ordered
`PseudoSlotId`s whose alias units are pairwise distinct.

## Closed Per-Profile Slot and Capacity Matrix

Symbolic ranges are exact stable ordinals, not renderer register names.
`FixedReserved` members are excluded before capacity is counted. A conditional
row is selected only when that capability/OS bit is present in the exact C1
fingerprint; otherwise its table is explicitly absent.

| Exact profile/capability | Storage domain and exact slots | Fixed reserved units | Allocatable singleton capacity | ABI-visible subset / optional form |
|---|---|---|---:|---|
| RV64 LP64 `Gpr` | `RG[0..31]` / `RU_G[0..31]`; `General` and aliasing `AggregateAddress` views | `RG{0,2,3,4}` zero/SP/GP/TP | 28 | args `RG[10..17]`, results `RG{10,11}`; complete |
| RV64 LP64 soft-float `Fpr` | no slots | none | 0 | ABI set empty; class `Absent` |
| RV64 LP64F `Fpr` | `RF[0..31]` / `RU_F[0..31]`; `Float32` view only | none | 32 | args/results `RF[10..17]` / `RF{10,11}`; `Float64/128` ABI eligibility absent |
| RV64 LP64D `Fpr` | `RF[0..31]` / `RU_F[0..31]`; `Float32/64` views, `Float128` absent | none | 32 | args `RF[10..17]`, results `RF{10,11}`; `Float32/64` enabled |
| RV64 `V` capability `Vreg` | `RV[0..31]` / `RU_V[0..31]`; `Vector64/128`, plus `Vector256` only when exact width capability is present | exact capability-table mask, v1 empty | 32 | no C ABI argument/result eligibility in v1; capability absent -> class `Absent` |
| AArch64 AAPCS64 `Gpr` | `AG[0..30]` / `AU_G[0..30]`; `General` and aliasing `AggregateAddress` views | `AG{29,30}` FP/LR; Darwin also `AG{18}` | 29 Linux, 28 Darwin | args `AG[0..7]`, indirect result `AG{8}`, results `AG{0,1}` |
| AArch64 AAPCS64 float/vector shared domain | `AV[0..31]` / `AU_V[0..31]`; `Float32/64/128` and `Vector64/128` aliasing views; `Vector256` absent | none | 32 shared, never 64 | args `AV[0..7]`, scalar results `AV{0,1}`, HFA/HVA results legal bases in `AV[0..7]` |
| x86-64 SysV `Gpr` | `XG[0..15]` / `XU_G[0..15]`; `General` and aliasing `AggregateAddress` views | `XG{4,5}` stack/frame bases | 14 | args `XG{7,6,2,1,8,9}`, results `XG{0,2}` |
| x86-64 SysV float/vector shared domain | `XV[0..15]` / `XU_V[0..15]`; `Float32/64` and `Vector64/128` views; `Float128` absent; `Vector256` only with AVX | none | 16 shared | float args `XV[0..7]`, results `XV{0,1}`; `Vector256` requires AVX capability |
| i686 SysV `Gpr` | `IG[0..7]` / `IU_G[0..7]`; `General` and aliasing `AggregateAddress` views | `IG{4,5}` stack/frame bases | 6 | arguments stack-required; integer results `IG{0,2}` |
| i686 SysV SSE2 float/vector shared domain | `IV[0..7]` / `IU_V[0..7]`; `Float32/64` and `Vector64/128` views; wider classes absent | none | 8 shared | ordinary C arguments stack-required; SSE2 absent -> domain/classes `Absent` |

Exact target capability tables may remove additional reserved units only by a
new fingerprinted schema row. They cannot silently reserve a unit at runtime,
and temporary sets never create capacity beyond the allocatable alias units.

## Alias, Reservation, and Capacity Rules

| Rule family | Exact rule | Optional / failure form |
|---|---|---|
| same ordinal scalar views | x86 byte/word/dword/qword and AArch64 W/X views of one GPR slot share one alias unit | a view absent from a class is `Absent`; overlapping units must alias |
| same ordinal float/vector views | AArch64 S/D/Q/V and x86 XMM/YMM views share the domain's one alias unit per ordinal | AVX-disabled YMM view is `Absent`, not a new unit |
| RV float widths | RV32F/RV64D semantic views of `RF[n]` share `RU_F[n]` | disabled D view is `Absent` |
| cross-domain relation | GPR, float and vector units do not alias except the explicitly shared float/vector domains above | any undeclared cross-domain alias fails verification |
| distinct ordinals | different alias-unit ordinals are known non-alias | duplicate physical mapping for distinct units fails |
| reservation | fixed-reserved and allocatable sets are disjoint; ABI-only membership does not imply reservation | overlap or missing mandatory reservation fails |
| capacity | count unique allocatable alias units satisfying the class/group legal-base rule | sum of aliasing views, names or architectural headline count is forbidden |
| temporary pool | an optional subset of allocatable units selected by exact table | cannot add units/capacity or override reservations |

## Exact ABI Eligibility Matrix

C2 publishes eligibility only. C3 chooses `register-eligible` versus
`stack-required` for a typed semantic value; D2 later performs transport.

| Profile family | Argument eligibility | Result eligibility | Group / hidden-carrier rule | Explicit non-eligible form |
|---|---|---|---|---|
| RV64 LP64 | integer/pointer `General` over `RG[10..17]`; float set depends exactly on LP64/F/D rows | integer `RG{10,11}`; float `RF{10,11}` only for matching hard-float profile | aligned pairs consume consecutive eligible units; no vector ABI row in v1 | excess/unsupported values are `StackRequired`, not missing layout |
| AArch64 AAPCS64 | `General` `AG[0..7]`; float/vector classes `AV[0..7]` | `AG{0,1}` or `AV[0..3]` as registered class/group permits | hidden result uses ABI-only `AG8`; HFA/HVA widths 1-4 require consecutive legal base | unsupported class/group is `StackRequired` or stable C3 failure |
| x86-64 SysV | `General` ordered `XG{7,6,2,1,8,9}`; float/vector `XV[0..7]` | `XG{0,2}` and `XV{0,1}` | class/group eligibility only; C3 owns eightbyte classification | x87/unsupported wide form is `StackRequired` or stable C3 failure |
| i686 SysV | ordinary arguments stack-required in v1 | integer `IG{0,2}`; no generic float result slot | no register argument group; hidden pointers follow stack requirement | float/x87 and unsupported aggregate result are explicit non-eligible forms |

No eligibility row selects a slot for a particular value, assigns a home,
creates a call move or establishes a concrete ABI placement.

## Private Concrete-Mapping Domain

The mapping domain is complete but private. Each `PseudoSlotId` and
`AliasUnitId` has exactly one target-unit record and `MappingRuleId` under the
exact mapping-table version. Renderer names are optional display metadata and
never keys. Reserved units also have records so verification can reject their
use. Shared float/vector views resolve to the same target-unit record.

| Symbolic slot domain | Exact private target-unit domain | View / alias rule | Failure rule |
|---|---|---|---|
| `RG[n]`, `n=0..31` | RV64 integer unit ordinal `n` | all GPR-width/address views use `RU_G[n]` | missing/different ordinal mapping fails |
| `RF[n]`, `n=0..31` | RV64 floating unit ordinal `n` | F/D views use `RU_F[n]` | capability-disabled view is absent; remap fails |
| `RV[n]`, `n=0..31` | RV64 vector unit ordinal `n` | all admitted vector-width views use `RU_V[n]` | V-disabled domain is absent; partial map fails |
| `AG[n]`, `n=0..30` | AArch64 integer unit ordinal `n` | W/X/address views use `AU_G[n]` | SP is not an `AG` slot; remap fails |
| `AV[n]`, `n=0..31` | AArch64 SIMD/FP unit ordinal `n` | S/D/Q/V views use `AU_V[n]` | separate float/vector physical units forbidden |
| `XG[n]`, `n=0..15` | x86-64 architectural GPR encoding-unit ordinal `n` | byte/word/dword/qword views use `XU_G[n]` | renderer spelling or reordered encoding is not identity |
| `XV[n]`, `n=0..15` | x86-64 SIMD unit ordinal `n` | XMM/YMM views use `XU_V[n]` | AVX-disabled YMM view absent; separate unit forbidden |
| `IG[n]`, `n=0..7` | i686 architectural GPR encoding-unit ordinal `n` | byte/word/dword views use `IU_G[n]` | renderer spelling or partial-byte view is not a new unit |
| `IV[n]`, `n=0..7` | i686 SIMD unit ordinal `n` | admitted XMM views use `IU_V[n]` | SSE2-disabled domain absent; partial map fails |

C2 exposes only mapping-domain identity and validation to later product keys.
E4 may query the private rule after exact assignment and target-realizability
verification; F1 applies that already selected rule. C2, C3 and D1 cannot use
the domain to choose a concrete unit, opcode, encoding or emitted spelling.

## Ordered Behavior

1. Validate exact C1 profile/fingerprint, verifier-bound Canonical stamp,
   schema/options/mapping versions and deterministic resource policy.
2. Select exactly one closed profile/capability table; unknown or ambiguous
   combinations fail before any table candidate becomes visible.
3. Build stable category/class/group/slot/alias/reservation/capacity/ABI/
   mapping rows in canonical ID order.
4. Verify ID uniqueness, complete references, alias equivalence, disjoint
   reservations, group legal bases, exact capacities, ABI subset membership,
   mapping totality and complete fingerprint coverage.
5. Recheck every input key, then atomically publish one immutable
   `VerifiedTargetLayout`; otherwise discard all private tables and publish
   none.

## Invariants

- One complete `TargetLayoutKey` names every table. No table, count, semantic
  hash, pointer or target name is an independent capability.
- All sets and maps use stable typed IDs and deterministic order. Optional
  classes/tables are explicit `Absent` or known empty, never partial.
- Alias relations are closed and capacities count unique eligible alias units.
- Canonical storage is read-only and target-independent. Selected target
  facts live only in immutable C1/C2 products.
- No allocator state, selected home, spill/frame fact or machine operation is
  an input or output.

## Verification, Failure, and Invalidation

The C2 verifier is part of the private publication transaction and checks the
complete candidate under one frozen key. Unknown profile combinations,
duplicate/foreign IDs, missing references, partial tables, asymmetric or
non-transitive aliases, reserved/eligible overlap, inconsistent capacities,
illegal groups/bases, ABI-ineligible mappings, incomplete concrete mappings,
stale Canonical/target keys, cancellation or resource exhaustion fail closed.

Failure publishes no layout, table prefix, capacity, verifier token, cache
entry or C3 fact. Inputs remain unchanged. Any Canonical stamp, target
fingerprint, profile/capability registry, layout/mapping schema or semantic
options change invalidates the capability and every C3-C9 successor. An old
handle never retargets; merely compatible layout data must be rederived and
reverified under the new exact key.

## Target and ABI Rules

Source-semantic sizes, alignments and address spaces in Canonical BIR are not
target-layout input. C2 never parses `target_profile`, triple or rendered
`data_layout` text from Raw/Canonical storage and never imports legacy
allocator pools, homes, spills or frame state. C1's validated target key is the
only target identity; C2 derives only finite layout vocabulary/eligibility.

## Implementation State

Implementation is absent. This directory contains only this README; it has no
build edge. No checked-in `VerifiedTargetLayout`, target-layout IDs/key/
fingerprint, closed profile tables, verifier, transaction, cache, C3 handoff or
focused runtime proof exists.

Legacy `backend/legacy/prealloc/target_register_profile.*` contains concrete
register-name pools and placement helpers, and other legacy paths contain
rendered-layout or allocator facts. They are migration evidence only: they do
not consume the exact C1/Canonical binding, publish this product key, verify
the closed tables above or satisfy C2.

## Proof Requirements

- prove exact C1 profile/fingerprint plus verifier-bound Canonical input and
  reject every reconstructed/compatible/imported substitute;
- prove every stable-ID table, optional/empty form, alias relation, reservation,
  capacity, ABI eligibility and mapping record is exhaustive and deterministic;
- prove atomic failure, complete invalidation and no Raw/Canonical mutation;
- prove C3 accepts exactly one same-key `VerifiedTargetLayout` and C2 performs
  no per-value classification or placement; and
- reconcile implementation claims with directory contents, build inclusion and
  callable reachability.

## Open Questions

None. Adding a profile, capability, class, group, slot domain, reservation,
alias or mapping rule requires a versioned coordinated C1/C2/C3 update.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Input/output and every finite table family are exhaustive.
- [x] Stable IDs, exact keys, optional/error forms and invalidation are closed.
- [x] Alias, reservation, capacity, ABI eligibility and mapping authority are exact.
- [x] Failure is atomic and Raw/Canonical remain unchanged.
- [x] C1/C2/C3 authority and adjacency are distinct.
- [x] Absent implementation truth is explicit.
