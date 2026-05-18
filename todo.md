# AArch64 Stack Frame SP Alignment Todo

Status: Active
Source Idea Path: ideas/open/288_aarch64_stack_frame_sp_alignment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Minimal SP Alignment Failure

# Current Packet

## Just Finished

The umbrella inventory completed Step 3/4 lifecycle routing and switched to
the focused AArch64 stack-frame/SP 16-byte alignment idea.

Source evidence from the inventory:
- Refreshed scan: 212 total, 80 passed, 129 failed non-timeout, 3 timed out.
- Runtime `Bus error` cluster: 28 cases.
- Minimal representative: `src/00004.c`.
- Nearby local pointer/array representatives: `src/00005.c`, `src/00013.c`,
  `src/00014.c`, `src/00015.c`, and `src/00016.c`.
- Generated `00004.c.s` includes `sub sp, sp, #24`, which leaves `sp`
  misaligned under the AArch64 16-byte stack-pointer alignment rule.

## Suggested Next

Supervisor should delegate Step 1 to establish the minimal `src/00004.c`
failure shape, inspect the generated prologue/epilogue and local stack-slot
offsets, and capture the baseline proof/log for the owned SP/frame-alignment
surface.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact frame-size shortcuts, local-variable
  spelling shortcuts, and emitted-instruction-text matching.
- Preserve local-slot load/store correctness when adding frame padding.
- Separate remaining bus-error cases that prove to be broader pointer, array,
  struct, function-pointer, or local-state owners.
- Keep timeout-sensitive cases `00132.c`, `00173.c`, and `00220.c` out of this
  route unless a separate timeout-specific idea is created.
- Keep compare/branch printer/lowering and other compile-stage backend gaps
  out of this route.

## Proof

Lifecycle split/switch only. No build or test proof was run.
