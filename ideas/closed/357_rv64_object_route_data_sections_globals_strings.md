# RV64 Object Route Data Sections for Globals and Strings

Status: Closed
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Add RV64 object emission support for prepared module data: globals, string
constants, data sections, symbol definitions, and code/data relocations needed
by generated RV64 object files.

This idea owns target object data sections, ELF symbols, and relocations only
for data that is already represented in the prepared module contract. If string
literals, globals, initializer bytes, alignments, or address-use semantics are
missing upstream, that is a separate BIR/LIR prepared-data contract problem and
must not be patched inside RV64 object emission.

## Why This Exists

The prepared-shape classification found `448` primary failures rejected before
function lowering because `object_emission.cpp` refuses prepared modules with
globals or string constants. This is the largest non-CFG blocker:

- `378` cases with globals or non-string global addresses
- `70` cases with string constants or string data addresses

## In Scope

- Emit `.rodata`/data-equivalent object sections for string constants and
  supported global initializers already described by prepared module data.
- Define data symbols with stable binding, size, alignment, and section
  placement from prepared-module metadata.
- Support code references to global/string addresses through RV64 relocation
  records compatible with the existing ELF writer when the prepared contract
  already exposes the symbol/address reference.
- Keep behavior compatible with clang RV64 linking and qemu execution.
- Add representative tests that exercise both string constants and mutable
  globals.
- When missing upstream representation is discovered, create or route to a
  BIR/LIR data-initializer contract idea instead of inventing that
  representation inside RV64 object emission.

## Out of Scope

- Full C aggregate initializer completion beyond what prepared BIR already
  represents.
- Defining missing prepared-module string/global/initializer representation.
- Scanning C/LIR/BIR in RV64 object emission to infer data initializers,
  symbol lifetimes, or address-use semantics that the prepared module did not
  publish.
- Linker relaxation support beyond the relocations needed for correctness.
- Disassembler pretty-printing.
- Default full-CTest inclusion for the heavy gcc torture scan.

## Representative Cases

- `src/20000224-1.c`: global `load_global`/`store_global` in multi-block code.
- `src/20000227-1.c`: global-address family.
- `src/20000112-1.c`: string constants from libc-style/string tests.
- `src/20000223-1.c`: many string addresses passed to same-module calls.

## Suspected Stage

RV64 object module construction and ELF relocation/section emission for
prepared data that already exists. Missing prepared data representation is an
upstream BIR/LIR contract stage, not this target-emission idea.

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
- If a representative case lacks prepared data representation, it is routed to
  an upstream BIR/LIR data-init idea with a precise diagnostic instead of being
  locally patched in RV64 object emission.

## Reviewer Reject Signals

- Reject if string/global testcase names are special-cased.
- Reject if global data is emitted as hard-coded text-only addresses that only
  work in one binary layout.
- Reject if mutable globals are treated as read-only or string data is emitted
  without correct symbol/relocation semantics.
- Reject if RV64 object emission invents initializer bytes, symbol semantics,
  or global/string address meaning that the prepared module did not provide.
- Reject if the repair bypasses the internal object model in a way that cannot
  support later data kinds.
- Reject if the heavy scan appears greener only because cases are skipped or
  downgraded.

## Closure Notes

Closed after the Step 5 milestone validation for idea 357. RV64 object emission
now consumes supported prepared globals and string constants through target
object data sections, ELF data symbols, and code/data relocation records instead
of rejecting the blanket `prepared.module.globals/string_constants` shape.

Focused object validation passed for the present object/data tests:
`backend_cli_riscv64_unsupported_global_diagnostic_obj`,
`backend_object_model_records`, and `backend_riscv_object_emission`. The full
RV64 gcc torture backend milestone scan completed with `total=1467 passed=145
failed=1322`. Representative cases `src/20000223-1.c` and `src/20000227-1.c`
passed; `src/20000112-1.c` and `src/20000224-1.c` routed to the later
`unsupported_terminator_fragment` bucket, not to data section, global symbol,
string constant, or relocation failures.

Remaining data-looking failures are outside this RV64 object-emission scope and
are intentionally left to follow-up layer owners under the open 354 umbrella:
44 `unsupported_global_data` object-route residuals and 52 upstream LIR-to-BIR
global/string publication gaps. Representative follow-up shapes include missing
prepared global memory facts, wider or non-linear prepared global storage,
direct global-symbol base-plus-offset memory addressing, non-byte-addressable
string-pool support, and broader LIR-to-BIR global publication. These must not
be repaired by making RV64 object emission infer facts that the prepared module
did not publish.
