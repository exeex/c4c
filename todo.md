Status: Active
Source Idea Path: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Indirect-Call Preservation Owner

# Current Packet

## Just Finished

Lifecycle switch from umbrella idea 295 to focused idea 309. The new active
owner is the `00189.c` AArch64 indirect function-pointer callee and argument
preservation failure across nested call setup.

## Suggested Next

Start Step 1 from `plan.md`: inspect the generated `00189.c` indirect-call
sequence and identify the backend call-lowering surface that owns callee and
outer-call argument preservation across nested call setup.

## Watchouts

- Do not broaden this owner into direct multi-argument shuffle
  (`00181.c`/`00182.c`), direct vararg aliasing (`00200.c`), or
  address-of-local direct-call argument preparation (`00218.c`).
- Do not reopen idea 308 unless generated assembly again uses direct non-PIC
  relocation forms for externally binding data symbols.
- Do not change expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, proof logs, or test contracts.
- Reject filename-only, instruction-string-only, `stdout`-only, or
  `fprintfptr`-only fixes.

## Proof

No build or tests were run for this lifecycle-only switch. The split uses the
completed Step 2 runtime-neighbor comparison from the previous `todo.md`.
