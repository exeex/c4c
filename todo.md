# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.5
Current Step Title: Migrate Atomics And Intrinsics Lowering Families
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed step 2.5's seam-private helper follow-up by localizing the
atomics/intrinsics-only SSE and float helper routines inside
`lowering/atomics_intrinsics_lowering.cpp` and removing their declarations
from the transitional `X86Codegen` surface in `x86_codegen.hpp`. The canonical
lowering owner still carries the live atomic load/store/RMW/cmpxchg/fence and
explicit intrinsic lowering entrypoints; only helper leakage through the shared
header was trimmed.

## Suggested Next

Finish auditing step 2.5 for any remaining atomics/intrinsics declarations or
includes that still live on the transitional shared x86 codegen surface even
though they are now owned solely by the canonical lowering seam.

## Watchouts

- `atomics.cpp` and `intrinsics.cpp` are intentionally dormant compatibility
  surfaces now; do not repopulate them just to preserve the legacy bucket.
- Keep explicit ISA lowering in the canonical lowering stack; do not leak this
  family into prepared dispatch or module emission helpers.
- Preserve the reviewed combined seam shape for now instead of splitting
  atomics and intrinsics into separate ownership buckets mid-step.
- The active lowering entrypoints still remain declared on `X86Codegen`; only
  helpers proven seam-private to `atomics_intrinsics_lowering.cpp` were
  localized in this packet.

## Proof

Step 2.5 atomics/intrinsics seam-private helper localization on 2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
