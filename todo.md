Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate Label Address Materialization

# Current Packet

## Just Finished

Lifecycle review after commit `90640a317`: the source idea is not complete
because label, GOT, and TLS materialization remain in scope. `plan.md` now
records the direct page+low12 global/string-constant printer milestone as
completed Step 4 and splits the remaining work into executable Steps 5-9.

## Suggested Next

Execute Step 5 from `plan.md`: identify why label address materialization is
still unpopulated or deferred, then populate/select structured label page+low12
facts without changing the completed direct global/string-constant path except
for shared coherence.

## Watchouts

- Do not close idea 233 until label, GOT, and TLS source-idea proof direction
  is satisfied or deliberately retired by a separate lifecycle decision.
- Label should reuse structured relocation operands rather than symbol-text
  templates.
- GOT policy must remain explicit and must not be inferred from rendered names.
- TLS records currently fail closed at printer time; keep that behavior until
  TLS model and thread-pointer-relative relocation facts are complete.

## Proof

Lifecycle-only plan/todo review. No implementation proof was required. The
accepted Step 4 implementation milestone proof has been rolled forward to
`test_before.log`: 139 backend tests passed.
