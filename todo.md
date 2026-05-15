Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Allocate Atomic Loop Printer Facts In Selection

# Current Packet

## Just Finished

Step 6 `Allocate Atomic Loop Printer Facts In Selection` now makes the
prepared-to-selected AArch64 atomic route allocate explicit reserved-MIR scratch
facts for printable loop records. RMW records carry a structured new-value
scratch register plus exclusive-store status register, boolean compare-exchange
records carry a loaded-value scratch register plus status register, and
old-value compare-exchange records reuse the result register for the loaded
value while still carrying status authority.

Selection now rejects selected atomic loop records that lack these required
printer facts before the printer sees them. The real prepared atomic route is
covered by dispatch proof that RMW and compare-exchange records preserve the
pointer/input/expected/desired/result/status/loaded-value register authorities
and print without fixed archived scratch-register assumptions.

## Suggested Next

Proceed to Step 7 `Prove Atomic Route And Decide Lifecycle`; supervisor should
run or delegate the lifecycle proof/closure decision against the source idea.

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Preserve ordinary volatile memory behavior separately from atomic behavior;
  atomic selection must continue to require carrier provenance.
- Atomic loop scratch facts are selected from structured reserved-MIR scratch
  register authority and collision-checked against prepared operand/result
  registers; if the reserved scratch facts cannot be published, selection fails
  closed instead of fabricating a loop.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Implementation proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green for Step 6.

Proof log: `test_after.log`.
