Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Find First Wrong Edge

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by gathering first-wrong-edge evidence for one
still-reproducing RV64 runtime representative per live mode:
`src/pr38533.c` abort, `src/20080506-2.c` segfault, and `src/pr81503.c`
exit 255. Evidence is recorded in
`docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`, with
per-case BIR, prepared-BIR, MIR, disassembly, linked binary, and qemu artifacts
under `build/agent_state/423_step3_first_wrong_edge/`.

## Suggested Next

Execute Step 4 grouping using the Step 3 evidence: group `src/pr38533.c` under
inline-asm tied-output/result materialization, `src/20080506-2.c` under
frame-slot call-argument publication/materialization, and `src/pr81503.c`
under binary operand/value materialization after prepared BIR.

## Watchouts

- This is a triage runbook, not an expectation or allowlist route.
- Do not weaken runtime comparison, expected output, unsupported markers, or
  pass/fail accounting.
- All three Step 3 `--dump-mir` artifacts report `route owner: x86/debug`
  even with the RV64 target, so treat MIR as route/debug evidence only.
- `src/20080506-2.c` has an explicit prepared-BIR
  `missing_frame_slot_arg_publication=yes` marker; that is producer-owned
  evidence before the later segfault.
- `src/pr38533.c` and `src/pr81503.c` have enough disassembly/prepared-BIR
  evidence for Step 4 grouping, but fixes should start at producer/lowering
  ownership rather than testcase-specific RV64 shortcuts.

## Proof

Passed:
`bash -lc 'set -u; out=build/agent_state/423_step3_first_wrong_edge; mkdir -p "$out"; : > test_after.log; cmake --build build --target c4cll >> test_after.log 2>&1 || exit 1; cases="src/pr38533.c src/20080506-2.c src/pr81503.c"; for case in $cases; do id=$(printf "%s" "$case" | sed "s#[^A-Za-z0-9_.-]#_#g"); src="$(pwd)/tests/c/external/gcc_torture/$case"; ./build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "$out/${id}.bir.txt" 2> "$out/${id}.bir.err" || true; ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "$out/${id}.prepared-bir.txt" 2> "$out/${id}.prepared-bir.err" || true; ./build/c4cll --dump-mir --target riscv64-linux-gnu "$src" > "$out/${id}.mir.txt" 2> "$out/${id}.mir.err" || true; ./build/c4cll --codegen obj --target riscv64-linux-gnu "$src" -o "$out/${id}.c4c.o" > "$out/${id}.obj.out" 2> "$out/${id}.obj.err" || true; if [ -f "$out/${id}.c4c.o" ]; then riscv64-linux-gnu-objdump -dr "$out/${id}.c4c.o" > "$out/${id}.c4c-objdump.txt" 2> "$out/${id}.c4c-objdump.err" || true; clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -std=gnu89 -w -I tests/c/external/gcc_torture "$src" -o "$out/${id}.clang.bin" > "$out/${id}.clang.out" 2> "$out/${id}.clang.err" || true; clang --target=riscv64-linux-gnu --gcc-toolchain=/usr "$out/${id}.c4c.o" -o "$out/${id}.c4c.bin" > "$out/${id}.link.out" 2> "$out/${id}.link.err" || true; if [ -f "$out/${id}.c4c.bin" ]; then riscv64-linux-gnu-objdump -dr "$out/${id}.c4c.bin" > "$out/${id}.c4c-bin-objdump.txt" 2> "$out/${id}.c4c-bin-objdump.err" || true; timeout 20 qemu-riscv64 -L /usr/riscv64-linux-gnu "$out/${id}.c4c.bin" > "$out/${id}.c4c-qemu.out" 2> "$out/${id}.c4c-qemu.err"; printf "%s" "$?" > "$out/${id}.c4c-qemu.rc"; fi; if [ -f "$out/${id}.clang.bin" ]; then riscv64-linux-gnu-objdump -dr "$out/${id}.clang.bin" > "$out/${id}.clang-bin-objdump.txt" 2> "$out/${id}.clang-bin-objdump.err" || true; timeout 20 qemu-riscv64 -L /usr/riscv64-linux-gnu "$out/${id}.clang.bin" > "$out/${id}.clang-qemu.out" 2> "$out/${id}.clang-qemu.err"; printf "%s" "$?" > "$out/${id}.clang-qemu.rc"; fi; fi; done; find "$out" -maxdepth 1 -type f | sort > "$out/artifacts.txt"; rg -n "abort|call|return|store|load|branch|seg|255|main|exit" "$out"/*.txt "$out"/*.err >> test_after.log 2>&1 || true; git diff --check -- docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md todo.md >> test_after.log 2>&1'`.
Proof output is preserved in `test_after.log`.
