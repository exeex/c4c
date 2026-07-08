Status: Active
Source Idea Path: ideas/open/611_rv64_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within Authorized Terminator Families

# Current Packet

## Just Finished

Step 3 from `plan.md` refreshed direct probes for the remaining RV64
`unsupported_terminator_fragment` residual rows without changing
implementation, tests, expectations, unsupported markers, or allowlists.

Direct object probes still report `unsupported_terminator_fragment` for all five
rows:
- `src/20030910-1.c`
- `src/ieee/20001122-1.c`
- `src/921124-1.c`
- `src/920710-1.c`
- `src/991030-1.c`

Prepared terminator evidence is present for all five rows, so none classify as
missing prepared branch-condition/control-flow evidence:
- `src/20030910-1.c`: prepared control flow has one `fused_compare`
  conditional branch, `ne double %t4, 0x40091EB851EB851F`, with register homes
  for the FPR input and GPR predicate. The residual is an unsupported RV64
  object-route terminator consumer shape, not missing terminator facts.
- `src/ieee/20001122-1.c`: prepared control flow has two `fused_compare`
  conditional branches over double values, including a loop backedge and a
  global-load compare. The prepared branch evidence is explicit, but
  `--codegen asm` stops earlier at unsupported prepared global storage layout,
  so the object-route residual should not be treated as a clean standalone
  terminator implementation packet.
- `src/921124-1.c`: prepared control flow has one double `fused_compare`
  branch followed by integer fused-compare short-circuit branches. It also
  records join/select carrier gaps, missing publication for parameter-carried
  join sources, and one available branch stack-load authority row. This is an
  unsupported mixed select/join and stack-backed condition consumer shape, not
  missing prepared terminator evidence.
- `src/920710-1.c`: prepared control flow has double, integer, and float
  `fused_compare` conditional branches plus predecessor-terminator parallel
  copy authority for the join feeding the integer branch. This is an unsupported
  mixed FP/integer compare plus join consumer shape, not missing terminator
  evidence.
- `src/991030-1.c`: prepared control flow has one `fused_compare` conditional
  branch, `ne double %t0, 0x400F000000000000`, with direct-global select-chain
  evidence. As with `src/ieee/20001122-1.c`, `--codegen asm` stops earlier at
  unsupported prepared global storage layout, so this belongs with global-data
  or global-storage classification before another terminator packet.

No single Step 3 implementation family emerged with explicit prepared operands
and control-flow authority that would avoid crossing into global storage,
join/select publication, stack-backed condition, or mixed integer-branch
consumer ownership.

## Suggested Next

Step 4 close-readiness classification packet:

Treat the remaining five direct object residuals as classified unsupported
consumer shapes rather than a single authorized Step 3 terminator family, and
decide whether the plan should close, split, or activate a new source idea for
global-storage and join/select/stack-condition follow-up work.

## Watchouts

- Direct object probes alone keep the five rows under
  `unsupported_terminator_fragment`, but prepared-BIR and asm probes show mixed
  root ownership. Do not claim another Step 3 implementation packet unless the
  supervisor intentionally chooses one semantic subfamily.
- `src/ieee/20001122-1.c` and `src/991030-1.c` have explicit fused-compare
  branch conditions, but RV64 asm emission currently stops at unsupported
  prepared global storage layout.
- `src/921124-1.c` and `src/920710-1.c` include join/select or
  predecessor-terminator parallel-copy evidence; keep these out of a narrow
  floating compare branch packet unless that packet explicitly owns the join
  publication and branch-condition consumer shape.
- The direct probe path is `build/c4cll`, not `build/src/apps/c4cll`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. Test subset: `^backend_`. Proof log: `test_after.log`.

Focused diagnostic probes used direct `build/c4cll --codegen obj --target
riscv64-linux-gnu -o <tmp>.o`, `--dump-prepared-bir`, and targeted
`--codegen asm` commands against the five GCC torture source paths under
`tests/c/external/gcc_torture/`. Object probes confirmed all five still report
`unsupported_terminator_fragment`; prepared-BIR probes provided the
classification evidence above.
