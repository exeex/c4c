Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Cover Aggregate And Function-Pointer Local Flow

# Current Packet

## Just Finished

Step 5 evidence packet reprobed the aggregate-local and function-pointer local
candidate set under
`build/rv64_c_testsuite_probe_latest/triage_312_step5/` with emit -> clang ->
qemu and a 5s timeout per phase.

All 22 candidates emitted and linked. End-to-end qemu results were `13/22`
passing. Passing cases were `00018`, `00026`, `00032`, `00037`, `00039`,
`00043`, `00058`, `00072`, `00073`, `00078`, `00130`, `00137`, and `00138`.
The Step 3/4 non-regression probes `00032`, `00072`, and `00130` remained
green.

Remaining runtime failures:

- `00019`, `00046`, and `00140` are aggregate-local residuals. `00019` stops
  after publishing `s.p = &s` and before chained aggregate pointer loads;
  `00046` stops on nested union/struct field offset stores; `00140` stops at
  aggregate byval parameter/local copy and also carries vararg/float aggregate
  ABI surface. This is real aggregate-local evidence, but not one small
  coherent repair family.
- `00087` and `00124` are function-pointer residuals. `00087` stops before
  storing function address `@foo` into a local struct field; `00124` additionally
  needs returning a function address and an indirect call through the returned
  pointer. Repairing only `00087` would be testcase-shaped because the real
  function-pointer-use representative crosses into indirect-call/global
  function-address policy.
- `00005` remains a pointer-to-pointer local residual after the first pointer
  dereference, outside aggregate/function-pointer Step 5.
- `00077` is array parameter/local array pointer flow, outside aggregate and
  function-pointer Step 5.
- `00144` is pointer select/inttoptr/ptrtoint conditional flow, not local stack
  address materialization.
- `00143` remains broad indexed local array select/update plus switch-shaped
  flow and should stay follow-up evidence.

No code/test repair was made in this packet because the remaining in-scope
evidence splits across aggregate subobject access, aggregate byval ABI, and
function-pointer indirect-call behavior. There is no single small coherent
aggregate/function-pointer local materialization rule to implement without
widening the packet.

## Suggested Next

Run Step 6 acceptance/reclassification using the Step 5 sweep as the latest
candidate baseline. Recommend closure or follow-up split rather than a Step 5
local repair: aggregate subobject/byval handling and function-pointer indirect
call/function-address handling should be separate initiatives if the supervisor
wants them pursued.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- `00032` is now an end-to-end proof for local array pointer reassignment and
  pointer-step materialization.
- `00072` remains the clean end-to-end proof for base local address
  publication.
- `00130` is now an end-to-end proof for constant local subobject address
  publication, byte element loads, pointer-value byte loads, and narrow
  extension before branch compares.
- Avoid accepting a narrow `00087` function-address store as Step 5 progress
  without addressing `00124`; that would not prove function-pointer local use.
- Keep `00143` as indexed local array/select-update-chain follow-up evidence.

## Proof

Evidence-only proof run:

- Reprobe candidate set with `./build/c4cll --codegen asm --target
  riscv64-linux-gnu`, `clang --target=riscv64-linux-gnu`, and
  `QEMU_LD_PREFIX=/usr/riscv64-linux-gnu qemu-riscv64`, each with a 5s timeout.
- `build/rv64_c_testsuite_probe_latest/triage_312_step5/probe_results.tsv`
  records the sweep.
- Representative `.s`, `.bir.txt`, `.prepared-bir.txt`, and phase
  stdout/stderr files are under
  `build/rv64_c_testsuite_probe_latest/triage_312_step5/`.

No build, CTest, or `test_after.log` was produced because this was an
evidence-only packet with no code changes.
