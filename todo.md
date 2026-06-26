Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Instruction-Fragment Bucket After Child Closures

# Current Packet

## Just Finished

Step 1 refreshed the current 395 instruction-fragment bucket after the
corrected 407 and reclosed 403 child work.

Representative statuses:

- `20000223-1`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `divmod-1`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `20000412-6`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `20000412-4`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `20000622-1`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `20000523-1`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.
- `20000801-1`: `c4cll_status=0`, `prepared_status=0`, no diagnostic.

Classification:

- No seed currently reproduces `unsupported_instruction_fragment`.
- No first owned 395 instruction-fragment opcode/family can be named from this
  refreshed set because every selected representative now succeeds through the
  delegated `c4cll --target riscv64-linux-gnu` command.
- The stale same-bucket seeds in this packet appear resolved by earlier child
  producer fixes or other already-landed work; they are not current 395-owned
  implementation targets.
- `divmod-1` does not regress 403/407 facts: direct `I16` formals publish
  `encoding=register bank=gpr reg=a0/a1`, same-module frame-slot `I16` call
  arguments retain `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`,
  `dest_bank=gpr`, and `missing_frame_slot_arg_publication=yes`, and the old
  `source_encoding=frame_slot ... dest_bank=none`, `call_arg_stack_to_stack`,
  `placement=none:call_argument`, and
  `encoding=register bank=none reg=aN` shapes are absent.

## Suggested Next

Supervisor should route 395 to plan-owner lifecycle review. Based on this
Step 1 refresh, 395 appears closure-ready for the refreshed seed bucket unless
the supervisor wants broader RV64 torture validation before closure.

## Watchouts

- Do not reopen 403 unless fresh prepared dumps again show direct `I16`
  formals with `encoding=register bank=none reg=aN`.
- Do not reopen 407 unless fresh prepared dumps again show the old same-module
  `I16` caller-side producer regression.
- Route producer-fact gaps to their owners instead of patching them under 395.
- If broader validation later discovers a new `unsupported_instruction_fragment`,
  classify it from fresh artifacts before assuming it belongs to this now-stale
  bucket.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_refresh_step1 && for case in 20000223-1 divmod-1 20000412-6 20000412-4 20000622-1 20000523-1 20000801-1; do src="tests/c/external/gcc_torture/src/${case}.c"; (build/c4cll --target riscv64-linux-gnu "$src" > "build/agent_state/395_refresh_step1/${case}.out" 2> "build/agent_state/395_refresh_step1/${case}.err"; printf 'c4cll_status=%s\n' "$?" > "build/agent_state/395_refresh_step1/${case}.status"); (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/agent_state/395_refresh_step1/${case}.prepared.txt" 2> "build/agent_state/395_refresh_step1/${case}.prepared.err"; printf 'prepared_status=%s\n' "$?" > "build/agent_state/395_refresh_step1/${case}.prepared.status"); done && for case in 20000223-1 divmod-1 20000412-6 20000412-4 20000622-1 20000523-1 20000801-1; do printf '== %s ==\n' "$case"; cat "build/agent_state/395_refresh_step1/${case}.status" "build/agent_state/395_refresh_step1/${case}.prepared.status"; rg -n 'unsupported_instruction_fragment|unsupported|error|fatal|bir\.|bank=none|bank=gpr|source_encoding=frame_slot.*dest_bank=none|call_arg_stack_to_stack|placement=none:call_argument|encoding=register bank=none reg=a[0-9]' "build/agent_state/395_refresh_step1/${case}.out" "build/agent_state/395_refresh_step1/${case}.err" "build/agent_state/395_refresh_step1/${case}.prepared.txt" "build/agent_state/395_refresh_step1/${case}.prepared.err" || true; done && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: command completed successfully. All seven seed cases reported
`c4cll_status=0` and `prepared_status=0`; all `.err` and `.prepared.err`
artifacts are empty. `test_after.log` is preserved and green with
`100% tests passed, 0 tests failed out of 326`.
