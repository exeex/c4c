# Execution State

Status: Active
Source Idea Path: ideas/open/63_x86_backend_runtime_correctness_regressions.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Runtime Seam
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 2 packet stayed inside
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` and repaired the
prepared `select` renderer so compare setup no longer clobbers a live true
value in `ecx` while pointer-backed per-slot store selects are being emitted.
With that change, regenerated `00040` asm now keeps the incremented
`t[x + 8*y]++` value alive across the index-0 store lane, and `gdb` confirms
the first recursive `go(n=1, x=0, y=0)` entry now sees `t0 = 1` instead of
`t0 = 0`. The delegated proof stayed narrow and
`backend_x86_handoff_boundary` still passes, but
`c_testsuite_x86_backend_src_00040_c` remains blocked by a later wrong-result
runtime seam inside `chk`.

## Suggested Next

Keep Step 2 on runtime correctness, but narrow the next packet to `chk`'s
occupied-square read/accumulation path now that the index-0 board store is
real. The next slice should explain why `chk(0, 0)` still returns `0` with
`t0 = 1`, which still lets execution reach `go(n=8, x=0, y=0)` with `t0 = 8`.

## Watchouts

- Keep ownership on runtime correctness; do not reopen idea 61's prepared
  call-bundle fallback path now that `00040` crosses codegen successfully.
- Pointer-home sync must stay conditional on real authoritative
  `PreparedAddressBaseKind::PointerValue` consumption; making it unconditional
  regresses the canonical asm for
  `pointer-backed same-module global equality-against-immediate guard route`.
- The repaired `chk` lane still emits `mov edi, ecx` / `mov esi, eax`; the old
  `mov edi, r11d` failure at that call site remains fixed.
- The new frame-sizing change is real: regenerated `00040` asm now gives `go`
  a `6992`-byte frame, which matches the previously overflowing stack traffic
  and removes the earlier undersized-frame hang route.
- The latest block-local resolver edits did not touch the real bad lane:
  this is no longer true for the owned lane. Regenerated `00040` asm now emits
  `mov eax, DWORD PTR [rsp] ; mov ecx, eax ; mov eax, DWORD PTR [rsp + 4] ; mov ecx, eax ; ... mov eax, DWORD PTR [rsp] ; add eax, ecx`
  after `call chk`, so `go` no longer rebuilds the index from `r12d`.
- The recursive `go(n + 1, x, y)` site still computes `n + 1` with
  `mov eax, edi ; add eax, 1`, but the new same-module call preservation lane
  now restores `edi/esi/edx` after `chk` and `gdb` sampling confirms the
  recursive entries advance as `go[1] n=0`, `go[2] n=1`, `go[3] n=2`,
  `go[4] n=3`.
- The index-0 board write seam is now fixed. `gdb` at `go(n=1, x=0, y=0)`
  reports `t0 = 1`, and the regenerated store lane no longer writes `ecx`
  back into `t[0]`.
- The remaining failure is now isolated to `chk`. Breaking on
  `chk(x=0, y=0)` with `*(int*)t != 0` still returns `eax = 0`, and execution
  can still reach `go(n=8, x=0, y=0)` with `t0 = 8`.
- The exact proof command is still red because `00040` exits with status `1`,
  and a direct local run now reports `EXIT:1`.
- The uncommitted code changes in `prealloc` and `x86_codegen.hpp` are part of
  the worktree state that exposed this runtime bug; do not discard them while
  diagnosing the active seam.

## Proof

Ran the active proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00040_c)$' | tee test_after.log`.
Current result: `backend_x86_handoff_boundary` passes, while
`c_testsuite_x86_backend_src_00040_c` now fails with
`[RUNTIME_NONZERO] ... exit=1`. Follow-up diagnosis confirms the regenerated
store lane in `go` now preserves the incremented value across the index-0
store select, and `gdb -q -batch -ex 'break go if $rdi==1 && $rsi==0 && $rdx==0' ...`
now reports `t0 = 1` at the first recursive entry. However,
`gdb -q -batch -ex 'break chk if $rdi==0 && $rsi==0 && *(int*)t!=0' -ex run -ex finish ...`
still reports `occupied chk ret=0 t0=1`, and
`gdb -q -batch -ex 'break go if $rdi==8 && $rsi==0 && $rdx==0' ...` still hits
with `t0=8`, so the remaining blocker is inside `chk` rather than the board
store lane. A direct local run with
`timeout 5s build/c_testsuite_x86_backend/src/00040.c.bin` still reports
`EXIT:1`.
