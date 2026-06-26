Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Representative Prepared Publications

# Current Packet

## Just Finished

Continuation Step 1 traced the representative prepared publications for
`va-arg-13.c` and compared them with the focused Step 4 fixture. Both `dummy`
calls have explicit prepared `call_argument_value_publications` facts:

- call inst 9 arg0: argument `%t7` value id 15, object slot #6 offset 24,
  source store inst 8, payload `%t7.memcpy.copy.0` value id 14, destination
  slot #6 offset 24.
- call inst 16 arg0: argument `%t14` value id 19, object slot #7 offset 32,
  source store inst 15, payload `%t14.memcpy.copy.0` value id 18, destination
  slot #7 offset 32.

The facts are not absent, and their callsite/object identities match the
frame-slot-address call-plan rows. The boundary class is `wrong effective
payload`: each explicit payload is a `load_local` result from `%lv.state.8`,
and the matching store-source rows print `source_load_local=yes`. The focused
Step 4 fixture used direct register payload `%lv.src` without the representative
load-local payload shape, so it did not prove this case.

## Suggested Next

Execute Step 2 by classifying the load-local payload materialization rule for
explicit call-argument value-publication facts. The next owner should preserve
the payload value `%t7.memcpy.copy.0` / `%t14.memcpy.copy.0` rather than
reinterpreting it as the local `va_list` storage address `%lv.state.8`.

## Watchouts

- Do not hard-code `va-arg-13.c`, `test`, `dummy`, literal registers, stack
  offsets, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer saved into the backing area.
- Do not treat generic `StoreLocalPublication` records alone as callsite
  authority; Step 392 requires the explicit prepared
  `call_argument_value_publications` fact and its referenced source/destination
  records to agree.
- Keep the argument object address, the local `va_list` storage address, and
  the initialized save-area pointer payload separate in all notes and proof.
- The representative facts are valid enough to identify the callsite/object,
  so do not route this as an absent-fact or mismatched-fact problem. The live
  question is how an explicit load-local payload should be materialized without
  collapsing back to the source storage address.

## Proof

Delegated proof run:
`cmake --build --preset default --target c4cll && mkdir -p build/agent_state && build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/392_cont_step1_va-arg-13.prepared.log 2>&1`.

Result: passed; prepared dump captured. Analysis log:
`build/agent_state/392_cont_step1_va-arg-13.analysis.log`.
