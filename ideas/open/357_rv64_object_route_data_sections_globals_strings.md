# RV64 Object Route Data Sections for Globals and Strings

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Add RV64 object emission support for prepared module data: globals, string
constants, data sections, symbol definitions, and code/data relocations needed
by generated RV64 object files.

## Why This Exists

The prepared-shape classification found `448` primary failures rejected before
function lowering because `object_emission.cpp` refuses prepared modules with
globals or string constants. This is the largest non-CFG blocker:

- `378` cases with globals or non-string global addresses
- `70` cases with string constants or string data addresses

## In Scope

- Emit `.rodata`/data-equivalent object sections for string constants and
  supported global initializers.
- Define data symbols with stable binding, size, alignment, and section
  placement.
- Support code references to global/string addresses through RV64 relocation
  records compatible with the existing ELF writer.
- Keep behavior compatible with clang RV64 linking and qemu execution.
- Add representative tests that exercise both string constants and mutable
  globals.

## Out of Scope

- Full C aggregate initializer completion beyond what prepared BIR already
  represents.
- Linker relaxation support beyond the relocations needed for correctness.
- Disassembler pretty-printing.
- Default full-CTest inclusion for the heavy gcc torture scan.

## Representative Cases

- `src/20000224-1.c`: global `load_global`/`store_global` in multi-block code.
- `src/20000227-1.c`: global-address family.
- `src/20000112-1.c`: string constants from libc-style/string tests.
- `src/20000223-1.c`: many string addresses passed to same-module calls.

## Suspected Stage

RV64 object module construction and ELF relocation/section emission.

## Proof Command

Narrow proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-data-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

with:

```text
src/20000112-1.c
src/20000223-1.c
src/20000224-1.c
src/20000227-1.c
```

Milestone proof:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

## Acceptance Criteria

- Representative string/global cases reach RV64 link and qemu execution or
  fail at a later, more specific backend bucket.
- The old blanket `prepared.module.globals/string_constants` rejection is gone
  for supported scalar/string data.
- Generated object files contain valid ELF sections, symbols, and relocations
  for referenced data.
- Existing object tests and c4c-as/c4c-objdump roundtrip tests remain green.

## Reviewer Reject Signals

- Reject if string/global testcase names are special-cased.
- Reject if global data is emitted as hard-coded text-only addresses that only
  work in one binary layout.
- Reject if mutable globals are treated as read-only or string data is emitted
  without correct symbol/relocation semantics.
- Reject if the repair bypasses the internal object model in a way that cannot
  support later data kinds.
- Reject if the heavy scan appears greener only because cases are skipped or
  downgraded.

