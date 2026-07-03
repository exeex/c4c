Status: Active
Source Idea Path: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-Run The Nine Representatives

# Current Packet

## Just Finished

Step 3 reran the nine retained RV64 `unsupported_instruction_fragment`
representatives through the requested evidence pipeline and recorded enriched
owner-routing diagnostics under
`build/agent_state/570_unsupported_instruction_fragment_diagnostics/`.

- All nine cases returned rc=0 for `--dump-bir`, `--dump-prepared-bir`, and
  `--dump-mir`, then rc=1 for the RV64 object-route CMake runner.
- The new diagnostics split the old generic RV64 object-lowering bucket into
  likely owner families: inline asm carrier lowering (`src/20071211-1.c`,
  `src/pr51933.c`, `src/pr56982.c`, `src/pr78438.c`), same-module call/result
  lowering (`src/20000412-2.c`, `src/20000622-1.c`), select / branch-published
  phi-select lowering (`src/20030408-1.c`), floating-point binary lowering
  (`src/20000605-1.c`), and pointer arithmetic lowering (`src/20000819-1.c`).
- The compact classification table is
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`;
  the narrative summary is
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/summary.md`.
- No unsupported RV64 lowering behavior, unsupported markers, allowlists, or
  runtime comparison files were changed. No testcase is claimed fixed.

## Suggested Next

Use the Step 3 classification table to pick the next focused Step 4 owner
packet. A compact first packet would be inline asm carrier lowering triage,
because four retained representatives now route there with `CallInst;
owner=none` at `llvm.inline_asm`.

## Watchouts

- This packet is evidence-only; the nine object-route failures remain expected
  unsupported lowering diagnostics.
- `src/pr78438.c` previously looked like an arithmetic/shift family from source
  shape, but the first enriched object fallback is the entry `llvm.inline_asm`
  call, so routing it to shift lowering would be premature without bypassing or
  implementing inline asm handling first.
- `src/20000605-1.c` points at `BinaryInst; owner=double %t5`, specifically a
  double FP division in prepared BIR, before later FP casts.
- `src/20000819-1.c` points at `BinaryInst; owner=ptr %t4`, a pointer add of a
  loaded base and scaled byte offset.
- Do not add named-case matching, expectation downgrades, unsupported marker
  changes, allowlist changes, or runtime comparison changes.

## Proof

Build proof:
`cmake --build --preset default > test_after.log 2>&1`

Result: passed (`ninja: no work to do`).
Log path: `test_after.log`.

Evidence rerun command family:
`./build/c4cll --dump-bir --target riscv64-linux-gnu <case>`,
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <case>`,
`./build/c4cll --dump-mir --target riscv64-linux-gnu <case>`, and the RV64
object-route runner from
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`.

Rerun results and per-case logs:
`build/agent_state/570_unsupported_instruction_fragment_diagnostics/`.
