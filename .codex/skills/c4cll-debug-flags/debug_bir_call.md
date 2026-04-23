# Debug BIR Call

Use this note when the question is "did call ownership and ABI placement come
out right?".

Start with:

```bash
./build/c4cll --dump-bir --target <triple> <file>
./build/c4cll --dump-prepared-bir --target <triple> --mir-focus-function <function> <file>
```

## What To Read

`--dump-bir`:

- `bir.call`
- `arg_abi`
- `result_abi`

`--dump-prepared-bir`:

- `--- prepared-call-plans ---`
- `--- prepared-value-locations ---`
- `--- prepared-storage-plans ---`
- `--- prepared-frame-plan ---` when calls interact with callee-saved policy

## Correctness Questions

Ask these in order:

1. Does semantic BIR publish the right call ABI class for args and results?
2. Does prepared BIR publish the per-call argument and result placement plan?
3. Do call-crossing values have stable homes that survive the call boundary?
4. Is clobber information visible before x86 emission?

## Good Signals

Semantic BIR:

- `bir.call` exists with the expected callee and argument list
- `arg_abi` records register-vs-stack placement intent
- `result_abi` records the return ABI class

Prepared BIR:

- `prepared-call-plans` has one `call` entry per callsite
- each `arg` shows source storage plus destination register or stack offset
- `result` shows source register plus final destination
- `value_bank` and register-bank fields distinguish `gpr`, `fpr`, and `vreg`
- call clobbers are visible before x86 lowering

Cross-call stability:

- `prepared-value-locations` / `prepared-storage-plans` make it clear where
  important values live across the call
- if callee-saved usage matters, `prepared-frame-plan` shows the saved-register
  contract explicitly

## Bad Signals

- call ABI can only be inferred from final x86 text
- prepared call plans are missing even though semantic BIR clearly has calls
- banks are collapsed so GPR/FPR/VREG differences are hidden
- the only way to understand call-crossing safety is to inspect x86 save and
  restore code manually

## Practical Reading Pattern

1. read `bir.call`
2. read `prepared-call-plans`
3. read `prepared-value-locations`
4. read `prepared-storage-plans`
5. read `prepared-frame-plan` if saved registers or call-crossing preservation
   matter

If `bir.call` already has the wrong ABI shape, do not debug x86 first. Fix the
semantic route or prealloc contract producer first.

## Example: Semantic Call ABI Looks Healthy

Command:

```bash
./build/c4cll --dump-bir \
  --target x86_64-unknown-linux-gnu \
  tests/c/external/c-testsuite/src/00204.c | \
  rg 'bir\.call|arg_abi|result_abi|bir\.func @stdarg|bir\.func @myprintf'
```

Healthy output usually contains call lines such as:

```text
bir.func @stdarg() -> void {
%t1 = bir.call i32 printf(ptr @.str48)
bir.call void myprintf(ptr @.str49, ...)
```

Interpretation:

- semantic BIR still exposes explicit calls
- call ownership is visible before prepared lowering

Abnormal output usually looks like:

```text
bir.func @stdarg() -> void {
...
```

with no call lines where the source program clearly calls helpers or externs.

## Example: Prepared Call Plan Looks Healthy

Command:

```bash
./build/c4cll --dump-prepared-bir \
  --target x86_64-unknown-linux-gnu \
  --mir-focus-function stdarg \
  tests/c/external/c-testsuite/src/00204.c | \
  rg '^---|^prepared\.func @stdarg|^  call |^    arg |^    result |^    clobber |^  move_bundle '
```

Healthy output should eventually look like this shape:

```text
--- prepared-call-plans ---
prepared.func @stdarg
  call block_index=... inst_index=... indirect=no callee=...
    arg index=0 value_bank=gpr ...
    result value_bank=gpr ...
    clobber bank=gpr reg=...
```

Interpretation:

- every callsite is published before x86
- argument and result placement are inspectable without reading asm
- bank information makes GPR/FPR/VREG differences explicit

Right now some functions may still show only:

```text
--- prepared-call-plans ---
```

or:

```text
prepared.func @stdarg
```

with no `call` rows. That is not a healthy final state. It means the new dump
section exists, but the producer has not filled that authority yet.

## Example: Cross-Call Stability Check

Command:

```bash
./build/c4cll --dump-prepared-bir \
  --target x86_64-unknown-linux-gnu \
  --mir-focus-function stdarg \
  tests/c/external/c-testsuite/src/00204.c | \
  rg '^---|^prepared\.func @stdarg|^  home |^  storage |^  saved_register'
```

Healthy output usually shows enough information to answer:

- where important values live before and after the call
- whether preserved registers are part of the frame contract

Abnormal output is when call behavior can only be understood from final x86
text and prepared BIR never publishes the relevant home, storage, or save
facts.
