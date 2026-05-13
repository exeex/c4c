# AArch64 Global Addressing Legacy Surface

This artifact preserves the useful production shape from the removed
`globals.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/globals.rs`.

## Role

The surface implemented AArch64 address materialization for globals, labels,
and thread-local globals in `ArmCodegen`. Each entry point built an address in
integer register `x0` and then stored that address through the backend's normal
destination helper.

The key distinction was whether a named global required Global Offset Table
indirection. Ordinary labels and non-GOT globals used page-relative symbol
address construction directly, while GOT globals loaded the final address from
the GOT entry.

## Entry Points

- `emit_global_addr_impl(dest, name)`: materializes the address for a named
  global. It queries `state.needs_got_aarch64(name)` to decide between GOT and
  direct page-relative addressing, then stores `x0` to `dest`.
- `emit_label_addr_impl(dest, label)`: materializes a local or backend label
  address with direct page-relative addressing and stores `x0` to `dest`.
- `emit_tls_global_addr_impl(dest, name)`: materializes a thread-local global
  address relative to `tpidr_el0` and stores `x0` to `dest`.

## Addressing Forms

For globals that require GOT indirection, the old route emitted:

```asm
adrp x0, :got:<name>
ldr x0, [x0, :got_lo12:<name>]
```

For ordinary global addresses, it emitted direct page-relative materialization:

```asm
adrp x0, <name>
add x0, x0, :lo12:<name>
```

Labels used the same direct `ADRP` plus low-12-bit `ADD` form:

```asm
adrp x0, <label>
add x0, x0, :lo12:<label>
```

Thread-local globals were based on the AArch64 thread pointer register:

```asm
mrs x0, tpidr_el0
add x0, x0, :tprel_hi12:<name>
add x0, x0, :tprel_lo12_nc:<name>
```

## Dependencies

The removed surface depended on these surrounding concepts:

- IR destination model: `Value`.
- Backend assembly emission through `state.emit` and `state.emit_fmt`.
- Target relocation-policy query: `state.needs_got_aarch64(name)`.
- AArch64 operand/result helper: `store_x0_to`.
- AArch64 scratch/result convention: all materialized addresses leave the
  address value in `x0` before storage.
- Platform relocation syntax for `:got:`, `:got_lo12:`, `:lo12:`,
  `:tprel_hi12:`, and `:tprel_lo12_nc:`.
- AArch64 thread pointer register access through `mrs x0, tpidr_el0`.

## Hidden Assumptions

- `x0` is available as the address construction scratch register and remains
  the value consumed by `store_x0_to`.
- `needs_got_aarch64(name)` fully captures when a global address must be loaded
  through a GOT entry instead of formed directly with `ADRP` and `ADD`.
- Label addresses are never routed through GOT indirection in this surface.
- Thread-local addresses use the local-exec style `tpidr_el0` plus
  thread-pointer-relative relocations.
- Symbol and label names are already assembler-safe; this surface did not own
  symbol sanitization or identity recovery.
- Relocation expressions are emitted as textual assembly operands and are
  expected to be understood by the later assembler/linker path.

## Known Failure Risks For Rebuild

- Treating all globals as direct addresses would break position-independent or
  externally visible symbols that require GOT loads.
- Treating all globals as GOT addresses would pessimize or misrepresent local
  symbol addressing and could diverge from the relocation policy encoded in
  `needs_got_aarch64`.
- Dropping the low-12-bit `ADD` after `ADRP` would leave only the symbol page
  address for direct globals and labels.
- Replacing TLS materialization with ordinary global addressing would lose the
  per-thread base and produce process-global addresses instead of thread-local
  addresses.
- Rebuilding this around rendered-name lookup would conflict with the new
  markdown-first plan's requirement to use structured backend facts rather than
  string fallback routes.

## Rebuild Guidance

Rebuild this surface around an explicit address-kind decision:

Global and label materialization should publish structured target MIR facts and
machine instruction nodes for address computation and relocation needs. Symbol
spellings, `ADRP`/`ADD` printer text, and relocation syntax are downstream
printer or encoding/object concerns, not lookup authority for codegen.

1. Keep global, label, and thread-local address materialization as separate
   cases.
2. Preserve the GOT decision as target relocation policy, not as a string
   naming convention.
3. Keep `x0` as the visible materialized-address result until the replacement
   backend defines a different target-local MIR value convention.
4. Prove direct global and label paths emit both `ADRP` and low-12-bit `ADD`.
5. Prove the TLS path reads `tpidr_el0` and applies thread-pointer-relative
   relocations before storing the result.
