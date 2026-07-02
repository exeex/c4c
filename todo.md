# Current Packet

Status: Active
Source Idea Path: ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Or Share Pure Helpers

## Just Finished

Completed Step 2 - Extract Or Share Pure Helpers for
`ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md`.

Implemented the shared low-level RV64 helper surface in
`rv64_line_assembler.*`:

- `rv64_encode_u_type`
- `rv64_encode_i_type`
- `rv64_encode_s_type`
- `rv64_encode_r_type`
- `rv64_encode_b_type`
- `rv64_encode_j_type`
- `rv64_append_le32`
- `rv64_append_le64`

The shared API takes only integer fields and byte vectors. It does not mention
`RiscvEncodedFragment`, `Rv64AsmLine`, labels, fixups, prepared BIR, object
modules, parser records, or ELF/module concepts.

Kept `object_emission.cpp` object-route wrappers named `encode_*` and
`append_le*`, but replaced their duplicate bit-packing and byte-append bodies
with delegates to the shared `rv64_*` helpers. All fragment, label, fixup,
symbol, relocation, prepared object, module assembly, ELF, parser, and public
object-route code stayed in its existing owner.

Preserved byte order and immediate masking behavior, including the object-route
`std::uint32_t imm20` U-type wrapper and the line-assembler U-type call path.

## Suggested Next

Execute Step 3 - Probe Remaining Direct Cleanup. Check whether any private
object-route wrappers can be removed without blurring ownership. Keep wrappers
if they protect object-route readability or avoid broad call-site churn.

## Watchouts

- Step 3 should not route object emission through `parse_rv64_asm_line` or
  `encode_rv64_asm_line`.
- Do not move `append_fragment`, `append_rv64_fragment`, symbol/fixup mapping,
  label binding, object module assembly, ELF writing, prepared data-object
  emission, or public object-route entrypoints.
- The current `rv64_line_assembler.hpp` helper declarations are
  object-independent, but the header also contains parser line variants. If a
  future packet needs a cleaner standalone helper header, that is a separate
  ownership decision.
- `clang-format` is not installed in this environment; formatting was kept
  consistent manually in touched regions.

## Proof

Delegated proof command:

```text
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 4/4 tests passing:

- `backend_riscv_object_emission`
- `backend_rv64_roundtrip_contract`
- `backend_cli_riscv64_return_zero_writes_elf_obj`
- `backend_cli_riscv64_vrm_insn_d_source_obj`
