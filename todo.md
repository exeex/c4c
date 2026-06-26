Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm The Bankless I16 Formal Register Gap

# Current Packet

## Just Finished

Lifecycle repaired after active 395 Step 1 showed the next implementation
target must not be RV64 opcode lowering. `src/divmod-1.c` fails first at
`bir.sext i16 %p.x to i32` because callee formal `%p.x` is physically in `a0`
but prepared storage publishes `encoding=register bank=none reg=a0`.

Reopened `ideas/open/403_prepared_i16_formal_abi_publication.md` because this
live producer-side `I16` formal ABI/home metadata gap still matches its source
intent. Corrected 407 caller-side facts remain complete, and
`src/20000223-1.c` passes.

## Suggested Next

Delegate Step 1 to an executor: identify the producer helper/function family
that should publish GPR bank facts for direct `I16` formals in ABI argument
registers, then record the exact proof command here.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer GPR bank inside RV64 object emission from `reg=a0`, parameter
  order, or formal type.
- Do not reopen 407 unless fresh prepared dumps show the old same-module
  caller-side `i16` producer regression.

## Proof

Lifecycle repair only. Evidence came from the active 395 Step 1 `todo.md`
record; no new build or test was run by the plan owner.
