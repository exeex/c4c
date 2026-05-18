# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Scalar Publication Primitive

# Current Packet

## Just Finished

Step 2 corrected the `src/00009.c` scalar integer multiply/divide/remainder
publication primitive so it preserves the existing AArch64 backend contracts
for unsupported public scalar-ALU vocabulary and unsigned power-of-two
div/rem reductions.

`src/backend/mir/aarch64/codegen/alu.cpp` keeps the public
`is_scalar_alu_integer_opcode` vocabulary narrow: `mul`, signed `div`, and
signed `rem` remain outside that supported predicate, and the prepared scalar
ALU record API still fails closed for signed reductions and non-power unsigned
reductions. A private publication predicate is used only while lowering a
machine instruction with an authoritative prepared result register, allowing
the local-store publication case to emit real `mul`, `sdiv`, and `sdiv`/`msub`
without broadening the API-level supported opcode set.

Unsigned `UDiv`/`URem` power-of-two records continue to route through the
existing structured reduction printer as `lsr`/`and`; the generic
multiply/divide/remainder printer does not claim those opcodes.

For `src/00009.c`, generated code now publishes each scalar expression result
before storing back to local `x`:

```asm
ldr w13, [sp]
mov w19, #10
mul w19, w13, w19
str w19, [sp]
ldr w13, [sp]
mov w19, #2
sdiv w19, w13, w19
str w19, [sp]
ldr w13, [sp]
mov w9, #3
sdiv w19, w13, w9
msub w19, w19, w9, w13
str w19, [sp]
```

The focused subset still has `00009.c`, `00012.c`, `00056.c`, and `00211.c`
passing. `00156.c` and `00161.c` remain failing in the loop/control-value
subfamily and were not special-cased in this packet.

## Suggested Next

Continue with the remaining representatives `src/00156.c` and `src/00161.c`
as a focused scalar branch/loop-control-value publication packet. Start from
their generated AArch64 control-flow and predicate materialization, and only
reuse the scalar ALU primitive from this packet if the same stale publication
defect directly appears.

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
- Do not reinterpret the remaining `00156`/`00161` failures as plain
  arithmetic publication without generated control-flow evidence.
- Prepared storage plans can name logical spill slots that are not current
  printable frame slots. For future frame-slot publication work, prefer
  prepared addressing plus stack-layout resolution when the value is produced by
  a load access.
- `clang-format` is not installed in this environment.

## Proof

Ran broader AArch64 backend proof exactly:

```sh
{ cmake --build build -j && ctest --test-dir build -R '^backend_aarch64_' -j 8 --output-on-failure; } > test_after.log 2>&1
```

Result: passed, 27/27. Proof log: `test_after.log`.

Then ran focused scalar subset exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_scalar_step2_00009_fix_subset.log 2>&1
```

Result: failed overall with 4/6 passing, as expected for the remaining owner.
`00009`, `00012`, `00056`, and `00211` passed; `00156` and `00161` failed
with runtime mismatch. Focused proof log:
`/tmp/c4c_aarch64_scalar_step2_00009_fix_subset.log`.

Additional check: `git diff --check` passed.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
