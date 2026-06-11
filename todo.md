Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Schema/Index Tests

# Current Packet

## Just Finished

Step 4 completed. Added focused Route 8 backend/BIR schema and index coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, plus a narrow
Route 8 key-matching fix in `src/backend/bir/bir.cpp`.

The new test covers positive same-block return-chain lookups for terminal return
identity and immediate named next-operand identity through both function and
block indexes. It also covers unsupported opcode, unnamed chain and terminal
values, broken walks, non-return terminators, cross-block relationships, missing
instruction keys, and duplicate conflicting records failing closed without
comparing against prepared helpers. Positive lookups now build query keys from
copied equivalent named BIR values to prove Route 8 matches stable `name_id` or
`name`+`type` before falling back to value pointer identity.

## Suggested Next

Execute Step 5 by validating the Route 8 slice handoff: confirm the focused
proof remains green, prepared return-chain helpers and AArch64 consumers remain
unchanged, and identify idea 178 as the next oracle-equivalence prerequisite.

## Watchouts

- Keep the route target-neutral: no homes, registers, ABI placement, scratch
  policy, alias checks, ALU record construction, or emission order.
- Do not migrate AArch64 consumers or hide prepared return-chain helpers in this
  idea.
- The Route 8 builder currently mirrors the prepared scalar binary opcode set
  and requires named chain and terminal values. Optional next-operand identity is
  published only when the immediate next operand is named.
- Conflicting duplicate return-chain publications mark the generated record
  unavailable with `DuplicateRecord`; manual duplicate records in an index still
  cause `route8_find_return_chain_record` to fail closed.
- Route 8 remains distinct from Route 1 producer identity and Route 7 comparison
  provenance; it reuses `Route1SourceValueIdentity` for values, not the Route 1
  index as the route itself.
- Keep oracle equivalence against prepared helpers for idea 178.
- Function/block key stability is owner-local: rebuild the Route 8 index after
  mutating indexed BIR functions, blocks, instructions, or value names.
- Route 8 lookup keys intentionally prefer stable named value identity over
  raw `Value*` identity so copied/equivalent named BIR values can query indexed
  records.

## Proof

Ran the delegated proof command:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Result: passed. The `backend_prepared_lookup_helper_test` target built and CTest
reported `backend_prepared_lookup_helper` passed. Proof log: `test_after.log`.
