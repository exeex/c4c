# x86 Prepared Edge Publication Consumer

## Goal

Implement the smallest real x86 consumer of shared prepared edge-publication
facts, using the handoff path identified by idea 19.

## Why This Exists

Idea 19 proved that shared prepared edge-publication facts are target-neutral
enough for non-AArch64 consumers and recommended x86 as the next implementation
target. x86 already has a prepared decoded-home fixture and an indexed shared
lookup surface available through:
`x86::ConsumedPlans::shared_function_lookups()->edge_publications`.

The next slice should turn that handoff into actual x86 lowering behavior
without creating duplicate edge-copy authority or moving target-local emission
policy into shared prepare.

## In Scope

- Consume indexed shared prepared edge-publication facts from the x86 lowering
  surface.
- Implement one narrow x86 edge or block-entry publication path that reads the
  shared semantic facts and emits through x86-owned lowering code.
- Keep target-local responsibilities in x86 code: scratch selection, clobber
  avoidance, physical register spelling, register-class constraints, stack
  operand syntax, move instruction spelling, branch/control-flow emission, and
  final assembly formatting.
- Add focused x86 tests proving the consumer uses shared prepared facts rather
  than rediscovering edge-copy semantics locally.
- Preserve existing AArch64 behavior and shared prepared lookup contracts.

## Out Of Scope

- Full x86 codegen completion.
- RISC-V consumer implementation.
- Rewriting x86 control-flow lowering broadly.
- Creating a new x86-specific prepared edge-copy authority parallel to shared
  prepared facts.
- Moving x86 emission details into shared prepare, BIR, or target-neutral
  helpers.
- Weakening supported-path tests or changing expectations to claim consumer
  progress.

## Acceptance Criteria

- At least one x86 lowering path consumes shared prepared edge-publication
  facts through the existing shared lookup authority.
- The implementation emits target-specific details from x86-owned code rather
  than shared prepare.
- Focused tests fail if the x86 path ignores shared edge-publication facts or
  invents an independent x86 edge-copy source of truth.
- Validation covers the new x86 consumer behavior and the relevant shared
  prepared lookup surface.
- The final handoff identifies whether the next follow-up should broaden x86
  coverage, unblock RISC-V readiness, or repair a discovered shared contract
  gap.

## Reviewer Reject Signals

- A patch matches one named testcase or fixture shape without adding a real x86
  consumer of shared `edge_publications`.
- A patch adds an x86-local edge-copy fact table or rediscovery pass instead of
  reading the shared prepared lookup authority.
- A patch moves x86 register names, scratch policy, clobber sequencing, stack
  syntax, move spelling, or assembly formatting into shared prepare or BIR.
- A patch downgrades supported-path expectations, marks the targeted path
  unsupported, or weakens tests to make the slice pass.
- A patch claims progress only through helper renames, wrapper churn, or
  classification changes while the x86 lowerer still does not consume shared
  prepared edge-publication facts.
- A patch broadly rewrites unrelated x86/AArch64/RISC-V lowering instead of
  landing the smallest auditable x86 consumer slice.
