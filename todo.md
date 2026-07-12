# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.5
Current Step Title: Migrate memory consumers

## Just Finished

- Plan Step 2.5 removed target-local `PreparedValueHomeLookups` rebuilding from
  `memory.cpp`. Executable memory lowering now consumes the traversal-attached
  common value-home authority, fails closed when it is unavailable, and
  preserves existing memory instruction and ABI policy.

## Suggested Next

- Execute Plan Step 2.6 as a bounded packet against the remaining call and ABI
  materializers.

## Watchouts

- Step 2.5 required no test changes or common contract changes. Standalone
  prepared-record APIs retain their existing contract by querying prepared
  value locations directly, without constructing a target-local lookup index.
  Preserve the traversal-attached fail-closed boundary in executable consumers.

## Proof

- Passed the exact supervisor-selected proof: `cmake --build --preset default`
  followed by `ctest --test-dir build -j --output-on-failure -R
  '^backend_aarch64_(prepared_)?memory_operand_(records|contract)$'` (3/3).
  Canonical combined proof output is in `test_after.log`; the subset was
  sufficient for this bounded Step 2.5 consumer migration.
