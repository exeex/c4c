# x86/RISC-V Prepared Edge Publication Handoff

## Goal

Validate that the shared prepared edge-publication authority introduced for
AArch64 is target-neutral enough for x86 and RISC-V consumers, and add the
smallest useful handoff path for future non-AArch64 lowering.

## Why This Exists

Idea `16_bir_edge_value_flow_authority.md` moved covered CFG edge/block-entry
value-flow authority into shared prepared facts so AArch64 no longer needs to
rediscover semantic edge producers locally. That work is only fully valuable if
future x86 and RISC-V lowering can consume the same prepared facts instead of
rebuilding their own target-local edge-copy reasoning.

Idea `18_aarch64_cts_00181_runtime_regression_reopen.md` also showed that
edge-publication consumers still need target-local register-hazard discipline.
The shared handoff should therefore prove the boundary: shared facts describe
what value must flow across an edge, while each target owns how to emit it
without clobbering live registers or target-specific scratch state.

This idea is a handoff/audit slice, not a full x86 or RISC-V backend rewrite.

## In Scope

- Audit the shared prepared edge-publication records and helpers from idea 16
  for hidden AArch64 assumptions.
- Audit current x86 and RISC-V lowering surfaces to identify where edge or
  block-entry value publication will need the shared prepared facts.
- Define the minimal consumer API shape expected by x86 and RISC-V.
- Add one narrow x86 or RISC-V consumer proof if the target surface is ready,
  or add a precise readiness note if the target lacks required lowerer
  infrastructure.
- Add tests or verifier coverage that proves the shared prepared facts are
  target-neutral and do not encode AArch64 register names, scratch policy, or
  assembly spelling.
- Document any target-local responsibilities that must remain outside shared
  prepare, especially scratch selection and clobber avoidance.

## Out Of Scope

- Implementing full x86 or RISC-V codegen.
- Rewriting x86 or RISC-V control-flow lowering broadly.
- Moving target-specific emission, register names, scratch-register policy, or
  assembly syntax into shared prepare or BIR.
- Further AArch64 dispatch slimming unless needed to expose the shared
  contract.
- Reopening the `00181` runtime repair except as a hazard lesson for consumer
  boundaries.

## Acceptance Criteria

- The shared prepared edge-publication contract is audited from the perspective
  of x86 and RISC-V consumers.
- Any AArch64-only assumptions in the shared facts are removed, documented as
  target-local, or explicitly rejected as out of scope.
- x86 and RISC-V have a clear minimal handoff path for consuming prepared
  edge-publication facts.
- At least one focused proof exists: either a working minimal non-AArch64
  consumer slice or a targeted readiness test/report that identifies the exact
  missing target surface.
- The final close note states whether the next step should be an x86 consumer
  implementation, a RISC-V consumer implementation, or another shared prepared
  contract repair.

## Reviewer Reject Signals

- A patch treats AArch64 success as proof that the shared contract is
  target-neutral.
- A patch moves target emission details into shared prepare or BIR.
- A patch creates a second x86/RISC-V-specific edge-copy authority instead of
  consuming shared prepared facts.
- A patch expands into broad backend rewrite without a minimal handoff proof.
- A patch ignores the register-clobber hazard lesson from idea 18.
