# 02. Semantic Authority And Fact Classes

Status: Step 1 skeleton

## Assigned Purpose

Classify which pointer/address facts authorize a value for a use and which
facts are only supporting evidence, target-consume facts, route proofs, or
diagnostic artifacts.

## Required Role Vocabulary

- Semantic authority: required to decide whether a pointer/address value is
  current for a specific use.
- Verifier/support fact: can prove route coherence but cannot authorize use by
  itself.
- Target-consume fact: needed by a backend after semantic authority has
  already been established.
- Route proof: supports diagnostics, review, or debugging.
- Diagnostic-only artifact: observational only and not semantic authority.

## Required Output Shape

For each family inventoried in
`01_pointer_address_family_inventory.md`, record:

| Family | Semantic authority rule | Support facts | Target-consume facts | Route proofs | Diagnostic-only facts | Deferred gaps |
| --- | --- | --- | --- | --- | --- | --- |
| TBD | TBD | TBD | TBD | TBD | TBD | TBD |

## Step 1 Boundary

This skeleton reserves the classification structure only. It intentionally
does not make Step 3 authority decisions.
