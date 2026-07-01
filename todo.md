Status: Active
Source Idea Path: ideas/open/503_rv64_before_return_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize The Before-Return Move

# Current Packet

## Just Finished

Step 2 implemented RV64 materialization for coherent prepared before-return
`return_stack_to_register` moves only.

The object emitter now accepts the narrow non-parallel-copy
`PreTerminatorCopies` shape with phase `BeforeReturn`, authority `None`,
destination kind `FunctionReturnAbi`, destination storage `Register`, op kind
`Move`, reason `return_stack_to_register`, a prepared GPR return
register/placement, and a prepared scalar stack-slot source home/layout. The
materializer emits a stack load into the prepared return GPR and suppresses the
terminator's duplicate generic stack return load only when the terminator's
named return value maps to the same prepared `(block_index, from_value_id)`
identity covered by the before-return move.

Focused coverage was added for the available stack-to-`a0` return path and for
fail-closed representatives covering missing source home, non-stack source,
non-GPR destination bank, missing destination register, unsupported source
type/size, destination kind confusion, before-instruction phase,
out-of-SSA authority, immediate source, parallel-copy step source, and the
regression where a valid before-return move for one scalar stack value appears
in a block whose terminator returns a different named scalar stack value.

Durable evidence:

- `build/agent_state/503_step2_before_return_stack_to_register/summary.md`

## Suggested Next

Execute Step 3 as the residual disposition packet for idea 503. Record whether
the active plan can close, extend, or route a follow-up owner after the narrow
before-return stack-to-register materialization landed.

## Watchouts

- Direct-global pointer returns remain handled by their existing authority path;
  this packet does not implement FPR, select-publication, before-instruction,
  or out-of-SSA materialization.
- The Step 2 materializer deliberately consumes prepared move-bundle facts and
  prepared source/destination homes; duplicate return-load suppression must
  remain keyed to prepared value identity, not only to block shape, raw return
  shape, or target-register inference.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output is preserved in `test_after.log`.
