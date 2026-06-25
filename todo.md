Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Declare RV64I Corpus And Canonical Contract

# Current Packet

## Just Finished

Step 1 - Declare RV64I Corpus And Canonical Contract: added a committed
`tests/backend/rv64/rv64i_ev64_roundtrip_corpus.s` corpus for RV64I plus c4c
EV64 `.insn.d`, documented the canonical roundtrip contract in
`tests/backend/rv64/rv64i_ev64_roundtrip_contract.md`, and registered
`backend_rv64_roundtrip_contract` CTest scaffolding for the intended
`input.s -> pass1.o -> pass1.s -> pass2.o -> pass2.s -> pass3.o` route.

## Suggested Next

Execute Step 2 from `plan.md`: begin generalizing c4c-as parsing/encoding from
the current minimal subset toward the committed RV64I plus EV64 corpus.

## Watchouts

- Current unsupported gaps are intentionally recorded in
  `tests/backend/rv64/rv64i_ev64_roundtrip_contract.md`: c4c-as does not yet
  parse the full RV64I mnemonic, immediate, label/fixup, branch, jump, and
  PC-relative surface; c4c-objdump does not yet decode full RV64I.
- `backend_rv64_roundtrip_contract` currently passes by requiring the broad
  corpus to fail closed at pass1 without writing an object. When assembler
  support expands, the same script is already structured to continue through
  objdump/as/objdump/as and assert text/object stability.
- Keep unsupported extensions and directives fail-closed; do not use external
  assembler or objdump output as the source of truth.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
