Status: Active
Source Idea Path: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: CLI Contract And ELF Text Extraction

# Current Packet

## Just Finished

Step 1 from `plan.md` is implemented: `c4c-objdump <input.o> -o
<output.s>` now reads a real input object, validates the supported ELF64
little-endian RV64 relocatable envelope, extracts `.text`, finds a global
function symbol at `.text` start, and writes a non-disassembly extraction stub
with the symbol and text byte hex. Focused coverage proves the canonical RV64
object succeeds while malformed input and an unsupported AArch64 ELF fail
closed.

## Suggested Next

Execute Step 2 from `plan.md`: add the first RV64 instruction decoder for the
extracted `.text` bytes, including EV64 `.insn.d`, the emitted `li a0, 0`
shape, `ret`, and an unsupported-byte fail-closed case.

## Watchouts

- Do not hard-code the full canonical byte string as disassembly.
- Do not use external objdump/as output as the semantic source of truth.
- Do not touch `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`.
- Step 1 output intentionally contains only extraction comments, not assembly
  instructions accepted by `c4c-as`; Step 3 should replace that once decoding
  exists.
- The app-local ELF reader exists because the translated shared
  `linker_common` C++ parser is referenced by backend stubs but not currently
  available as a buildable module.

## Proof

Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_objdump_|c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed, 3/3 tests. Proof log: `test_after.log`.
