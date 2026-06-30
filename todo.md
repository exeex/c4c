Status: Active
Source Idea Path: ideas/open/444_no_external_caller_formal_authority_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit No-External-Caller Authority Inputs

# Current Packet

## Just Finished

Activated `ideas/open/444_no_external_caller_formal_authority_producer.md` as
the next active plan. Selection rationale: it is the required producer
follow-up from closed idea 443 before parked idea 442 can safely publish
external-linkage formal pointer provenance.

## Suggested Next

Execute Step 1: audit frontend, LIR, module, linkage, visibility, and callgraph
facts to determine whether any can prove no-external-caller or equivalent
closed-world authority for non-internal callee formals.

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module callsites alone.
- Do not treat hidden/protected visibility, same-module wrappers, `LinkNameId`,
  or `can_elide_if_unreferenced` as no-external-caller authority without a
  tested contract.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep `%mr_TR - 8` pointer-delta propagation behind proven base formal pointer
  authority.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
