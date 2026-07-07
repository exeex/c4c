Status: Active
Source Idea Path: ideas/open/580_rv64_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement Semantic Compare Publication

# Current Packet

## Just Finished

Finished the `plan.md` Step 3 follow-up for the simple representative gap:
ordinary F32/F64 `eq`/`ne` compare publication now accepts a materializable
floating zero immediate operand in addition to prepared FPR-home operands.

The RV64 object-emission repair is semantic: it recognizes zero F32/F64 BIR
immediates by type and immediate bits, materializes zero through a scratch GPR
and scratch FPR, then emits the existing `feq.s`/`feq.d` plus `xori` path for
`ne`. It does not match testcase names, source file names, route logs, ordered
compares, or arbitrary nonzero floating constants.

Focused coverage now includes an F32 compare publication fixture matching the
`%lhs != 0.0f` shape that blocked `src/20080529-1.c`.

## Suggested Next

Rerun `plan.md` Step 4 representative route proof for `src/20080529-1.c` and
`src/loop-8.c` to confirm the simple route now advances past
`unsupported_scalar_compare_publication` and to re-record the current later
owner, if any.

## Watchouts

- Zero-immediate materialization requires an available temporary GPR; fully
  occupied temporary-GPR shapes still fail closed.
- The implementation intentionally stays limited to F32/F64 `eq`/`ne`
  publication. Ordered compares and nonzero floating immediates remain outside
  this packet.
- The previous Step 4 proof found `src/loop-8.c` already advanced to a later
  pre-terminator move-bundle owner; this packet did not rerun per-case routes.

## Proof

Command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed. The focused RV64 object-emission subset ran
`backend_riscv_object_emission`, 1 test, 0 failed. Proof log:
`test_after.log`.
