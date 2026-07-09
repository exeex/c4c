# Global Residual Runtime Mismatch

Source idea: `ideas/open/642_rv64_global_residual_runtime_mismatch_research.md`

This file publishes the focused research answer for the residual
`src/pr79737-2.c` runtime mismatch. It is intentionally research-only: no
implementation, expectation, unsupported-marker, allowlist, timeout, runtime
comparison, or accounting behavior changed for this classification.

## Rerun Evidence

The row was refreshed with:

```sh
(cmake --build --preset default && ALLOWLIST=build/agent_state/642_step1_pr79737.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

The focused rerun still fails as one runtime residual:

- `test_after.log`: build succeeded, then
  `[rv64-gcc-torture] total=1 passed=0 failed=1`.
- `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
  `c4c_exit=Subprocess aborted`.
- Row artifacts:
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/c4c.o`,
  `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/c4c.bin`,
  and `build/rv64_gcc_c_torture_backend/src_pr79737-2.c/clang.bin`.

Step 2 inspection artifacts:

- `build/agent_state/642_step2_symbols.txt`
- `build/agent_state/642_step2_relocations.txt`
- `build/agent_state/642_step2_c4c_o_disasm.txt`
- `build/agent_state/642_step2_c4c_bin_disasm.txt`
- `build/agent_state/642_step2_clang_bin_disasm.txt`

## Final Classification

First owner: packed bitfield/global-object layout and bitfield access
lowering.

`src/pr79737-2.c` declares two packed file-scope instances of a bitfield
struct. The field widths are `18 + 1 + 24 + 15 + 14 = 72` bits, so the control
layout stores each object in 9 bytes. The captured evidence shows C4C instead
materializes `i` and `j` as 12-byte globals and accesses them through
32-bit word lanes. The C4C binary then reaches the source self-check and calls
`abort()` after the generated values diverge from the clang control binary.

Concrete evidence:

- `build/agent_state/642_step2_symbols.txt` records C4C `c4c.o` symbols
  `i` size `12` and `j` size `12`.
- The same symbol artifact records linked C4C addresses `i=0x2019`,
  `j=0x2025`, while clang links `i=0x2019`, `j=0x2022`. That is the expected
  12-byte stride versus 9-byte packed stride difference.
- `build/agent_state/642_step2_c4c_o_disasm.txt` and
  `build/agent_state/642_step2_c4c_bin_disasm.txt` show C4C using `lw` and
  `sw` accesses for the globals at word-lane offsets.
- `build/agent_state/642_step2_clang_bin_disasm.txt` shows byte-oriented
  packed access with `lbu` and `sb` over the 9-byte globals.
- `build/agent_state/642_step2_c4c_bin_disasm.txt` branches to `abort@plt`
  after the generated comparisons over those loaded global values.

This is not true runtime support ownership. The clang control binary exits
cleanly under the same harness, and the C4C binary reaches libc `abort` through
the source-level predicate after its global bitfield data has been mis-modeled.

## Candidate Owners

| Candidate owner | Classification | Evidence |
| --- | --- | --- |
| Packed bitfield/global-object layout and access lowering | First owner | C4C uses 12-byte global objects and word-lane access for a 72-bit packed bitfield struct that clang lays out as 9-byte objects with byte-lane access. |
| Global/local memory lowering | Ruled out as first owner | Global symbol address materialization works far enough to compile, link, and execute. The first proven bad fact is the object size and lane model consumed by global accesses. |
| Object relocation | Ruled out | `642_step2_relocations.txt` shows ordinary linked relocation shapes for this row, not a loader relocation assertion like the `src/990106-1.c` call-lowering row. |
| Stack layout | Ruled out | The checked storage homes are file-scope globals `i` and `j`; stack traffic in `main` is comparison scratch. |
| ABI or call setup | Ruled out as first owner | `foo()` and `bar()` have no parameters or return value, and `main` reaches both direct calls before the final predicate. |
| Branch/control flow | Ruled out as first owner | The branch to `abort@plt` is downstream of comparisons over loaded bitfield values, with no independent bad target, timeout, or unreachable-control evidence. |
| True runtime support | Ruled out | The clang binary exits `0` under the same runtime path; C4C aborts because generated data differs. |

## Follow-Up Recommendation

Open a separate packed bitfield/global-layout implementation idea if this row
is selected for repair. Keep the proof surface narrow and owner-first:

- prove C4C lays out this packed 72-bit bitfield aggregate as 9-byte globals
  for both `i` and `j`;
- prove global bitfield loads and stores use packed byte-lane access rather
  than 12-byte word-lane storage for this shape;
- rerun `src/pr79737-2.c` without weakening expectations, unsupported markers,
  allowlists, timeout policy, runtime comparison, or accounting.

No broader runtime-support idea is justified by this row.
