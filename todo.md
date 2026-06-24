Status: Active
Source Idea Path: ideas/open/331_rv64_minimal_relocatable_elf_object_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect RV64 Emission And Object API Seams

# Current Packet

## Just Finished

Step 1 inspection completed for the RV64 direct object-emission seam.

Chosen seam: add an RV64 target-owned object path beside, not inside,
`emit_prepared_module_text()`. The first implementation should introduce
RV64-local encoded fragment/fixup records under `src/backend/mir/riscv/codegen/`
and a new API such as `emit_prepared_module_object(...)` that walks the same
prepared module/function surfaces as `emit_prepared_module_text(...)`, lowers
only the selected supported subset into `mir::object::ObjectModule`, and then
passes RV64 ELF config to the shared relocatable ELF writer. The shared object
writer should only receive numeric relocation types after RV64 has chosen them.

Do not fork the existing asm writer or parse its printed `.s` output. Keep
`append_simple_prepared_bir_function_asm(...)`, `append_prepared_global_storage_asm(...)`,
and existing route tests as independent asm coverage.

First minimal smoke subset for the implementation slice:

- one `.text` section with a global RV64 function symbol and basic prologue-free
  scalar return bytes
- one same-module or undefined direct-call case encoded as the normal RV64
  `auipc` + `jalr` call pair
- no `.data`, `.bss`, `.rodata`, branch-heavy control flow, or global address
  materialization in the first slice

Required relocation kinds for that first slice:

- target enum value for `CallPlt`, lowered to ELF relocation number
  `R_RISCV_CALL_PLT` (`19`)
- a synthetic AUIPC-site label only if the first call representation is split
  into explicit high/low records; otherwise keep call-pair relocation atomic for
  Step 2 and defer explicit `pcrel_lo` pairing to the global-address slice

The inspected follow-up relocation set needed after the first slice is:
`PcrelHi20` (`R_RISCV_PCREL_HI20`), `PcrelLo12I`
(`R_RISCV_PCREL_LO12_I`), `PcrelLo12S` (`R_RISCV_PCREL_LO12_S`), and local
`Branch`/`Jal` handling for resolved intra-section labels. Those are required
for `lla`, load/store global access, and fused-branch control-flow coverage, but
should not be pulled into the first minimal implementation unless the target API
needs the enum scaffolding.

## Suggested Next

Implement the RV64-local encoded fragment/fixup records and first object-module
construction helper for the minimal text + direct-call subset. Add a focused
`tests/backend/mir/backend_riscv_object_emission_test.cpp` and CMake registration
that proves the helper emits `.text`, a function symbol, an undefined callee
symbol, `R_RISCV_CALL_PLT`, and RV64 ELF config (`EM_RISCV` machine 243, current
flags 0x5) through the shared writer.

Focused proof command for that implementation slice:

```sh
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_object_model_records)$' --output-on-failure) > test_after.log 2>&1
```

## Watchouts

- Do not route RV64 object output through printed assembly text.
- Keep RV64 relocation kinds and `pcrel_hi` / `pcrel_lo` pairing target-owned.
- Preserve existing RV64 asm-route coverage beside new object-route proof.
- Do not expand into AArch64, CLI exposure, c-testsuite default switching, or
  textual assembler work in this child.
- Known unsupported object features after the first implementation slice:
  `.data`, `.bss`, `.rodata`, string constants, local branch relaxation or
  relocation promotion, global load/store address materialization, full
  `pcrel_hi`/`pcrel_lo` pairing, GOT/TLS relocations, linker relaxation hints,
  compressed instruction emission, CLI `--codegen obj`, and runtime/link smoke.
- The old assembler encoder has a useful relocation vocabulary, but the direct
  object route should not depend on parser operands or assembler state.

## Proof

Inspection-only packet; no build required by delegation.

Local validation: `git diff --check`.
