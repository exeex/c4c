Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Instruction Fragments

# Current Packet

## Just Finished

Completed Step 1 - Classify Instruction Fragments for the delegated
representative packet. The five-case proof reproduced one
`unsupported_instruction_fragment` case, `src/20000223-1.c`, while the other
four representatives classify into adjacent non-owned buckets:
`unsupported_local_memory_access` (`src/20000722-1.c`,
`src/20010123-1.c`), semantic `lir_to_bir` pre-handoff failure
(`src/20010605-2.c`), and `unsupported_global_data`
(`src/20010924-1.c`).

First reusable fragment family: ordinary prepared `bir.call` lowering for
same-module/direct fixed-arity void calls whose GPR ABI arguments include a
PC-relative global symbol address and a small integer immediate. The concrete
representative is `tests/c/external/gcc_torture/src/20000223-1.c`, where
`main` contains repeated `bir.call void check(ptr @.strN, i32 K)` callsites.
Prepared facts for the first call include `block=0 inst=0`,
`wrapper=same_module`, `callee=check`, `arg0 bank=gpr
from=symbol_address:@.str0 to=a0`, `arg1 bank=gpr from=immediate:8 to=a1`,
and no result. Later same-family callsites add call-preservation pressure for
global pointer homes, including callee-saved GPR homes and no-op stack-slot
preservation effects.

Observed artifacts:
`test_after.log`,
`build/rv64_gcc_c_torture_backend/src_20000223-1.c/case.log`,
`build/agent_state/395_step1_classification.txt`,
`build/agent_state/395_step1_representatives.allowlist.txt`,
`build/agent_state/395_step1_20000223-1.prepared-bir.txt`, and
`build/agent_state/395_step1_20000223-1.semantic-bir.txt`.

## Suggested Next

Execute Step 2 on the `src/20000223-1.c` call-fragment family: add a focused
diagnostic/probe if needed to locate the exact `fragment_for_prepared_call()`
nullopt, then repair same-module/direct fixed-arity void calls with GPR
symbol-address and immediate arguments. Keep the repair semantic and fail
closed for unsupported banks, aggregate transports, stack arguments, relocation
policy, and non-scalar forms.

## Watchouts

- Do not match on testcase filenames or source-specific constants.
- Do not weaken gcc_torture expectations, supported-path contracts, or
  allowlists.
- Do not reconstruct BIR control/data-flow ownership inside RV64 object
  emission.
- Split producer-side missing facts into a separate idea instead of hiding them
  in target lowering.
- Current source already contains a `SymbolAddress` argument branch in
  `fragment_for_prepared_call()`, while the proof still fails with the generic
  instruction-fragment diagnostic. The next packet should confirm whether the
  rejection is symbol-address materialization, preservation effects, call fixup
  emission, or a prepared-contract mismatch before changing lowering behavior.

## Proof

Ran the delegated proof exactly. `test_after.log` exists and records
`[rv64-gcc-torture] total=5 passed=0 failed=5`; the required `rg` output shows
the `unsupported_instruction_fragment`/`unsupported RV64 object lowering`
diagnostic for `src/20000223-1.c` and the runner summary. Additional
classification dumps were generated with `--dump-prepared-bir` and
`--dump-bir` for `src/20000223-1.c` under
`build/agent_state/395_step1_20000223-1.*`.
