Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Loop-2e Runtime Boundary Evidence

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and reset this execution scratchpad for
Step 1.

## Suggested Next

Execute Step 1: refresh BIR, prepared-BIR, ASM, object, disassembly, clang
runtime, and c4c RV64 runtime evidence for
`tests/c/external/gcc_torture/src/loop-2e.c`; preserve the completed `%t23`
producer/branch-authority facts and identify the first indirect-store or
postincrement writeback boundary.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not special-case `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.

## Proof

Lifecycle-only activation; no build or test proof required.
