Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within The Same Fragment Family

# Current Packet

## Just Finished

Step 3 from `plan.md` refreshed the residual diagnostics after commit
`2964b8ea5` (`Lower RV64 narrow add-immediate fragments`). The existing
add-immediate implementation remains a semantic same-family consumer: i8/i16
`bir.add` with immediate RHS `+1` or `-1`, complete prepared homes, narrow
wraparound masking, and GPR or stack publication.

Current direct RV64 gcc_torture backend-object add-immediate movement:

- `src/921123-1.c`: passed; moved past `%t2 = bir.add i16 %t1, -1`.
- `src/930208-1.c`: passed; moved past `%t3 = bir.add i8 %t2, 1` and
  `%t7 = bir.add i8 %t6, 1`.
- `src/961017-1.c`: passed; moved past `%t2 = bir.add i8 %t1, -1`.
- `src/doloop-1.c`: passed; moved past `%t4 = bir.add i8 %t3, -1`.
- `src/doloop-2.c`: passed; moved past `%t4 = bir.add i16 %t3, -1`.
- `src/pr20100-1.c`: passed; moved past `%t21 = bir.add i16 %t20, 1`.
- `src/pr39233.c`: passed; moved past `%t7 = bir.add i16 %t6, -1`.
- `src/pr59387.c`: passed; moved past `%t15 = bir.add i8 %t14, -1`.
- `src/pr59747.c`: passed; moved past `%t9 = bir.add i16 %t8, -1`.
- `src/pr69320-1.c`: passed; moved past `%t11 = bir.add i8 %t10, 1`.
- `src/pr69320-3.c`: passed; moved past `%t7 = bir.add i16 %t6, 1`.
- `src/pr77766.c`: passed; moved past `%t31 = bir.add i16 %t30, 1`.

Refreshed stale `BinaryInst` residual classification:

- Same narrow integer family: only `src/931110-1.c` remains as scalar integer
  `BinaryInst`, owner `i16 %t13.bf.sext`; this is `ashr`, not add-immediate.
- Pointer `BinaryInst` guards remain outside this packet: `src/930526-1.c`
  owner `ptr %t10`, `src/20000801-1.c` owner `ptr %t2`,
  `src/20050502-1.c` owner `ptr %t7`, `src/20120808-1.c` owner `ptr %t210`,
  `src/960327-1.c` owner `ptr %t86`, `src/961125-1.c` owner `ptr %t3`,
  `src/961213-1.c` owner `ptr %t12`, `src/990811-1.c` owner `ptr %t12`,
  `src/builtin-prefetch-4.c` owner `ptr %t1`, `src/loop-13.c` owner
  `ptr %t16`, `src/loop-2f.c` owner `ptr %t10`, `src/loop-2g.c` owner
  `ptr %t2`, `src/mode-dependent-address.c` owner `ptr %t17`,
  `src/pr20527-1.c` owner `ptr %t10`, `src/pr36038.c` owner `ptr %t7`,
  `src/pr41395-2.c` owner `ptr %t3`, `src/pr41463.c` owner `ptr %t3`,
  `src/pr43560.c` owner `ptr %t14`, `src/pr59643.c` owner `ptr %t9`,
  `src/pr62151.c` owner `ptr %t3`, `src/ssad-run.c` owner `ptr %t21`,
  `src/strct-pack-3.c` owner `ptr %t9`, and `src/usad-run.c` owner
  `ptr %t21`.
- Mixed/out-of-family guards from the stale binary list now stop elsewhere:
  `src/pr84524.c` at inline asm policy, `src/20150611-1.c` at `CastInst`
  owner `i32 %t28`, `src/pr17252.c` at terminator lowering,
  `src/pr42614.c` at ambiguous stack-source consumer classification,
  `src/pr58277-2.c` at move-bundle target shape, and `src/strcpy-1.c` at
  branch stack-load source freshness.

This refresh does not identify a bounded same-family adjacent implementation
packet larger than a testcase-shaped one-row `ashr` slice.

## Suggested Next

Recommended next packet: move to Step 4 close-readiness/split classification.
Do not take a Step 3 same-family implementation packet unless fresh diagnostics
find more positive `ashr` rows or a shared narrow integer shift abstraction with
real non-testcase breadth. Current positive row for such a hypothetical packet:
only `src/931110-1.c`; negative guards are the pointer `BinaryInst` rows and
the mixed/out-of-family rows listed above.

## Watchouts

- `src/931110-1.c` is a real scalar integer `BinaryInst` residual, but it is a
  singleton `ashr` row in this refresh; implementing it alone would be a
  testcase-shaped micro-slice.
- Pointer `BinaryInst` rows require pointer/address authority decisions and
  should not be folded into narrow integer add/shift lowering.
- Cast, call, select, inline asm, terminator, move-bundle, ambiguous stack
  source, and branch freshness rows remain separate owners for Step 4
  classification.

## Proof

Ran delegated proof command exactly and preserved the output in `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` reports total real test time `1.96 sec`.

Focused direct probes were classification-only and used temporary object files
outside the repo. The previous direct probe log for the original five positives
and guard rows remains available at
`build/agent_state/612_step3_add_immediate_direct_probes.log`.
