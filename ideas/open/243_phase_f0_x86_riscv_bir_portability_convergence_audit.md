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
