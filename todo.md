Status: Active
Source Idea Path: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Inventory And Follow-Up Decision

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: recorded the closure inventory for selected
pointer-value memory-use freshness authority without broadening into adjacent
freshness families or target migrations.

Audited producers and consumers:

- Producers: BIR/lowering publication of pointer-value memory addresses,
  `PreparedMemoryAccess` records with `PreparedAddressBaseKind::PointerValue`,
  pointer-value provenance/base identity, prepared access lookup, generic
  selected-freshness lookup vocabulary, and support checks around
  `prepared_pointer_value_memory_has_proven_authority(...)`.
- Consumers: AArch64 prepared memory operand formation for pointer-value
  loads/stores, RV64 pointer-value memory emission enough to classify it as a
  deferred target migration, and adjacent stack/layout/publication surfaces
  only enough to separate address legality from pointer freshness.

Selected authority dimensions:

- Authority is selected by `PreparedValueFreshnessUseKind::PointerValueMemoryUse`,
  `PreparedValueFreshnessSourceKind::PointerValueMemoryAccess`,
  `PreparedValueFreshnessProofKind::PointerValueMemoryAuthority`, and
  `PreparedValueFreshnessSourceRank::PointerValueMemory`.
- It names the exact pointer value id/name used as the memory address base,
  the load/store memory use mode, function/block/instruction program point,
  offset/range coordinate, provenance/base identity, layout authority, target
  memory operand shape, and required support facts.
- Loaded value ids, store source ids, pointer arithmetic results, semantic GEP
  targets, and global symbol identities are not substitutes for this authority.

Required but insufficient support facts:

- Pointer address base kind, pointer value id/name presence, object extent,
  offset/range proof, provenance identity, layout authority/local layout,
  target offset encodability, final target memory operand shape, diagnostics,
  and prepared dumps are required route support where applicable but do not
  prove pointer-value freshness alone.

Migrated representative consumer:

- AArch64 prepared pointer-value memory operand formation now requires selected
  pointer-value memory freshness before accepting the pointer value as current
  for the representative prepared load/store memory use.

Fail-closed coverage:

- Missing/no candidate, ambiguous authority, stale function/block/instruction
  point, wrong pointer id/name, missing pointer identity, wrong load/store use,
  wrong use/source/proof/rank vocabulary, range/offset-only, provenance-only,
  layout-only, support-only, and target-shape-only cases fail closed.

Separate follow-ups:

- Loaded-value freshness, store-source freshness, pointer arithmetic selected
  authority, semantic GEP target consumption, global symbol memory freshness,
  RV64/x86/string or broad target migration, MIR view design, ABI lowering, and
  final assembly expectation coverage remain separate initiatives.

## Suggested Next

Ask the plan owner for the closure decision for
`ideas/open/600_pointer_value_memory_use_freshness_authority.md`.

## Watchouts

- This closure inventory is todo-only; `plan.md` and the source idea remain
  unchanged for the plan-owner closure pass.
- Do not treat the adjacent follow-ups as hidden remaining work for this
  runbook unless the plan owner chooses to split or continue the lifecycle.

## Proof

No tests rerun for this todo-only Step 5 packet.

Proof already available for this run:

- Step 3 backend subset passed 346/346 in `test_after.log`.
- Step 4 focused contract proof passed:
  `ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_memory_operand_contract$'`
- Step 4 broad selected regex passed 85/85 in `test_after.log`:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64|mir|bir|.*memory|.*freshness|.*prepared|.*contract)') > test_after.log 2>&1`
- Baseline candidates were accepted twice with 3375/3375 non-regressive.

`git diff --check` passed.
