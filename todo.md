Status: Active
Source Idea Path: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Callee and Argument Preservation

# Current Packet

## Just Finished

Step 2 stopped before implementation because the observed `00189.c` failure
cannot be repaired completely inside the delegated owned files without
recovering facts that were already lost before AArch64 call lowering.

Smallest blocking path:

- LLVM/LIR for `main` contains `%t2 = getelementptr [4 x i8], ptr @.str1, ...`
  and the outer indirect call passes `%t2`.
- `--dump-bir` for `main` has no `%t2` producer and keeps the outer indirect
  call as `%t5 = bir.call i32 %t0(ptr %t1, ptr %t2, i32 %t4)`.
- `--dump-prepared-bir --mir-focus-function main` therefore records arg 1 as
  `source_encoding=register source_value_id=8 source_reg=x20`, while
  `prepared-addressing` has no inst_index=4 string-constant materialization for
  `.str1`.
- The AArch64 owned surfaces can retarget available scalar producers and lower
  prepared address materializations, but they have no semantic way to recover
  that `%t2` is `.str1` once BIR/prepared facts publish it only as an ordinary
  register value with no producer.

I also found a separate callee-register consistency hazard: prepared storage
prints `%t0` as `reg=x21`, but ordinary non-GOT `load_global` lowering converts
callee-saved placements through the AArch64 ABI pool where placement slot 1 maps
to `x20`. That is why current assembly loads `fprintfptr` into `x20` while the
prepared indirect callee still names `x21`. That could be worked around in the
delegated AArch64 dispatch/call surfaces, but the missing `%t2` -> `.str1`
identity remains a hard blocker for completing `00189.c` semantically.

## Suggested Next

Supervisor should split or widen the next packet to the semantic BIR/string
argument preservation producer before returning to AArch64 call lowering. The
smallest next packet is to make indirect calls receive the same string-pointer
argument rewrite/materialization authority that direct calls already get from
the LIR `getelementptr @.str*` alias path, then rerun Step 2 against the updated
prepared call/addressing facts.

## Watchouts

- Do not repair the missing `.str1` argument by guessing from assembly text,
  `00189.c`, the argument index, or a one-string-constant heuristic in AArch64.
  The semantic identity must be present in BIR/prepared facts before the target
  call-boundary consumer lowers it.
- After the BIR/prepared string argument fact is repaired, re-check the
  callee-saved placement/name mismatch for AArch64 non-GOT `load_global`
  results; current `fprintfptr` loading disagrees with the prepared indirect
  callee register.
- Do not broaden this owner into direct multi-argument shuffle
  (`00181.c`/`00182.c`), direct vararg aliasing (`00200.c`), or
  address-of-local direct-call argument preparation (`00218.c`).
- Do not reopen idea 308 unless generated assembly again uses direct non-PIC
  relocation forms for externally binding data symbols.
- Do not change expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, proof logs, or test contracts.
- Reject filename-only, instruction-string-only, `stdout`-only, or
  `fprintfptr`-only fixes.
- Existing local coverage proves pieces, not this failure shape:
  `backend_prepare_frame_stack_call_contract_test.cpp` has indirect-call
  publication, cross-call preservation, stack preservation, and variadic
  nested dynamic-stack contracts; AArch64 MIR tests retain prepared call
  provenance and preserved-value effects. I did not find a local AArch64 test
  that combines an indirect callee, outer arguments live across an inner call,
  and final indirect variadic call ABI setup.
- Treat the `[sp, #8]` `w2` load as a red flag: that offset is also used for
  the prologue save/restore of `x21` in the current assembly, so Step 2 should
  verify whether the nested result's prepared home is missing, stale, or
  overlapping saved-register storage before choosing the final repair.

## Proof

Blocked before code changes, so the delegated build/CTest proof was not run and
`test_after.log` was not updated. Evidence used: AST-backed lookup via
`c4c-clang-tool-ccdb` on the delegated owned C++ surfaces, `--dump-bir`,
`--dump-prepared-bir --mir-focus-function main`, current generated assembly for
`00189.c`, and the captured `test_before.log`.
