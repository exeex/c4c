# Target-Neutral Publication Plan Record

Closure note: completed by introducing
`prepare::PreparedScalarPublicationPlan` /
`prepare::plan_prepared_scalar_publication`, adding focused neutral record
coverage, routing selected AArch64 scalar publication fallbacks through a
narrow target-local adapter, and adding the x86-labeled
`backend_x86_publication_plan_reuse` record-level proof. Remaining direct
AArch64 publication fallbacks are intentionally target-policy-adjacent
reload, immediate, pointer/global address, scratch, register-alias, and
instruction-emission cases rather than blockers for this plan-record idea.

## Intent

Introduce a target-neutral publication-plan record for scalar value publication
into Prepared homes while keeping target-local hooks responsible for actual
register views, memory operands, immediates, and final instructions.

## Why This Exists

AArch64 value materialization and publication code still performs broad
Prepared-home interpretation, entry-publication checks, clobber checks, and
publication-to-home decisions. Those decisions are reusable pipeline mechanics.
x86 and RISC-V need equivalent planning for stack homes, register homes,
globals, immediates, and pointer-base-plus-offset homes, but each target must
spell the actual instructions and ABI lanes differently.

This idea deliberately stops at the plan-record boundary to avoid moving target
emission policy too early.

## In Scope

- Define a publication intent record that describes what logical value must be
  published to which Prepared home.
- Move target-independent publication eligibility, clobber, and home-selection
  planning into prealloc or shared MIR ownership.
- Add AArch64 target hooks or adapters that lower the neutral plan to the
  existing AArch64 emission paths.
- Preserve current AArch64 behavior for stack homes, register homes, globals,
  immediates, and pointer-base-plus-offset homes.
- Include a small x86 or RISC-V consumer sketch, fixture, or adapter showing
  how another target would consume the plan record with different emission.

## Out Of Scope

- AArch64 instruction spelling, ABI lane policy, stack-pointer sequences,
  memory operand spelling, and final machine instruction construction.
- AArch64 ADRP/ADD sequences, load/store mnemonics, FP immediate materializing,
  register-width spelling, or target-specific clobber registers.
- Store-source recovery, call-boundary republication, branch fusion, dynamic
  stack operations, or final machine-record construction.
- Any target-neutral rule that assumes AArch64 addressing legality.

## Acceptance Criteria

- Publication planning data can be inspected before target emission and is not
  represented only as AArch64 instructions.
- AArch64 codegen consumes the plan through a narrow target-local lowering
  hook or adapter.
- Existing AArch64 publication behavior remains equivalent for the covered
  home classes.
- The plan record has an explicit x86 or RISC-V reuse path and does not depend
  on AArch64 namespaces for its core data model.

## Proof Expectations

- Build proof for affected C++ targets.
- Focused AArch64 tests or fixtures covering stack, register, global,
  immediate, and pointer-base-plus-offset publication paths touched by the
  migration.
- One x86 or RISC-V compile/unit fixture or adapter proving the plan record is
  reusable without adopting AArch64 emission spelling.

## Reviewer Reject Signals

- The patch moves AArch64 register spelling, ADRP/ADD construction, load/store
  mnemonics, FP immediate handling, or memory operand formatting into prealloc
  or shared MIR.
- Tests are weakened, expectations are downgraded, or a supported publication
  path is made unsupported without explicit user approval.
- The new plan record is a thin wrapper around an AArch64 instruction object or
  AArch64-specific operand enum.
- The implementation claims capability progress through helper renames,
  expectation rewrites, or classification-only changes while publication
  decisions remain target-local.
- The x86/RISC-V reuse story is absent or cannot compile without depending on
  AArch64 codegen headers.
