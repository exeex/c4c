Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Build Function-Local Index and Lookup Helpers

# Current Packet

## Just Finished

Step 3 completed. Implemented the Route 8 return-chain index builders in
`src/backend/bir/bir.cpp` for function and block owner scopes. Builders now walk
same-block named scalar `BinaryInst` chains that reach a named return terminator,
publish accepted records keyed by function/block/instruction index plus chain
value identity, and preserve the optional immediate next-operand identity when
that operand is named.

Lookup helpers now operate over populated indexes: positive record lookups return
terminal return identity and optional next operand identity, while unsupported
opcodes, unnamed chain or terminal values, broken walks, non-return terminators,
cross-block relationships, missing instruction keys, and conflicting duplicate
publications fail closed without selecting a winner.

## Suggested Next

Execute Step 4 by adding focused backend/BIR schema and index tests for Route 8
positive terminal and next-operand lookups plus negative unsupported, unnamed,
broken-walk, non-return, cross-block, and duplicate-conflict cases.

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

## Proof

Ran the delegated proof command:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Result: passed. The `backend_prepared_lookup_helper_test` target built and CTest
reported `backend_prepared_lookup_helper` passed. Proof log: `test_after.log`.
