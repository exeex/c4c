#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
C4CLL="${C4CLL:-$BUILD_DIR/c4cll}"
CLANG="${CLANG:-clang}"
TEST_ROOT="$ROOT/tests/external/llvm-preprocessor-tests/clang/test/Preprocessor"

if [[ ! -x "$C4CLL" ]]; then
  echo "missing executable: $C4CLL" >&2
  exit 2
fi

if [[ ! -d "$TEST_ROOT" ]]; then
  echo "missing test root: $TEST_ROOT" >&2
  exit 2
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

normalize() {
  sed 's/[[:space:]]*$//' | awk 'NF > 0'
}

run_one() {
  local rel="$1"
  local lang="c"
  case "$rel" in
    *.cpp) lang="c++" ;;
  esac

  local src="$TEST_ROOT/$rel"
  local ours_out="$tmpdir/${rel//\//_}.ours.out"
  local ours_err="$tmpdir/${rel//\//_}.ours.err"
  local clang_out="$tmpdir/${rel//\//_}.clang.out"
  local clang_err="$tmpdir/${rel//\//_}.clang.err"
  local norm_ours="$tmpdir/${rel//\//_}.ours.norm"
  local norm_clang="$tmpdir/${rel//\//_}.clang.norm"

  local ours_rc=0
  local clang_rc=0

  "$C4CLL" --pp-only "$src" >"$ours_out" 2>"$ours_err" || ours_rc=$?
  "$CLANG" -E -P -nostdinc -x "$lang" "$src" >"$clang_out" 2>"$clang_err" || clang_rc=$?

  normalize <"$ours_out" >"$norm_ours"
  normalize <"$clang_out" >"$norm_clang"

  if [[ "$ours_rc" -ne "$clang_rc" ]]; then
    echo "FAIL $rel (exit code ours=$ours_rc clang=$clang_rc)"
    return 1
  fi

  if ! diff -u "$norm_clang" "$norm_ours" >"$tmpdir/${rel//\//_}.diff"; then
    echo "FAIL $rel (output diff)"
    sed -n '1,80p' "$tmpdir/${rel//\//_}.diff"
    return 1
  fi

  echo "PASS $rel"
  return 0
}

pass_count=0
fail_count=0

while IFS= read -r rel; do
  [[ -z "$rel" ]] && continue
  if run_one "$rel"; then
    pass_count=$((pass_count + 1))
  else
    fail_count=$((fail_count + 1))
  fi
done <<'EOF'
macro_paste_simple.c
macro_expand_empty.c
macro_rescan.c
macro_fn_varargs_iso.c
stringize_space.c
c99-6_10_3_3_p4.c
c99-6_10_3_4_p5.c
c99-6_10_3_4_p6.c
c99-6_10_3_4_p7.c
c99-6_10_3_4_p9.c
macro_arg_empty.c
macro_paste_empty.c
macro_fn_lparen_scan.c
macro_rparen_scan.c
macro_rparen_scan2.c
builtin_line.c
hash_line.c
EOF

echo "summary: pass=$pass_count fail=$fail_count"
if [[ "$fail_count" -ne 0 ]]; then
  exit 1
fi
