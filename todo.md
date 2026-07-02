# Current Packet

Status: Active
Source Idea Path: ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Probe Remaining Direct Cleanup

## Just Finished

Completed Step 3 - Probe Remaining Direct Cleanup for
`ideas/open/534_rv64_object_encoding_byte_helpers_cleanup.md`.

Probed the remaining private object-route wrappers in
`object_emission.cpp` after the shared helper extraction:

- `encode_u_type`
- `encode_i_type`
- `encode_s_type`
- `encode_r_type`
- `encode_b_type`
- `encode_j_type`
- `append_le32`
- `append_le64`

No further direct code cleanup was safe for this packet. Removing the wrappers
would force broad call-site churn across object-route emission and would reduce
the local object-route spelling/readability benefit without shrinking the
shared API. The wrappers also keep callers away from parser-looking
`parse_rv64_asm_line` / `encode_rv64_asm_line` surfaces while preserving the
object-route ownership boundary.

Left all code unchanged. Further extraction is parked unless a later source
idea creates a narrower standalone RV64 encoding helper header or accepts a
broad object-route call-site rename.

## Suggested Next

Execute Step 4 - Close Readiness Review. Review the current diff against the
source idea, confirm the remaining wrappers are an intentional boundary, and
decide whether closure is ready or a separate helper-header/call-site-rename
follow-up is needed.

## Watchouts

- Do not route object emission through `parse_rv64_asm_line` or
  `encode_rv64_asm_line`.
- Do not move `append_fragment`, `append_rv64_fragment`, symbol/fixup mapping,
  label binding, object module assembly, ELF writing, prepared data-object
  emission, or public object-route entrypoints.
- The retained wrappers are deliberate compatibility/readability shims, not
  duplicate bit-packing implementations.
- The current `rv64_line_assembler.hpp` helper declarations are
  object-independent, but the header also contains parser line variants. A
  cleaner standalone helper header remains a separate ownership decision.
- `clang-format` is not installed in this environment; formatting was kept
  consistent manually in touched regions.

## Proof

Probe-only packet; no code changed, so the delegated build/test proof was not
run.

Delegated proof command if code changed:

```text
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|rv64_roundtrip_contract|cli_riscv64_return_zero_writes_elf_obj|cli_riscv64_vrm_insn_d_source_obj)' > test_after.log 2>&1
```

Result: not run by design. No `test_after.log` was produced or rewritten by
this packet.
