Status: Active
Source Idea Path: ideas/open/118_lir_bir_legacy_type_text_removal.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Legacy Text Classification And Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 6 by recording the final legacy text classification
and handoff state for active idea 118.

Accepted removals now recorded for this runbook:
- Step 3: structured aggregate sret calls no longer populate
  `CallInst::return_type_name` when `structured_return_type_name` exists.
- Step 2: `lookup_backend_aggregate_type_layout` can use valid named
  structured layout metadata when no legacy type declaration body exists, while
  keeping the parity-mismatch fallback.

Structured-only proof is preserved from the focused code slices and the later
broader backend checkpoint: the structured sret and named structured layout
paths were exercised without relying on the removed legacy text authority.

## Suggested Next

Supervisor should call `c4c-plan-owner` for the close/continue decision on
active idea 118. No executor-created source idea is needed from the current
inventory; handoff recommendations are recorded below for plan-owner judgment.

## Watchouts

- The broad backend proof covered runnable `backend_` tests only; 12 disabled
  MIR-focus tests did not run.
- Keep separating final spelling/output data from lowering authority. Some
  legacy strings are expected to remain as output spellings even when they are
  no longer acceptable as semantic type authority.
- Do not open a new idea automatically from this executor packet. If plan-owner
  continues work, the recommended split is type-ref-authority replacement for
  raw `LirTypeRef` text consumers, not a Step 6 todo-only change.

## Remaining Legacy Surface Inventory

Final spelling/output data:
- Raw `LirTypeRef` text remains final spelling/output data for LIR printing and
  final LLVM emission where the backend still needs exact textual type
  spellings.
- Function `signature_text`, extern `return_type_str`, and global `llvm_type`
  strings remain final declaration, ABI, global storage, initializer, and
  emission spellings.
- `CallInst::return_type_name` remains final spelling/output data for
  scalar/pointer returns, runtime intrinsics, inline asm, prepared-BIR/manual
  calls, calls without structured return context, and structured aggregate sret
  calls that still lack `structured_return_type_name`.

Type-ref-authority work:
- Raw `LirTypeRef` text remains blocked type-ref authority for call return
  parsing, typed call args, GEP/load/store/cast/phi/select type strings, and
  aggregate field text. This is the clearest follow-on family if the active
  idea continues or a successor idea is opened.
- Function `signature_text`, extern `return_type_str`, and global `llvm_type`
  strings still act as active lowering authority in some declaration, ABI,
  global, initializer, and emission paths; separating their final spelling role
  from semantic type authority is remaining work.

Planned-rebuild residue:
- `LirModule::type_decls` and `TypeDeclMap` remain layout/lowering fallback
  residue for unnamed aggregate text, named types without valid structured
  layout metadata, invalid structured layouts, and proof-only structured-layout
  parity notes.

Explicitly deferred compatibility:
- Named structured types with legacy parity mismatches intentionally keep the
  legacy fallback rather than trusting mismatched structured metadata.
- Structured aggregate sret calls that lack `structured_return_type_name`
  intentionally keep the old `CallInst::return_type_name` path until their
  producers supply structured return context.

## Proof

Supervisor-run validation checkpoint:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`;
107/107 runnable backend tests passed, and 12 disabled MIR-focus tests did not
run. Proof log: `test_after.log`.
