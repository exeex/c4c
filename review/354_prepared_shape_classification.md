# RV64 gcc_torture Prepared Shape Classification

Source idea: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Evidence

Loaded the committed RV64 gcc torture backend scan artifacts from:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/**/case.log`

The scan contains `1012` cases whose compile log reports:

```text
error: --codegen obj failed: RISC-V backend object route unsupported
prepared module shape
```

For classification, each prepared-shape case was re-dumped with:

```sh
build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir \
  --target riscv64-linux-gnu tests/c/external/gcc_torture/<case>
```

No link or qemu execution rerun was required.

## Backend Gate

The diagnostic is coarse. `src/backend/backend.cpp` calls
`write_rv64_prepared_relocatable_elf_object(prepared)` and collapses any
`std::nullopt` into the single message:

```text
RISC-V backend object route unsupported prepared module shape
```

The hidden gate is in `src/backend/mir/riscv/codegen/object_emission.cpp`.
The direct object path rejects:

- any prepared module with `prepared.module.globals` or string constants
- any prepared function that is not a single-block non-variadic function
- function parameter homes that are not simple GPR homes
- instruction shapes outside a narrow direct fragment encoder
- local memory outside the current 4-byte frame-slot subset
- declaration/control-flow entries that do not map to a defined BIR function

There is a separate prepared text emitter in
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp`, but the object route
does not currently use it as the canonical path. This is the architecture-level
finding: RV64 object emission is split between a narrow direct encoder and a
more general prepared-asm emitter, so fixes risk being duplicated unless the
route is consolidated.

## Primary Buckets

Primary bucket means the first durable feature family that should own repair
work, not every feature present in the case.

| Count | Bucket | Representative cases |
| ---: | --- | --- |
| 540 | multi-block control flow | `src/20000113-1.c`, `src/20000205-1.c`, `src/20000217-1.c` |
| 378 | globals or non-string global addresses | `src/20000224-1.c`, `src/20000227-1.c`, `src/20000314-2.c` |
| 70 | string constants / string data addresses | `src/20000112-1.c`, `src/20000121-1.c`, `src/20000223-1.c` |
| 11 | general call lowering shape | `src/20010119-1.c` |
| 6 | non-i32 or pointer local memory width | `src/20001203-1.c` |
| 3 | variadic or vararg entry/call shape | `src/20030914-2.c`, `src/920908-1.c`, `src/va-arg-13.c` |
| 2 | declaration control-flow entries | `src/20030216-1.c`, `src/20030330-1.c` |
| 1 | floating-point or FPR ABI value | `src/20030125-1.c` |
| 1 | local memory addressing/home shape | `src/920410-1.c` |

These counts sum to `1012`.

## Architecture Conclusion

This is not a local one-instruction patch family. The dominant blocker is the
shape of the RV64 object route itself:

- `448` cases are rejected before function lowering because object emission
  has no data-section/global/string-constant path.
- `540` cases are rejected by the direct encoder's single-block assumption.
- the remaining `24` cases are ABI, memory-width, declaration, and edge-shape
  coverage gaps.

The repair order should therefore be:

1. add structured diagnostics for the RV64 prepared object route;
2. decide and implement the object-route architecture, preferably by sharing
   the prepared asm emitter plus internal assembler/object writer instead of
   extending the narrow direct encoder case by case;
3. add module data sections and relocations;
4. cover the residual ABI and memory-width edges.

## Child Ideas

- `ideas/open/355_rv64_prepared_object_shape_diagnostics.md`
- `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`
- `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`
- `ideas/open/358_rv64_object_route_abi_width_edges.md`

