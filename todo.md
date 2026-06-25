Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit RV64 Fixup, Relocation, and Label Ownership

# Current Packet

## Just Finished

Step 4 ownership audit completed for the named RV64 fixup, relocation, and label
surfaces without implementation changes.

| Surface | Classification | Evidence | Suggested next packet |
| --- | --- | --- | --- |
| `src/backend/mir/riscv/codegen/object_emission.{hpp,cpp}`: `RiscvObjectFixup`, `RiscvEncodedFragment`, `build_rv64_text_object_module` | low-level object concern | `RiscvEncodedFragment` carries raw bytes plus local labels/fixups; `append_rv64_local_jump` and `append_rv64_local_branch` create `Jal`/`Branch` fixups; `build_rv64_text_object_module` lays out fragments, binds fragment labels, maps fixup kinds through `rv64_elf_relocation_type`, declares undefined symbols when needed, and attaches relocations to `.text`. This is object-file assembly/linkage plumbing, not prepared/BIR semantic reconstruction. | Consolidate production RV64 object emission around this fragment/fixup/module path and add any missing general fixup kinds here rather than in ad hoc writer branches. |
| `src/apps/c4c-as.cpp::resolve_local_control_flow_labels` and its use of `build_rv64_text_object_module` | textual assembler local-label concern | The resolver operates on parsed textual `Rv64BranchLine`/`Rv64JumpLine`, computes immediate offsets from labels in the same parsed text stream, clears target labels, and verifies encoding range. `build_object_function` then preserves non-global parsed labels as `RiscvObjectLabel`s and `write_object_file` calls `build_rv64_text_object_module`, so object writing uses the shared object module builder after assembler-local label resolution. | Keep local branch/jump immediate resolution in `c4c-as` unless assembler text needs relocatable external branch/jump support; route object-file output through the shared `RiscvObjectFunction`/fragment builder. |
| `src/backend/mir/riscv/assembler/elf_writer.cpp` minimal/pending relocation path: `PendingReloc`, `MinimalJalRelocationSlice`, `parse_minimal_jal_relocation_slice`, `build_minimal_jal_relocation_object`, `minimal_jal_relocation_slice_` emission branch | duplicate path to unify or remove | `PendingReloc` exists only as unused writer state (`pending_branch_relocs_`). `parse_minimal_jal_relocation_slice` recognizes exactly an 8-statement `.text`, `.globl main`, `.globl helper`, `main: jal helper; ret; helper: ret` shape. `build_minimal_jal_relocation_object` hand-builds `.text`, `.rela.text`, `.symtab`, `.strtab`, and an `R_RISCV_JAL` relocation. `ElfWriter::process_statements` records this slice and `write_elf` chooses the bespoke object builder before the minimal activation fallback. This duplicates the shared `RiscvObjectFixupKind::Jal` plus `build_rv64_text_object_module` route and is shape-specific. | Replace or retire the minimal JAL relocation object branch by translating parsed assembler statements into the shared fragment/fixup object route; remove unused pending relocation scaffolding when no caller needs it. |

## Suggested Next

Next packet should unify the `assembler/elf_writer.cpp` minimal JAL relocation
branch with the shared `RiscvObjectFixupKind::Jal` and
`build_rv64_text_object_module` path, or remove that branch if the current
`c4c-as.cpp` object route fully owns textual assembler output.

## Watchouts

- Do not claim additional representative allowlist progress from this audit;
  current accepted state remains 20000205 and 20010119 passing, 20000113 and
  20030216 failing.
- `object_emission.{hpp,cpp}` owns low-level RV64 object fixups and fragment
  layout; do not move prepared/BIR CFG, edge-copy, value-home, frame, or
  diagnostic semantics into fixup attachment.
- `c4c-as.cpp::resolve_local_control_flow_labels` is assembler-text local-label
  lowering before shared object-module construction, not a prepared/BIR route.
- `assembler/elf_writer.cpp` minimal JAL relocation support is a shape-specific
  duplicate object writer and should not become a second production path.
- Keep rejecting testcase-name shortcuts, expectation weakening, target-local
  CFG reconstruction, and silent block/edge dropping.

## Proof

Audit-only packet; no build or CTest was delegated or run, and no root-level
log was created. Inspected:

- `src/backend/mir/riscv/codegen/object_emission.hpp`: `RiscvObjectFixup`,
  `RiscvEncodedFragment`, `build_rv64_text_object_module` declaration.
- `src/backend/mir/riscv/codegen/object_emission.cpp`:
  `append_rv64_local_jump`, `append_rv64_local_branch`,
  `rv64_elf_relocation_type`, `build_rv64_text_object_module`.
- `src/apps/c4c-as.cpp`: `resolve_local_control_flow_labels`,
  `build_object_function`, `write_object_file`.
- `src/backend/mir/riscv/assembler/elf_writer.cpp`: `PendingReloc`,
  `MinimalJalRelocationSlice`, `parse_minimal_jal_relocation_slice`,
  `build_minimal_jal_relocation_object`, `ElfWriter::process_statements`,
  `ElfWriter::write_elf`, `minimal_jal_relocation_slice_`.
