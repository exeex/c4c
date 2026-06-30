Status: Active
Source Idea Path: ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Scalar Arg ABI Producer And Contract Surface

# Current Packet

## Just Finished

Activated `ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md` as the
next active plan. Selection rationale: 434 is the immediate producer-contract
repair split from the closed 431 review; 435 depends on clean producer facts,
and 436 is a narrower `pr88904` review.

## Suggested Next

Execute Step 1: audit where scalar call arguments receive `CallArgAbiInfo`, and
choose the first bounded implementation packet for raw scalar-vs-aggregate ABI
coherence.

## Watchouts

- Do not implement RV64 aggregate lowering in this plan.
- Do not rely on later prepared call-plan scalar repair as proof that raw BIR
  `arg_abi` is coherent.
- Do not special-case representative testcase names, function names, or exact
  bogus alignment literals.
- Preserve coherent aggregate `sret`/`byval` pointer metadata.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
