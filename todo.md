Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate and Hand Off

# Current Packet

## Just Finished

Step 5 completed. Validated the Route 8 schema/index slice and left it ready for
handoff without further implementation expansion.

The delegated focused proof is green. Boundary checks confirm the Route 8 slice
since activation touches only `src/backend/bir/bir.*`,
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and `todo.md`; it
does not modify `src/backend/prealloc/prepared_lookups.*` or
`src/backend/mir/aarch64/codegen/alu.cpp`. Prepared return-chain helpers remain
public, and AArch64 consumers were not migrated.

## Suggested Next

Hand off to lifecycle/supervisor review. The next lifecycle prerequisite is
`ideas/open/178_bir_return_chain_oracle_equivalence.md`, which should compare
Route 8 answers against the public prepared return-chain helper oracle before
any consumer migration or prepared API contraction.

## Watchouts

- Keep the route target-neutral: no homes, registers, ABI placement, scratch
  policy, alias checks, ALU record construction, or emission order.
- Do not migrate AArch64 consumers or hide prepared return-chain helpers in this
  idea; Step 5 confirmed this slice did neither.
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
reported `backend_prepared_lookup_helper` passed, with 1/1 test passing. Proof
log: `test_after.log`.
