# 243 Phase F0 x86/riscv BIR portability convergence audit

## Goal

Audit what remains after Phase E5 before the prepared/BIR thinning program can
claim that x86 and riscv are portable over the same BIR semantic facts.

This idea is analysis-only. It must produce follow-up ideas and a durable
convergence map. It must not implement adapter work, open draft 155, delete
prepared aggregates, or claim prepared retirement readiness directly.

## Starting Position

The end state should be more precise than "delete `PreparedBirModule`":

- BIR owns semantics.
- Route facts, indexes, and identities gradually become BIR-owned.
- Prepared owns only policy that is still target- or emission-specific.
- ABI, layout, register, stack, emission, and target wrapper policy may remain
  in prepared/target code while they still consume BIR-owned facts.
- Prepared APIs become private pass context instead of a public durable IR.
- Duplicate semantic helpers are deleted once their BIR-owned replacements are
  proven across target behavior.

The desired convergence point is that x86 and riscv can port the same semantic
route facts without each target re-deriving a private second copy of BIR
semantics through prepared state.

## Prerequisite

This idea is open because E5 and its first narrow successor adapters have
closed:

- `ideas/closed/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `ideas/closed/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`
- `ideas/closed/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
- `ideas/closed/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`

The audit must start from those closure notes and verify:

- whether draft 155 should be rewritten, superseded, opened later, or kept
  blocked;
- the `PreparedBirModule` and `PreparedFunctionLookups` field groups that are
  ready for demotion, must remain as target policy, or remain blocked;
- whether open E4 follow-up 238 closed before E5 ran or remained a blocker;
- the fallback, diagnostic/oracle, string-authority, and cross-target
  compatibility surfaces that still prevent retirement.

E5 concluded that broad demotion remains blocked. F0 must therefore turn that
blocker map and the 240-242 adapter evidence into smaller x86/riscv
portability follow-up ideas.

## Candidate Work Streams

### 1. x86/riscv BIR fact parity audit

Audit whether the semantic facts already consumed by x86 are portable enough
for riscv, and whether riscv has target-local prepared inference that should
instead consume BIR-owned facts.

Expected questions:

- Which BIR facts exist for x86-selected routes but are absent, unproven, or
  unconsumed for riscv?
- Which riscv consumers still infer route identity, source identity, block
  identity, or producer/consumer relationships from prepared shape?
- Which facts are genuinely semantic and should become target-neutral BIR
  indexes?
- Which facts are target policy and should stay target-local?

Expected output:

- Follow-up ideas for each missing portable fact or target-specific blocker.
- Explicit non-goals for ABI, layout, register, stack, emission, and wrapper
  policy that must remain target-owned.

### 2. Prepared public lookup demotion packets

Turn remaining public `PreparedFunctionLookups` and prepared lookup-group
surfaces into private pass context, BIR-owned indexes, target-policy products,
or compatibility adapters.

Expected questions:

- Which public lookup APIs are still durable semantic authority?
- Which consumers only need a transient packet during codegen?
- Which lookup groups can be replaced by BIR-owned indexes?
- Which lookup groups must remain because diagnostics, fallback, or tests still
  observe prepared naming/status behavior?

Expected output:

- One demotion or blocker idea per lookup group.
- A compatibility plan for preserving fallback behavior and status strings.

### 3. Target-policy wrapper split packets

Separate semantic route facts from target wrapper policy so prepared/target code
owns only ABI, layout, register, stack, emission, formatting, and wrapper
decisions.

Expected questions:

- Where does wrapper code still re-derive semantic route/source facts?
- Which wrapper decisions are pure policy over BIR facts?
- Which x86-only evidence is insufficient for riscv or cross-target claims?
- Which target-local adapters are required while both targets converge?

Expected output:

- Small target-specific ideas that consume BIR facts without moving policy into
  BIR.
- A proof strategy that preserves wrapper output, fallback behavior, and
  expected strings.

### 4. Diagnostic/oracle authority replacement packets

Move diagnostic, helper-oracle, route-debug, and expected-string authority away
from prepared shape where they are still acting as semantic proof.

Expected questions:

- Which tests or debug rows still prove behavior by observing prepared
  structure instead of BIR facts?
- Which helper-oracle names and statuses must remain stable during transition?
- Which prepared printer/debug surfaces are still needed for regression
  diagnosis?
- Which string authorities can be transferred only after both x86 and riscv
  prove the same semantic fact?

Expected output:

- Follow-up ideas for oracle/status replacement without expectation weakening.
- Explicit baseline and string-stability proof requirements.

### 5. PreparedBirModule field deletion packets

After E5 maps each field group, create final field-group deletion or demotion
ideas only for fields whose semantic authority has moved elsewhere.

Expected questions:

- Which fields are purely duplicate semantic cache and can be deleted?
- Which fields should become private pass context?
- Which fields should become target-policy products?
- Which fields must remain because public consumers, diagnostics, fallback, or
  cross-target parity are not ready?

Expected output:

- One final deletion/demotion idea per safe field group.
- A blocker list for fields that cannot yet move.
- A full-suite and baseline proof strategy for each final packet.

## Expected Output

The closure note must produce:

- a post-E5 convergence map grouped by BIR semantic facts, prepared pass
  context, target policy, diagnostics/oracles, and deletion candidates;
- a clear x86/riscv parity assessment;
- follow-up ideas for every remaining blocker or safe final packet;
- explicit criteria for when prepared aggregate retirement can proceed;
- explicit criteria for when draft 155 should be rewritten, superseded, or
  retired as obsolete.

## Out Of Scope

- Direct implementation of any retirement packet.
- Broad deletion of `PreparedBirModule` or `PreparedFunctionLookups`.
- Claiming riscv readiness from x86-only evidence.
- Moving ABI, layout, register, stack, emission, or wrapper policy into BIR.
- Weakening expected strings, helper-oracle statuses, supported-path contracts,
  fallback behavior, route-debug output, wrapper output, or baseline tests.

## Reviewer Reject Signals

- Treating "BIR owns semantics" as permission to move target policy into BIR.
- Treating prepared as retired while public prepared APIs still act as durable
  semantic authority.
- Hiding remaining prepared semantic state behind renamed private wrappers.
- Opening deletion work without x86/riscv parity proof or an explicit blocker
  exception.
- Producing one broad retirement idea instead of field-group or fact-family
  follow-ups.

## Closure Note

Closed by the Phase F0 convergence audit after Steps 1 through 6 completed.
This was an analysis-only lifecycle slice; it did not modify implementation,
tests, docs outside `ideas/`, or draft 155.

### Post-E5 Convergence Map

#### BIR Semantic Facts

- Route 3 memory/source identity has one proven narrow AArch64
  agreement-gated adapter from E5 successor 240. It does not prove riscv
  edge-publication memory-source parity or whole `memory_accesses` retirement.
- Route 4/5 edge-publication identity has one proven narrow AArch64
  current-block join-source adapter from E5 successor 241. It does not prove
  riscv direct edge-publication consumption or whole
  `edge_publications`/`edge_publication_source_producers` retirement.
- Route 6 scalar call-use source identity has one proven narrow AArch64
  call-result adapter from E5 successor 242 and narrow x86 scalar `i32`
  route-debug / `ConsumedPlans` prerequisite evidence from closed idea 238.
  That evidence does not prove broad x86 wrapper migration, riscv readiness,
  or route-wide call-plan retirement.
- x86 currently has the strongest Route 6 scalar call argument diagnostic
  evidence, but wrapper authority still depends on prepared agreement,
  `ConsumedPlans`, helper-oracle statuses, fallback, and stable assembly.
- riscv currently has prepared edge-publication consumption only. Route 5 is
  the candidate owner for edge/source identity and Route 3 is the candidate
  owner for memory/source identity, but both must be agreement-gated against
  prepared publication/source-memory authority before riscv can claim parity.

#### Prepared Pass Context

- `PreparedBirModule` and `PreparedFunctionLookups` remain mixed public
  compatibility, private pass context, target policy product, and duplicate
  semantic cache. No whole aggregate, lookup group, or field group is ready for
  deletion, privatization, or replacement.
- Prepared lookup delivery remains required for diagnostics, helper-oracle
  tests, route-debug compatibility, wrappers, fallback behavior, and baseline
  diagnosis.
- Prepared APIs may become private pass context only after route-native facts
  prove the same positive, missing, mismatch, duplicate, fallback, and wrapper
  behavior for both affected target paths.

#### Target Policy

- BIR should own route/source/edge/memory semantic identity facts.
- Target code keeps ABI, layout, register, stack, scratch-register, offset,
  instruction-spelling, formatting, wrapper, and emission policy.
- x86 Route 6 call-wrapper work may consume BIR source identity, but
  call-wrapper assembly, ABI placement, call-bundle policy, helper/carrier
  protocol, and fallback policy remain target/prepared-owned.
- riscv Route 5/Route 3 edge-publication work may consume BIR edge/source and
  memory/source identity, but register moves, stack loads, large-offset
  address materialization, pointer-base handling, and fail-closed unsupported
  policy remain target-owned.

#### Diagnostics And Oracles

- x86 route-debug rows, helper-oracle statuses, `ConsumedPlans` compatibility,
  direct-call wrapper assembly, focus behavior, and prepared dump equivalence
  are compatibility authority until route-native Route 6 diagnostics prove the
  same matrix beside them.
- riscv `EdgePublicationMoveIntentStatus`, prepared publication/source-memory
  statuses, helper-oracle behavior, fallback behavior, unsupported fail-closed
  behavior, and exact instruction text are compatibility authority until
  route-native Route 5/Route 3 diagnostics prove the same positive and negative
  cases beside them.
- Prepared status strings must not be renamed, collapsed into ambiguous
  route-native names, or treated as route ownership proof from string equality
  alone.

#### Deletion Candidates

- No `PreparedBirModule` field group is deletion-ready now.
- No `PreparedFunctionLookups` group is deletion-ready now.
- No prepared publication/status compatibility surface is deletion-ready now.
- The only safe final packet is a later field-group demotion gate after the
  x86 Route 6, riscv Route 5/Route 3, and prepared compatibility-retention
  ideas close.

### Follow-Up Ideas Opened

- `ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
  for x86 Route 6 call-wrapper diagnostic/oracle replacement.
- `ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md`
  for the riscv Route 5/Route 3 edge-publication adapter.
- `ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md`
  for retained prepared publication/status compatibility.
- `ideas/open/247_phase_f1_final_prepared_field_group_demotion_gate.md`
  for a future final prepared field-group demotion/deletion readiness gate, with
  the explicit current finding that no deletion candidate is ready.

### Prepared Aggregate Retirement Criteria

Prepared aggregate retirement can proceed only after field-group-specific proof
shows:

- route-native BIR facts cover the semantic identity previously supplied by
  the prepared group;
- x86 and riscv affected consumers prove positive, missing, mismatch,
  duplicate, fallback, wrapper-output, and baseline behavior;
- prepared diagnostic/oracle/status strings are either preserved as explicit
  compatibility output or replaced by route-native diagnostics with identical
  externally required behavior;
- target policy remains outside BIR;
- public prepared fallback is removed only after no supported path or baseline
  still depends on it.

### Draft 155 Disposition Criteria

- Rewrite draft 155 only when the later field-group gate can name narrow
  deletion/demotion packets with complete x86/riscv, diagnostic, fallback,
  wrapper, baseline, and public-consumer proof.
- Supersede draft 155 if the useful scope is fully covered by smaller
  fact-family and field-group ideas.
- Keep draft 155 blocked while any prepared public lookup group, diagnostic
  string, fallback behavior, wrapper output, or target-policy boundary remains
  public authority.
- Retire draft 155 obsolete only if successor ideas close all intended
  aggregate-retirement scope without requiring a broad prepared aggregate
  deletion plan.

Closure validation used `c4c-regression-guard` with a docs/lifecycle-only
matching before/after backend diagnostic subset:
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication`. The guard used
`--allow-non-decreasing-passed` and passed with no new failing tests.
