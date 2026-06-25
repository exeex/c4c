Status: Active
Source Idea Path: ideas/open/351_c4c_objdump_rv64_custom_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: RV64 Instruction Decoder

# Current Packet

## Just Finished

Step 2 from `plan.md` is implemented: `c4c-objdump` now decodes extracted
RV64 `.text` bytes into a structured report for the initial supported
instruction subset. The decoder reconstructs EV64 `.insn.d` fields for major
10, decodes the currently emitted `li a0, 0` shape, decodes `ret`, and fails
closed before writing output when unsupported instruction bytes are present.
Focused coverage proves the canonical object reports `.insn.d`, `li`, and
`ret`, and a patched valid RV64 object with an unsupported final word is
rejected.

## Suggested Next

Execute Step 3 from `plan.md`: replace the decoder report comments with the
canonical assembly printer accepted by `c4c-as`, then prove
`a.o -> a.s -> b.o` preserves the canonical `.text` bytes.

## Watchouts

- Do not hard-code the full canonical byte string as disassembly.
- Do not use external objdump/as output as the semantic source of truth.
- Do not touch `ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md`.
- Step 2 output intentionally remains an extraction/decoder report in comments,
  not final `c4c-as`-compatible assembly. Step 3 should be the first packet
  that emits canonical assembly instructions.
- The current `.insn.d` discriminator is the supported EV64 major field `10`;
  the operation, vector operands, and dtype are decoded from their fields.
- The app-local ELF reader exists because the translated shared
  `linker_common` C++ parser is referenced by backend stubs but not currently
  available as a buildable module.

## Proof

Supervisor-selected proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(c4c_objdump_|c4c_as_|cli_riscv64_vrm_insn_d_source_obj$)' | tee test_after.log
```

Result: passed, 3/3 tests. Proof log: `test_after.log`.
