# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Enumerate identity-authority seams

## Just Finished

- Plan Step 1 established four reproducible, module-transactional identity
  baselines at clean HEAD `a80243c0b`:
  - `global_store.c` first fails at `LirStoreOp` (`store i32 7, ptr
    @g_counter`): typed `LirTypeRef(i32)`, but text-only Immediate `7` and
    Global `@g_counter`; no typed immediate payload or `LinkNameId`.
  - `defined_pointer_global_pointer.c` first fails at `LirLoadOp` (`%t0 = load
    ptr, ptr @gpp`): typed `LirTypeRef(ptr)`, but text-only SsaValue result and
    Global pointer; no result `LirValueId` or global `LinkNameId`. The producer
    is the global-rvalue route in `hir_to_lir/expr/coordinator.cpp`.
  - `defined_global_array.c` first fails at `LirCastOp` (`%t0 = sext i32 1 to
    i64`), not its later GEP: cast kind and source/destination `LirTypeRef`s are
    typed, but result and Immediate operand are text-only. Later `LirGepOp`
    also has text-only result/pointer identity and raw combined index fragments.
  - `riscv64_zero_aggregate_global_storage.c` has no ordinary instructions in
    `main` and first fails at `LirRet` (`ret i32 0`): only
    `value_str="0"` and `type_str="i32"`, with no typed immediate/value identity.
- Global load, store, and cast producer paths were corroborated in
  `expr/coordinator.cpp`, `lvalue.cpp`, and typed `LirCastOp` helpers.

## Suggested Next

- Produce the exhaustive checked `LirInst` / `LirTerminator` authority matrix.
  Give every operand, result, immediate, and symbol row its authority class,
  producer, verifier, focused probe, blocked 734 consumer, and disposition.

## Watchouts

- Step 2 is inventory-only: do not edit LIR schema, producers, verifier, new
  BIR, or importer code before the exhaustive matrix and probe bindings exist.
- Do not parse or match operand text, `value_str`, `type_str`, symbol spelling,
  printer output, or testcase identity.
- Preserve the moving-first-fact result: the array case reaches `LirCastOp`
  before `LirGepOp`; do not label GEP as its first failure.

## Proof

- Fresh backend proof at HEAD `a80243c0b` is 4/4; accepted full baseline is
  3033/3033.
- All four focused commands fail before publication; no partial Raw or
  Canonical module escapes.
