# RV64 gcc_torture Runtime Mismatch Representatives

Status: Step 2 representative reproduction complete.

## Runner Command Shape

Each representative was reproduced with the single-case RV64 gcc_torture
backend object runner after rebuilding `c4cll`:

```sh
cmake --build build --target c4cll
cmake \
  -DCOMPILER="$(pwd)/build/c4cll" \
  -DCLANG="$(command -v clang)" \
  -DQEMU_RISCV64="$(command -v qemu-riscv64)" \
  -DSRC="$(pwd)/tests/c/external/gcc_torture/<case>" \
  -DROOT="$(pwd)/tests/c/external/gcc_torture" \
  -DTARGET_TRIPLE=riscv64-linux-gnu \
  -DSYSROOT=/usr/riscv64-linux-gnu \
  -DOUT_CLANG_BIN="$(pwd)/build/agent_state/423_step2_runtime_reproduction/<id>.clang.bin" \
  -DOUT_OBJECT="$(pwd)/build/agent_state/423_step2_runtime_reproduction/<id>.c4c.o" \
  -DOUT_C4C_BIN="$(pwd)/build/agent_state/423_step2_runtime_reproduction/<id>.c4c.bin" \
  -DCASE_TIMEOUT_SEC=20 \
  -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

The repeatable evidence lives under
`build/agent_state/423_step2_runtime_reproduction/`. The tabular runner
summary is
`build/agent_state/423_step2_runtime_reproduction/summary.tsv`.

## Representative Results

| Case | Step 1 bucket | Reproduction result | Case log |
| --- | --- | --- | --- |
| `src/20000314-2.c` | abort | pass in this rerun | `build/agent_state/423_step2_runtime_reproduction/src_20000314-2.c.case.log` |
| `src/pr38533.c` | abort | `clang_exit=0 c4c_exit=Subprocess aborted` | `build/agent_state/423_step2_runtime_reproduction/src_pr38533.c.case.log` |
| `src/pr83298.c` | abort | `clang_exit=0 c4c_exit=Subprocess aborted` | `build/agent_state/423_step2_runtime_reproduction/src_pr83298.c.case.log` |
| `src/20080506-2.c` | segfault | `clang_exit=0 c4c_exit=Segmentation fault` | `build/agent_state/423_step2_runtime_reproduction/src_20080506-2.c.case.log` |
| `src/pr43008.c` | segfault | `clang_exit=0 c4c_exit=Segmentation fault` | `build/agent_state/423_step2_runtime_reproduction/src_pr43008.c.case.log` |
| `src/zerolen-1.c` | segfault | `clang_exit=0 c4c_exit=Segmentation fault` | `build/agent_state/423_step2_runtime_reproduction/src_zerolen-1.c.case.log` |
| `src/pr81503.c` | exit 255 | `clang_exit=0 c4c_exit=255` | `build/agent_state/423_step2_runtime_reproduction/src_pr81503.c.case.log` |

Six of the seven representatives still reproduce as runtime mismatches with
empty captured `clang_out` and `c4c_out` blocks. The reproduced mismatch
population is:

- Abort: `src/pr38533.c`, `src/pr83298.c`
- Segfault: `src/20080506-2.c`, `src/pr43008.c`, `src/zerolen-1.c`
- Exit 255: `src/pr81503.c`

`src/20000314-2.c` was an abort row in the Step 1 full-scan inventory, but the
single-case runner now reports:

```text
-- [PASS][rv64-gcc-torture-backend-obj] /workspaces/c4c/tests/c/external/gcc_torture/src/20000314-2.c
```

Treat that case as stale or non-deterministic for Step 3 unless a later
supervisor-selected scan reproduces the original abort again.

## Step 3 Recommendation

Start first-wrong-edge work from a still-reproducing case in each live exit
mode rather than from `src/20000314-2.c`. A compact Step 3 packet can compare
prepared/BIR/RV64 lowering and qemu behavior for:

- `src/pr38533.c` or `src/pr83298.c` for the abort family.
- `src/20080506-2.c`, `src/pr43008.c`, or `src/zerolen-1.c` for the segfault
  family.
- `src/pr81503.c` for the exit-255 singleton.

The first pass should identify the first wrong edge for one representative
per mode before attempting a whole-family repair.
