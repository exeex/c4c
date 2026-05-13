Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Document Memory Operand Contract And Close Readiness

# Current Packet

## Just Finished

Completed Step 6 from `plan.md`: documented the AArch64 memory operand record
contract and added close-readiness proof.

Concrete work completed:
- Updated `records.md` with the supported memory operand bases, preserved
  prepared/BIR facts, volatility and address-space contract, fail-closed
  conversion behavior, and deferred load/store lowering boundary.
- Updated `memory.md` to point current memory record work at `records.md` and
  keep the file as a legacy lowering reference rather than active selection or
  emission ownership.
- Added `backend_aarch64_memory_operand_contract` to prove supported memory
  base vocabulary, structured identities, volatility/address-space
  preservation, explicit unsupported/fail-closed diagnostics, and record-only
  memory instruction ownership.
- Verified the memory operand contract does not own branch/scalar/call/return,
  assembler, object, load/store selection, assembly, or object behavior.

## Suggested Next

Hand off to the plan owner to review close readiness for
`ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md`.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- This Step 6 packet is docs/test only. Implementation files were not changed.
- Close-readiness review should compare the completed runbook against the
  source idea and decide whether to close, extend, or split follow-up work.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with
`backend_aarch64_memory_operand_contract` included and green: 131 tests
passed, 0 failed; 12 disabled MIR trace tests were not run. Proof log path:
`test_after.log`.
