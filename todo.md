# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extend Across Control And Boundary Consumers

# Current Packet

## Just Finished

Step 3 repaired the focused fused-compare branch/control immediate publication
primitive. The prepared control-flow facts for both remaining loop
representatives carried authoritative scalar immediate compare operands, but
AArch64 fused-compare printing rebuilt nonzero immediates with
`immediate_bits=0`, so generated control flow compared against zero.

`src/backend/mir/aarch64/codegen/comparison.cpp` now routes fused-compare
immediate operands through the shared scalar immediate builder, preserving the
semantic immediate value and source identity instead of reconstructing a stale
unsigned immediate locally.

For `src/00156.c`, prepared control flow says:

```text
branch_condition for.cond.1 kind=fused_compare condition=%t1 compare=sle i32 %t0, 10
```

Generated AArch64 now emits the authoritative loop bound and `00156.c` passes:

```asm
ldr w13, [sp]
cmp w13, #10
b.le .LBB89_3
```

For `src/00161.c`, prepared control flow says:

```text
branch_condition dowhile.cond.1 kind=fused_compare condition=%t9 compare=slt i32 %t8, 100
```

Generated AArch64 also now emits `cmp w13, #100`, advancing the branch
predicate from stale zero. `00161.c` still times out because the loop-carried
scalar `a = t + p` publication is missing before the compare: the generated
body loads `t` and `p`, but does not publish/store the `%t6 = add %t4, %t5`
result to local `a`.

## Suggested Next

Continue Step 3 with `src/00161.c` as a loop-carried scalar publication packet:
repair prepared scalar ALU results whose authoritative storage is a frame slot
feeding a local store/control value. The current evidence centers on `%t6 =
bir.add i32 %t4, %t5`, prepared as `storage %t6 frame_slot ... offset=12`, with
move obligations immediately before `bir.store_local %lv.a, i32 %t6`.

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
- `src/00161.c` no longer has a stale branch immediate; its remaining timeout
  is the missing loop-carried scalar add publication for `%t6`, so the next
  packet should not keep reworking fused branch comparison unless this regresses.
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

Result: failed overall with 5/6 passing. `00009`, `00012`, `00056`, `00156`,
and `00211` passed. `00161` timed out after the branch immediate advanced to
`#100`, exposing the remaining loop-carried scalar add publication gap. Proof
log: `test_after.log`.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
