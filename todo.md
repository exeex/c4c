# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Focused 22-Case Family

# Current Packet

## Just Finished

Step 4 classified the remaining 21 focused c-testsuite failures after the
immediate-left repair. The accepted focused baseline in `test_before.log` still
has 6 passed / 21 failed, and every residual failure still reaches
`opcode=compare_branch: fused compare branch operands are not printable`.

Narrow `--dump-bir` / `--dump-prepared-bir` diagnostics show the residual cases
split into these semantic operand-form subfamilies:

- Constant-vs-constant fused compares still marked `can_fuse_with_branch=yes`:
  `00034`, `00037`, `00038`, `00054`, `00055`, `00057`, `00059`, `00076`,
  `00077`, `00085`, `00092`, `00093`, `00101`, `00127`, `00200`, `00207`,
  `00212`, `00214`, `00215`. Representative prepared records include
  `ne i32 1, 0`, `ne i64 1, 1`, `ult i64 4, 2`, `eq i64 2, 2`, and repeated
  `ne i32 0, 0` loop/logic guards. Several of these files contain later
  register-vs-immediate compares, but the first printer blocker is a
  constant-vs-constant compare branch.
- Non-encodable register-vs-immediate fused compares that need immediate
  materialization or non-fused fallback instead of direct `cmp #imm` printing:
  `00041` has `slt i32 %t0, 5000`; `00203` has
  `slt i64 %t1, -2147483648` and later `slt i64 2147483647, %t8`.
- Already-covered encodable register-vs-immediate forms are present as
  secondary branches in `00034`, `00037`, `00077`, `00092`, `00127`, `00200`,
  `00212`, `00214`, and `00215`; these are not the next blocker after the
  immediate-left repair.

## Suggested Next

Repair the constant-vs-constant fused compare branch publication rule first.
The highest-value packet is to stop publishing both-immediate compares as
directly printable fused compare branches: either fold the branch direction at
semantic/prepared authority, or keep the compare branch fail-closed and lower a
plain branch/control-flow form that the AArch64 printer can spell. This should
be implemented as a semantic operand-form rule, not by matching testcase names
or individual literal pairs.

## Watchouts

- The constant-vs-constant family is 19 of 21 residual failures, so it should
  stay in this idea as the next repair packet. It is not a separate lifecycle
  split unless inspection shows branch folding belongs to a different active
  backend owner.
- The non-encodable immediate family (`00041`, `00203`) is still inside fused
  compare-branch operand forms, but it is a distinct second implementation
  packet after constant-vs-constant publication is repaired.
- The current accepted repair intentionally does not constant-fold branch
  direction, materialize compare immediates, add scratch-register compare
  lowering, or mutate semantic compare records.
- Do not match c-testsuite filenames, test numbers, or exact emitted
  instruction strings.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.
- Keep this owner limited to fused compare-branch operand publication/printing;
  do not absorb the remaining scalar machine-printer, `lir_to_bir` admission,
  runtime, or timeout buckets from umbrella idea 295.
- Proving only one known failing case is not enough; the full 22-case focused
  family must be checked before claiming capability progress.

## Proof

Accepted baseline analyzed: `test_before.log` records 6 passed / 21 failed for
the focused scope after `00030` was repaired.

Narrow diagnostics used for classification only:
`./build/c4cll --dump-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/<case>.c`
and
`./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/<case>.c`
for representative residual cases. No broad backend regex was rerun, and no
root proof log was modified for this classification-only packet.
