Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Atomic Authority And Fail-Closed Gaps

# Current Packet

## Just Finished

Step 1 `Inventory Atomic Authority And Fail-Closed Gaps` completed the first
backend inventory and added a focused fail-closed memory test.

Current atomic authority inventory:
- `docs/backend/x86_codegen_legacy/atomics.cpp.md` and
  `src/backend/mir/aarch64/codegen/atomics.md` preserve legacy atomic intent:
  ordering, width, pointer/value operands, RMW operation, compare-exchange
  success/failure ordering, result mode, exclusive loops, signed narrow-load
  normalization, and fences.
- Current BIR has only ordinary `LoadLocalInst`, `LoadGlobalInst`,
  `StoreLocalInst`, and `StoreGlobalInst` memory instructions. Their structured
  memory facts are base, offset, size, alignment, address space, and
  `is_volatile`; there is no BIR owner for atomic ordering, fences, RMW,
  compare-exchange, result mode, or failure ordering.
- Prepared addressing preserves ordinary memory facts in
  `PreparedMemoryAccess`; it carries address-space and volatility only, not
  atomic operation facts.
- AArch64 selected memory nodes consume `PreparedMemoryAccess` and emit plain
  `Load`/`Store` machine nodes with optional `VolatileMemoryAccess` side
  effect. There are no atomic/fence/RMW/CAS selected records yet.
- Prepared printer surfaces do not currently dump memory-operation authority
  beyond existing prepared call/address/value facts, so printer text is not an
  atomic source of truth.

Added test coverage:
- `volatile_memory_does_not_fabricate_atomic_authority` in
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  proves a volatile prepared load remains ordinary volatile memory, not
  inferred atomic authority.
- The same test proves missing structured BIR memory facts fail closed with
  `AddressFactMismatch` instead of accepting prepared volatility as enough
  authority.

## Suggested Next

Begin Step 2 `Define Structured Atomic Operation Carriers` by adding the
smallest target-neutral BIR/prepared atomic carrier boundary:
`AtomicOperation` facts attached before AArch64 selection, with kind
`load/store/fence/rmw/compare_exchange`, ordering, width/type, pointer operand,
value operand where required, result mode, RMW opcode, compare expected/desired
operands, and compare-exchange failure ordering.

The first implementation packet should define the data shapes and printer/debug
dump for complete carrier facts only; AArch64 instruction selection should wait
until those facts are preserved in tests.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Keep unsupported or partial atomic facts fail-closed; a carrier missing
  ordering, width/type, pointer/value, result mode, or compare-exchange failure
  ordering must diagnose before selection.
- Preserve ordinary volatile memory behavior separately from atomic behavior.
- RMW and compare-exchange must preserve old-value result semantics; compare
  exchange also needs explicit boolean-vs-old-value result mode and failure
  ordering.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green.

Proof log: `test_after.log`.
