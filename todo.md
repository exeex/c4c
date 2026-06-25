Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace the Blanket Single-Block Gate

# Current Packet

## Just Finished

Step 3 implemented the first block-aware RV64 prepared object emission path.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `todo.md`

What changed:

- Added RV64 object fixup kinds for local conditional branches and local jumps:
  `Branch` -> `R_RISCV_BRANCH`, `Jal` -> `R_RISCV_JAL`.
- Reworked `build_rv64_text_object_module` to append/bind labels before
  attaching relocations for each function. This lets forward local labels,
  recursive calls, and same-module references resolve through explicit symbols
  instead of being mistaken for permanent undefineds.
- Skipped declaration-only prepared control-flow entries before object
  conversion, matching the `.s` emitter's behavior.
- Replaced the blanket `function->blocks.size() != 1` rejection with
  block-aware prepared-function conversion:
  - one local label fragment per BIR/prepared block,
  - instruction fragments selected from prepared BIR and prepared lookups,
  - explicit return / branch / conditional branch terminator fragments,
  - prepared fused branch conditions consumed as the branch authority,
  - local label references emitted as fixups/relocations, not recovered CFG.
- Added minimal general scalar object fragments needed by the clean
  representative path: `and`, `xor`, and materialized `eq` / `ne`.

Result buckets for the Step 1 representative allowlist:

| Case | Result | Current layer |
| --- | --- | --- |
| `src/20000113-1.c` | fail | No longer the first clean target; still fails generic prepared-shape diagnostic. Likely next blocker is stack-homed local load/store/publication and richer scalar/local-memory fragment support, not the original multi-block gate. |
| `src/20000205-1.c` | pass | Clean multi-block object route now compiles to `.o`, links with clang, and matches clang under qemu-riscv64. |
| `src/20010119-1.c` | pass | Declaration-only `exit` control-flow entry is skipped correctly; the single-block same-module/external-call object path still works. |
| `src/20030216-1.c` | fail | Expected out-of-scope globals/data-section blocker for idea 357. |

The key acceptance point for this packet is met: `src/20000205-1.c` no longer
fails at the prepared object single-block gate and now passes executable RV64
link-and-qemu comparison.

## Suggested Next

Execute Step 4 as a focused encoding/fixup cleanup and sharing packet:

- Add regression coverage for a small prepared multi-block object module with
  local conditional branch / jump labels, preferably near
  `backend_riscv_object_emission`.
- Decide whether the duplicated RV64 B/J encoders in object emission and
  `rv64_line_assembler.cpp` should be shared now or left until after the next
  capability blocker.
- If continuing capability work before Step 4 cleanup, split a narrow packet
  for the `20000113-1.c` next blocker: stack-homed local load/store/publication
  fragments for object emission. Do not hide that inside CFG work.

## Watchouts

- Do not move CFG semantics into assembler/object writer. The new code consumes
  prepared BIR block order, labels, branch conditions, call plans, and value
  homes as authority.
- The current edge-publication/copy-obligation path is not fully emitted in
  object emission yet. `20000205-1.c` does not need it; `20000113-1.c` likely
  exposes local-memory/publication support next.
- The top-level `--codegen obj` diagnostic is still coarse for remaining
  failures. That is a diagnostics/shape reporting problem, not proof that the
  single-block gate remains first.
- `20030216-1.c` should stay with idea 357 unless the active plan is explicitly
  switched to globals/data sections.

## Proof

Proof log: `test_after.log`

Delegated proof command:

```sh
cmake --build --preset default && \
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Allowlist used:

```text
src/20000113-1.c
src/20000205-1.c
src/20010119-1.c
src/20030216-1.c
```

Proof result:

- build: pass, `[build-exit] 0`
- narrow scan: `total=4 passed=2 failed=2`, `[rv64-scan-exit] 1`
- pass: `src/20000205-1.c`, `src/20010119-1.c`
- fail: `src/20000113-1.c`, `src/20030216-1.c`

Additional touched-surface validation:

```sh
ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure
```

Result: pass, `[ctest-riscv-object-exit] 0`.
