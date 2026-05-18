# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extend Across Control And Boundary Consumers

# Current Packet

## Just Finished

Step 3 finished the focused starter subset by repairing loop-carried scalar
frame-slot publication for `src/00161.c`.

Prepared BIR already named the missing authority:

```text
%t6 = bir.add i32 %t4, %t5
bir.store_local %lv.a, i32 %t6
storage %t6 value_id=6 encoding=frame_slot bank=gpr spill_slot=slot#3+stack12 width=1 slot_id=#3 stack_offset=12
access block=block_1 inst_index=7 base=frame_slot stored=%t6 frame_slot=#0 offset=0 size=4 align=4 base_plus_offset=yes
```

`src/backend/mir/aarch64/codegen/alu.cpp` now treats a prepared scalar ALU
result whose home/storage is a frame slot as a real publication target: it
computes the result through a spill-authority scratch register and emits the
authoritative stack-slot store. `src/backend/mir/aarch64/codegen/memory.cpp`
and `machine_printer.cpp` now let a following scalar local store consume that
prepared frame-slot value by loading it from the prepared stack address and
storing it to the destination local slot.

Generated AArch64 for `src/00161.c` now publishes `%t6 = t + p` and updates
local `a` before the loop-control compare:

```asm
ldr w13, [sp, #8]
ldr w20, [sp, #4]
add w9, w13, w20
str w9, [sp, #12]
ldr w9, [sp, #12]
str w9, [sp]
...
ldr w13, [sp]
cmp w13, #100
```

The focused starter subset now passes all six representatives: `00009`,
`00012`, `00056`, `00156`, `00161`, and `00211`.

## Suggested Next

Ask the supervisor for acceptance-level validation for the current focused
AArch64 scalar authority slice. The starter subset is green, but the route has
changed return publication, scalar call-argument placement, branch/control
immediates, scalar ALU publication, and frame-slot local-store publication, so
broader AArch64 backend proof is the next coherent packet before close or
handoff.

## Watchouts

- Do not solve this by filename checks, expectation changes, allowlist changes,
  unsupported downgrades, or CTest edits.
- Keep pointer/aggregate address failures, timeout/hang cases, and
  compile-stage printer gaps out of this owner.
- `src/00211.c` now passes after the immediate GPR call-argument placement fix;
  do not spend the next packet on that representative unless it regresses.
- `src/00056.c` now passes because the call-boundary load uses the prepared
  memory access for `%t8` (`frame_slot=#3`, current offset `12`) rather than
  the logical storage-plan spill slot `#6+stack24`.
- `src/00009.c` now passes because `mul`, `sdiv`, and signed remainder are
  emitted from the lowering-only scalar publication path; do not add these
  opcodes to the public supported-integer vocabulary or route UDiv/URem
  power-of-two cases around the existing `lsr`/`and` reduction path.
- `src/00156.c` now passes because fused-compare immediate operands preserve
  nonzero semantic constants; keep this as branch/control immediate authority,
  not a filename-shaped loop special case.
- `src/00161.c` now passes because the prepared scalar add `%t6` publishes to
  its frame-slot storage and the subsequent local store consumes that prepared
  frame-slot value; keep this as scalar frame-slot publication, not a Fibonacci
  or loop-shape shortcut.
- Prepared storage plans can name logical spill slots that are not current
  printable frame slots. For future frame-slot publication work, prefer
  prepared addressing plus stack-layout resolution when the value is produced by
  a load access.
- `clang-format` is not installed in this environment.

## Proof

Ran focused scalar subset exactly as delegated:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: passed, 6/6. `00009`, `00012`, `00056`, `00156`, `00161`, and `00211`
all passed. Proof log: `test_after.log`.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
