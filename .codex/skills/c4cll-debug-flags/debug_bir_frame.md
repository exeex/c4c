# Debug BIR Frame

Use this note when the question is "did fixed frame ownership come out right?".

Start with:

```bash
./build/c4cll --dump-prepared-bir --target <triple> --mir-focus-function <function> <file>
```

## What To Read

Primary sections:

- `--- prepared-stack-layout ---`
- `--- prepared-frame-plan ---`
- `--- prepared-dynamic-stack-plan ---`

Secondary section:

- `--- prepared-storage-plans ---`

## Correctness Questions

Ask these in order:

1. What is the fixed frame size and alignment?
2. Which frame slots belong to the fixed frame?
3. Which saved registers belong to the frame contract?
4. If dynamic stack exists, what is the stable anchor for fixed slots?

## Good Signals

- `prepared-frame-plan` has one `prepared.func` entry per relevant function
- `frame_size` and `frame_alignment` match the fixed frame facts from
  `prepared-stack-layout`
- `frame_slot_order` is explicit
- callee-saved preservation appears as `saved_register`
- if dynamic stack exists, `fixed_slots_use_fp=yes` appears where fixed slots
  need a stable base

## Bad Signals

- frame size only exists in stack-layout but not in frame-plan
- saved-register decisions are invisible or only inferable from later MIR/x86
- a function with dynamic stack still looks like fixed slots are implicitly
  addressed off transient `rsp`
- frame decisions can only be understood by reading x86 asm instead of the
  prepared contract

## Practical Reading Pattern

1. Read `prepared-stack-layout` to see raw fixed objects and slots
2. Read `prepared-frame-plan` to see the published frame authority
3. If dynamic stack exists, read `prepared-dynamic-stack-plan` immediately
   after
4. Use `prepared-storage-plans` only to verify where values finally live inside
   that frame contract

Frame debugging should answer:

- what the fixed frame owns
- what it saves
- what anchor fixed slots use

If those answers are not visible in prepared BIR, the contract is not finished.

## Example: Fixed Frame Looks Healthy

Command:

```bash
./build/c4cll --dump-prepared-bir \
  --target x86_64-unknown-linux-gnu \
  --mir-focus-function stdarg \
  tests/c/external/c-testsuite/src/00204.c | \
  rg '^---|^prepared\.func @stdarg|^slot #|^  saved_register|^  frame_slot_order|has_dynamic_stack|fixed_slots_use_fp'
```

Healthy output usually contains:

```text
--- prepared-stack-layout ---
slot #... object_id=... func=stdarg offset=... size=... align=...
--- prepared-frame-plan ---
prepared.func @stdarg frame_size=... frame_alignment=...
  frame_slot_order slot_id=#...
```

Interpretation:

- raw fixed slots exist in stack-layout
- frame-plan republishes the same function as the fixed-frame authority
- the frame is inspectable without looking at x86 text

If callee-saved preservation is active, healthy output may also contain:

```text
  saved_register bank=gpr reg=...
```

## Example: Dynamic Frame Anchoring Looks Healthy

Command:

```bash
tmp_c="$(mktemp /tmp/c4c-frame-vla-XXXXXX.c)"
cat >"$tmp_c" <<'EOF'
int f(long n) {
  int x = 7;
  int a[n];
  a[0] = x;
  return x;
}
EOF
./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu --mir-focus-function f "$tmp_c" | \
  rg '^---|^prepared\.func @f|has_dynamic_stack|fixed_slots_use_fp|^  frame_slot_order|^  dynamic_stack_op'
```

Healthy output usually contains:

```text
--- prepared-frame-plan ---
prepared.func @f frame_size=... has_dynamic_stack=yes fixed_slots_use_fp=yes
--- prepared-dynamic-stack-plan ---
prepared.func @f requires_stack_save_restore=yes fixed_slots_use_fp=yes
  dynamic_stack_op ... kind=stack_save ...
  dynamic_stack_op ... kind=dynamic_alloca ...
  dynamic_stack_op ... kind=stack_restore ...
```

Interpretation:

- the fixed frame still exists
- the function admits dynamic stack explicitly
- fixed slots are expected to use a stable frame anchor

Abnormal output:

```text
prepared.func @f frame_size=... has_dynamic_stack=yes fixed_slots_use_fp=no
```

That is suspicious for VLA-shaped functions because fixed slots may no longer
have a stable base after stack motion.
