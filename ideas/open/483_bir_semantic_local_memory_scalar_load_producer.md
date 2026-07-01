# BIR Semantic Local-Memory Scalar Load Producer

Status: Open
Type: Focused BIR semantic producer implementation idea
Parent: `ideas/closed/422_bir_semantic_producer_high_impact_cleanup.md`
Source Evidence:
- `build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv`
- `build/agent_state/422_step3_first_producer_packet/decision.md`
- `build/agent_state/422_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic local-memory load producer

## Goal

Produce explicit BIR semantic facts for ordinary scalar local-memory loads when
the frontend/lowering pipeline has durable source object, access range, layout,
and result identity evidence.

## Why This Exists

Idea 422 found 373 semantic `lir_to_bir` producer failures and selected the
local-memory load family as the first implementation-ready ordinary C bucket.
That bucket has 79 rows and a coherent BIR producer owner, but it needs focused
probes and a fail-closed scalar local-load contract before implementation.

Representative rows:

- `src/20041124-1.c`
- `src/20071219-1.c`
- `src/991228-1.c`
- `src/multi-ix.c`
- `src/pr22098-1.c`

## In Scope

- Audit focused per-representative local-memory load probes.
- Define the scalar local-object load contract from BIR semantic producer facts.
- Require explicit semantic load identity and scalar result type.
- Require explicit source memory/address value, local object or frame-slot
  identity, layout, size, alignment, and access range.
- Admit only ordinary C scalar local-load shapes that do not depend on GEP,
  pointer/provenance, byval/va_arg, aggregate/member, volatile/atomic, complex,
  vector, F128, bootstrap, or target fallback inference.
- Add focused fail-closed tests before broadening the producer surface.

## Out Of Scope

- Local-memory GEP/address producer repair.
- Local-memory store producer repair.
- Direct-call argument/result metadata producer repair.
- Memcpy/memset byte/object-representation producer work.
- Alloca-derived object identity/lifetime/layout production.
- Scalar/local-memory mixed, function-signature, call-return, scalar-binop, or
  bootstrap row repair.
- Prepared/RV64 recovery from raw BIR shape, value names, testcase names, final
  homes, or target fallback inference.
- RV64 target lowering, branch/select consumers, stack-home materialization, or
  call/return lowering.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused probes identify the exact scalar local-load shape selected for the
  first implementation packet.
- The producer contract names accepted evidence and rejected adjacent shapes.
- Positive coverage proves at least one ordinary scalar local-memory load gets
  explicit semantic producer facts.
- Negative coverage keeps aggregate/member, GEP-derived, pointer/provenance,
  byval/va_arg, volatile/atomic, complex, vector, F128, bootstrap, and
  RV64-recovery shapes fail-closed.
- Fresh residual disposition says whether the 79-row local-memory load bucket
  needs additional load subfamily follow-ups or can hand off to the next BIR
  producer bucket.
- Default backend proof remains stable for any code-changing packet.

## Reviewer Reject Signals

Reject patches that:

- infer local-memory load semantics from testcase names, value names, raw dump
  order, final homes, or RV64 target fallback behavior;
- combine load, store, GEP, call, memcpy/memset, alloca, or bootstrap producer
  repairs into one broad rewrite;
- accept aggregate/member, pointer/provenance, byval/va_arg, volatile/atomic,
  complex, vector, or F128 loads without explicit contract coverage;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim classification-only changes as implementation progress;
- make `conversion.c` or F128 paths drive this ordinary C scalar local-load
  producer route.
