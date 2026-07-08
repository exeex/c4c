Status: Active
Source Idea Path: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Fail-Closed Behavior

# Current Packet

## Just Finished

Implemented Step 4 from `plan.md`: added focused AArch64 memory operand
contract coverage proving the migrated pointer-value memory-use freshness route
accepts only explicit selected authority and fails closed for support-only or
wrong-authority evidence.

Files changed:

- `tests/backend/mir/backend_aarch64_memory_operand_contract_test.cpp`
- `todo.md`

Covered cases:

- Exact selected pointer-value memory freshness is accepted.
- Missing/no candidate and ambiguous candidates fail closed.
- Missing pointer identity, stale function/block/instruction point, wrong
  pointer id/name, and wrong load/store use fail closed.
- Wrong use/source/proof/rank vocabulary fails closed.
- Offset/range, provenance identity, layout authority, and target memory
  operand shape mutations fail closed.
- Complete address support facts from
  `prepared_pointer_value_memory_has_proven_authority(...)` remain insufficient
  without selected pointer-value memory freshness.

## Suggested Next

Execute Step 5 from `plan.md`: record the closure inventory and follow-up
decision without broadening into adjacent freshness families or target
migrations.

## Watchouts

- Step 4 proves the shared helper and AArch64/prepared contract surface; it
  does not add final assembly expectation checks or route allowlist changes.
- Do not broaden this packet into loaded-value freshness, store-source
  freshness, pointer arithmetic, semantic GEP target consumption, global symbol
  memory freshness, or broad target migration.

## Proof

Focused proof passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_memory_operand_contract$'`

Required proof command was run and wrote `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64|mir|bir|.*memory|.*freshness|.*prepared|.*contract)') > test_after.log 2>&1`

Result: build completed; CTest passed 85/85.

`git diff --check` passed.
