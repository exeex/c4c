Status: Active
Source Idea Path: ideas/open/317_aarch64_variadic_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Va Start Helper Payloads

# Current Packet

## Just Finished

Lifecycle transition complete: idea 315 closed after Step 4 classified both
remaining `00204.c` residuals as outside large frame setup/teardown
materialization. Idea 317 is now active.

## Suggested Next

Execute Step 1 from `plan.md`: localize the AArch64 `va_start` helper payloads,
owning printer/lowering surface, representative tests, and smallest focused
proof command.

## Watchouts

- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not repair scalar ALU immediate materialization here; that is idea 318.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Preserve prepared variadic metadata guardrails while replacing raw helper
  text in generated assembly.

## Proof

Close-time regression guard for idea 315 used the focused proof scope recorded
before activation of this plan. Non-decreasing close mode passed with 10/11
tests passing before and after, and no new failing tests.
