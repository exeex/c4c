# Debug BIR Stack

Use this note when the question is "did stack ownership come out right?".

Start with:

```bash
./build/c4cll --dump-bir --target <triple> <file>
./build/c4cll --dump-prepared-bir --target <triple> --mir-focus-function <function> <file>
```

## What To Read

`--dump-bir`:

- local slots in `bir.func`
- `bir.load_local` / `bir.store_local`
- dynamic stack calls such as `llvm.stacksave`, `llvm.dynamic_alloca.*`,
  `llvm.stackrestore`

`--dump-prepared-bir`:

- `--- prepared-stack-layout ---`
- `--- prepared-frame-plan ---`
- `--- prepared-dynamic-stack-plan ---`
- `--- prepared-addressing ---`

## Correctness Questions

Ask these in order:

1. Did fixed local storage become fixed stack objects and frame slots?
2. Did dynamic stack stay dynamic instead of being packed into fixed frame
   slots?
3. Are fixed-slot offsets and alignments stable and sensible?
4. If VLA or dynamic `alloca` exists, is that visible in the dynamic-stack
   plan rather than hidden inside x86-only logic?

## Good Signals

Fixed stack:

- `prepared-stack-layout` has `object` entries for fixed locals
- matching `slot` entries exist
- `frame_size` / `frame_alignment` match the fixed storage actually owned by
  the function

Dynamic stack:

- semantic BIR still shows `llvm.stacksave`, `llvm.dynamic_alloca.*`, and
  `llvm.stackrestore`
- `prepared-dynamic-stack-plan` shows `dynamic_stack_op` entries instead of
  silently turning dynamic allocation into a fixed frame slot
- `prepared-frame-plan` shows `has_dynamic_stack=yes` when the function has VLA
  or dynamic alloca behavior

Addressing:

- `prepared-addressing` uses fixed frame slots only for fixed storage
- offsets and alignments agree with the fixed slot being referenced

## Bad Signals

- a VLA result appears as an ordinary fixed `object` / `slot`
- `frame_size` grows as if dynamic storage were part of the fixed frame
- `prepared-dynamic-stack-plan` is empty even though semantic BIR clearly has
  `stacksave` / `dynamic_alloca` / `stackrestore`
- fixed-slot loads and stores only make sense if x86 guesses from moving `rsp`
  instead of following prepared frame ownership

## Practical Reading Pattern

Read in this order:

1. semantic `bir.func`
2. `prepared-stack-layout`
3. `prepared-frame-plan`
4. `prepared-dynamic-stack-plan`
5. `prepared-addressing`

If semantic BIR is already wrong, stop there. Do not debug x86 or MIR first.

## Example: Fixed Stack Looks Healthy

Command:

```bash
./build/c4cll --dump-prepared-bir \
  --target x86_64-unknown-linux-gnu \
  --mir-focus-function run \
  tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c | \
  rg '^---|^prepared\.func|^object #|^slot #|^  frame_slot_order|^  access '
```

Healthy output usually contains:

```text
--- prepared-stack-layout ---
object #... func=run name=...
slot #... object_id=... func=run offset=...
--- prepared-frame-plan ---
prepared.func @run frame_size=...
  frame_slot_order slot_id=#...
--- prepared-addressing ---
prepared.func @run frame_size=...
  access block=entry inst_index=...
```

Interpretation:

- fixed locals became stack objects
- stack objects got frame slots
- frame-plan exposes the fixed slot order
- addressing references those fixed slots explicitly

Suspicious output:

```text
--- prepared-stack-layout ---
frame_size=0 frame_alignment=0
--- prepared-frame-plan ---
prepared.func @run frame_size=0
```

That usually means fixed locals were not published as stack-owned state.

## Example: Dynamic Stack / VLA Looks Healthy

Command:

```bash
tmp_c="$(mktemp /tmp/c4c-vla-XXXXXX.c)"
cat >"$tmp_c" <<'EOF'
int f(long n) {
  int x = 1;
  int a[n];
  a[0] = x;
  return a[0];
}
EOF
./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu "$tmp_c" | \
  rg 'llvm\.stacksave|llvm\.dynamic_alloca|llvm\.stackrestore|bir\.func'
./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu --mir-focus-function f "$tmp_c" | \
  rg '^---|^prepared\.func|dynamic_stack_op|has_dynamic_stack|fixed_slots_use_fp|^object #|^slot #'
```

Healthy output usually contains:

```text
bir.call ptr llvm.stacksave()
bir.call ptr llvm.dynamic_alloca.i32(i64 %n)
bir.call void llvm.stackrestore(ptr %saved.sp)
--- prepared-frame-plan ---
prepared.func @f frame_size=... has_dynamic_stack=yes fixed_slots_use_fp=yes
--- prepared-dynamic-stack-plan ---
prepared.func @f requires_stack_save_restore=yes fixed_slots_use_fp=yes
  dynamic_stack_op ... kind=stack_save ...
  dynamic_stack_op ... kind=dynamic_alloca ...
  dynamic_stack_op ... kind=stack_restore ...
```

Interpretation:

- semantic BIR preserved the dynamic stack operations
- prepared BIR publishes dynamic-stack ownership explicitly
- fixed slots are anchored safely when dynamic stack exists

Abnormal output usually looks like one of these:

```text
--- prepared-dynamic-stack-plan ---
```

with no `dynamic_stack_op` lines even though semantic BIR showed dynamic stack
calls, or:

```text
object #... name=vla.buf ...
slot #... object_id=...
```

which suggests dynamic alloca was incorrectly turned into a fixed frame slot.
