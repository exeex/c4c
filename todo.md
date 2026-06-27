Status: Active
Source Idea Path: ideas/open/409_prepared_packed_fp128_global_initializer_admission.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representative And Residual Ownership

# Current Packet

## Just Finished

Step 3 proved the accepted packed integer aggregate initializer repair remains
effective and classified the current representative residual.

- `c4cll_status=0`.
- `prepared_status=1`.
- The old bootstrap packed/global admission diagnostic remains absent. The
  proof did not report `bootstrap global admission`, `only scalar
  integer/pointer globals`, `linear integer-array globals`,
  `aggregate-backed globals`, or `honest byte-address semantics`.
- The object-route compile completed successfully. The prepared dump still
  fails before prepared handoff.
- First diagnostic:
  `error: backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff: semantic lir_to_bir failed outside the currently admitted capability buckets ... latest function failure: semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family`.

Residual classification:

- `fn1A` is a semantic function-lowering residual, not a 409 producer global
  initializer residual. The first failing function is emitted after globals
  have been accepted and contains bitfield-style scalar operations over a
  packed `%struct.A`: `load i16`, `lshr i16`, `and i16`, `zext i16 to i32`,
  `add i32`, `trunc i32 to i16`, `shl i16`, `or i16`, and `store i16`.
- The residual does not prove a remaining packed or `fp128` global initializer
  admission gap. It should route to the semantic scalar-binop/local-memory
  lowering owner or be split into a new producer semantic-lowering initiative
  if no active/open idea owns that family.
- 409 appears closure-ready for plan-owner review for the first supported
  packed integer aggregate initializer family: the committed focused test
  covers packed integer aggregate storage facts, and the representative no
  longer stops first on the packed/global admission boundary.

## Suggested Next

Ask the plan owner to review 409 for close/deactivation handling. Route the
fresh `fn1A` scalar-binop pre-handoff residual separately, outside 409, because
it is a semantic function-lowering boundary after packed global admission.

## Watchouts

- Do not edit `src/backend/riscv/rv64/object_emission.cpp`; this packet is a
  producer/global admission lifecycle proof.
- Do not absorb `fn1A` scalar-binop lowering into 409 unless a future diagnostic
  proves it is still a packed/`fp128` global initializer admission problem.
- `20040709-2.c` still cannot produce a prepared dump because of the separate
  `fn1A` semantic scalar-binop boundary, so fresh prepared global-storage lines
  from this representative are unavailable until that boundary is handled.
- No code changes, expectation rewrites, unsupported downgrades, allowlist
  filtering, target inference, or filename-specific handling were used in this
  Step 3 packet.

## Proof

Exact delegated proof command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/409_step3_packed_global_proof && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step3_packed_global_proof/20040709-2.out 2> build/agent_state/409_step3_packed_global_proof/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/409_step3_packed_global_proof/20040709-2.status) && (build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.txt 2> build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.err; printf 'prepared_status=%s\n' "$?" > build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.status) && cat build/agent_state/409_step3_packed_global_proof/20040709-2.status build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.status && rg -n 'bootstrap global admission|only scalar integer/pointer globals|linear integer-array globals|aggregate-backed globals|honest byte-address semantics|semantic lir_to_bir function|fn1A|scalar-binop|fp128|fpext|fcmp|unsupported|error|fatal|global|packed|<\{|%struct\.|prepared|handoff' build/agent_state/409_step3_packed_global_proof/20040709-2.out build/agent_state/409_step3_packed_global_proof/20040709-2.err build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.txt build/agent_state/409_step3_packed_global_proof/20040709-2.prepared.err || true && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `c4cll_status=0`.
- `prepared_status=1` with the `fn1A` scalar-binop diagnostic above.
- Old packed/global admission diagnostic absent.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
