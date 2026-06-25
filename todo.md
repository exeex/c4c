Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Generalize RV64 Objdump Decoding And Canonical Printing

# Current Packet

## Just Finished

Step 4 - Generalize RV64 Objdump Decoding And Canonical Printing: completed a
bounded non-control RV64I decode/print slice for `c4c-objdump`. The app now
field-decodes and canonically prints U-type `lui`/`auipc`, I-type
arithmetic/logical, RV64/RV64W shift immediates, R-type and word R-type
arithmetic/logical, loads, stores, and label-free non-`ret` `jalr`, while
preserving the existing EV64 `.insn.d`, `li`, and `ret` canonical output.
Focused objdump tests now assemble representative source with `c4c-as`, verify
the dumped canonical text, reassemble it, and prove text/object stability for
that subset. Branch and `jal` bytes remain fail-closed in objdump.

## Suggested Next

Continue Step 4 with a bounded control-flow objdump slice: decode local
B-type branches and `jal` into stable canonical labels or documented numeric
PC-relative form, then advance the broad roundtrip contract beyond its current
dump1 fail-closed boundary if the printed form can be reassembled truthfully.

## Watchouts

- c4c-objdump still intentionally rejects B-type branch and J-type `jal`
  instruction bytes, so `backend_rv64_roundtrip_contract` still passes by
  accepting the honest dump1 fail-closed boundary with no partial dumped text.
- Generic `jalr` is decoded, but the existing `jalr zero, 0(ra)` pseudo remains
  canonicalized as `ret`; tests cover a distinct `jalr t0, 4(ra)` form for the
  generic path.
- External relocation/fixup semantics are still unsupported; do not print
  symbolic targets unless the object truthfully carries the information needed
  to reassemble them.

## Proof

Passed:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(c4c_as|c4c_objdump|cli_riscv64|rv64)'
```

Proof log: `test_after.log`.
