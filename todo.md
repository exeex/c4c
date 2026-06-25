Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Generalize RV64 Objdump Decoding And Canonical Printing

# Current Packet

## Just Finished

Step 4 - Generalize RV64 Objdump Decoding And Canonical Printing: completed a
bounded control-flow RV64I decode/print slice for `c4c-objdump`. The app now
retains local `.text` label symbols from supported objects, field-decodes
B-type `beq`/`bne`/`blt`/`bge`/`bltu`/`bgeu` and J-type `jal`, and prints
canonical label operands only when the target offset is backed by the global
entry label or a preserved local label. It emits preserved local labels at
instruction boundaries so dumped text can be reassembled, and fails closed for
control-flow targets or labels that are not truthful/reassemblable. Existing
non-control RV64I, EV64 `.insn.d`, `li`, and `ret` output is preserved.
Focused objdump tests now cover branch/`jal` text stability, object byte
stability, and an untruthful patched control-flow object. The broad RV64I plus
EV64 roundtrip contract now reaches full `input.s -> pass1.o -> pass1.s ->
pass2.o -> pass2.s -> pass3.o` stability for the committed corpus.

## Suggested Next

Continue Step 4 by deciding whether any remaining objdump canonicalization gaps
inside the current RV64I plus EV64 corpus need cleanup, or hand the runbook to
plan-owner/reviewer for Step 4 acceptance if the supervisor treats the full
corpus roundtrip as sufficient.

## Watchouts

- External relocation/fixup semantics remain unsupported and still fail closed
  through the existing `.text` relocation rejection; this slice only supports
  local same-file branch/`jal` targets backed by existing object labels.
- `jalr zero, 0(ra)` remains canonicalized as `ret`; generic non-`ret` `jalr`
  remains covered by the non-control Step 4 slice.
- `c4c-objdump` deliberately does not fabricate labels for arbitrary numeric
  PC-relative targets. If a target lacks a global/local symbol at that offset,
  dumping fails rather than printing unreassemblable text.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
