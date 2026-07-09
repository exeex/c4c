Status: Active
Source Idea Path: ideas/open/656_20000722_local_memory_access_object_route.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Object-Route Local-Memory Evidence

# Current Packet

## Just Finished

Lifecycle repair after idea 648 Step 3 focused coverage passed without
implementation changes.

Idea 648 is parked because its focused RV64 text-route coverage already proves
`arg.source_selection=local_frame_address_materialization` emits a selected
frame-slot address setup, while the representative `src/20000722-1.c` object
route now stops earlier at `unsupported_local_memory_access`.

The fresh object-route blocker is split into active idea 656.

## Suggested Next

Execute Step 1 for idea 656: refresh focused diagnostics for
`src/20000722-1.c`, name the first `unsupported_local_memory_access` owner, and
record the exact lowering or object-emission boundary to inspect next.

## Watchouts

- Do not change idea 648 call-argument materialization in this route.
- Do not rely on the historical `mv a0,s2` disassembly as current evidence.
- Do not broaden string-constant local-memory policy or
  `StringConstantLabelPointer` admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- If diagnostics reach a renewed call-argument mismatch after this blocker,
  hand that back to idea 648 instead of fixing it here.

## Proof

Lifecycle-only repair. No build or ctest proof required.
