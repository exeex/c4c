#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
from typing import Dict, List, Optional

from ast_nodes import *
from errors import CompileError
from lexer import Lexer
from parser import Parser
from sema import FunctionSig, SemanticAnalyzer

# Map GCC/Clang math builtins to their standard library equivalents
_MATH_BUILTIN_MAP: Dict[str, str] = {
    "__builtin_fabs": "fabs", "__builtin_fabsf": "fabsf", "__builtin_fabsl": "fabsl",
    "__builtin_sqrt": "sqrt", "__builtin_sqrtf": "sqrtf",
    "__builtin_sin": "sin", "__builtin_cos": "cos",
    "__builtin_floor": "floor", "__builtin_ceil": "ceil",
    "__builtin_abs": "abs",
    "__builtin_inf": "isinf", "__builtin_inff": "isinff",  # __builtin_inf returns infinity constant
    "__builtin_isnan": "__isnan", "__builtin_isinf": "__isinf",
    "__builtin_isfinite": "__isfinite", "__builtin_isnormal": "__isnormal",
    "__builtin_signbit": "__signbit",
}


class IRBuilder:
    def __init__(self, prog: Program):
        self.prog = prog
        self.tmp_idx = 0
        self.label_idx = 0
        self.lines: List[str] = []
        self.preamble_lines: List[str] = []  # for global string constants
        self.slots: Dict[str, str] = {}
        self.local_arrays: Dict[str, int] = {}
        self.local_types: Dict[str, str] = {}
        self.loop_stack: List[tuple[str, str]] = []
        self.break_stack: List[str] = []
        self.func_sigs: Dict[str, FunctionSig] = {}
        self.struct_defs: Dict[str, StructDef] = {s.name: s for s in prog.struct_defs}
        self.global_vars: Dict[str, Optional[Node]] = {}
        self.global_arrays: Dict[str, int] = {}
        self.global_types: Dict[str, str] = {}
        self.global_is_extern: Dict[str, bool] = {}
        self.global_fp_target: Dict[str, str] = {}  # fptr var → target function name
        self.user_labels: Dict[str, str] = {}
        self.switch_case_stack: List[Dict[int, str]] = []
        self.switch_default_stack: List[Optional[str]] = []
        self.string_consts: Dict[str, str] = {}  # value -> global name
        self.string_idx = 0
        self.used_intrinsics: set = set()  # track LLVM intrinsics used
        self.auto_declared_fns: Dict[str, str] = {}  # undeclared called fns: name -> "declare ..." line
        self.enum_consts: Dict[str, int] = dict(prog.enum_consts) if prog.enum_consts else {}
        self._current_fn: str = ""
        self._current_fn_ret_type: str = "void"
        # Track element types for arrays
        self.local_array_elem_types: Dict[str, str] = {}
        self.global_array_elem_types: Dict[str, str] = {}
        # Track stride for multi-dimensional arrays (e.g. arr[2][4]: stride=4)
        self.local_array_strides: Dict[str, int] = {}  # name -> stride (# elements per row)
        for d in prog.decls:
            params = [p.typ for p in d.params if p.typ != "..."]
            is_var = any(p.typ == "..." for p in d.params)
            sig = FunctionSig(d.ret_type, params, is_var)
            self.func_sigs[d.name] = sig
        for g in prog.globals:
            self.global_vars[g.name] = g.init
            self.global_is_extern[g.name] = g.is_extern
            # Compute effective size (may come from ArrayInit or StructArrayInit)
            eff_size = g.size
            if eff_size is None and isinstance(g.init, ArrayInit):
                eff_size = len(g.init.values)
            if eff_size is None and isinstance(g.init, StructArrayInit):
                eff_size = g.init.size
            if eff_size is not None:
                self.global_arrays[g.name] = eff_size
                if g.base_type == "char" and g.ptr_level == 0:
                    self.global_types[g.name] = "char_array"
                    self.global_array_elem_types[g.name] = "char"
                else:
                    self.global_types[g.name] = "array"
                    self.global_array_elem_types[g.name] = g.base_type
            elif g.ptr_level == 0 and g.base_type == "char" and isinstance(g.init, StringLit):
                # char name[] = "..." - infer array size from string
                str_len = len(g.init.value) + 1  # +1 for null terminator
                self.global_arrays[g.name] = str_len
                self.global_types[g.name] = "char_array"
                self.global_array_elem_types[g.name] = "char"
            elif g.ptr_level > 0:
                self.global_types[g.name] = g.base_type + ("*" * g.ptr_level)
                # Track function pointer initializer target
                if isinstance(g.init, UnaryOp) and g.init.op == "&" and isinstance(g.init.expr, Var):
                    self.global_fp_target[g.name] = g.init.expr.name
                elif isinstance(g.init, Var):
                    self.global_fp_target[g.name] = g.init.name
            elif isinstance(g.init, StructInit):
                # struct with brace initializer but no size
                self.global_types[g.name] = g.base_type
            else:
                self.global_types[g.name] = g.base_type
        for fn in prog.funcs:
            params = [p.typ for p in fn.params if p.typ != "..."]
            is_var = any(p.typ == "..." for p in fn.params)
            self.func_sigs[fn.name] = FunctionSig(fn.ret_type, params, is_var)

    def llvm_ty(self, ty: str) -> str:
        if ty == "void":
            return "void"
        # Array field type: arr_field:N:elemtype
        if ty.startswith("arr_field:"):
            parts = ty.split(":", 2)
            n = int(parts[1])
            elem = self.llvm_ty(parts[2])
            return f"[{n} x {elem}]"
        if ty.startswith("struct:"):
            tag = ty.split(":", 1)[1]
            return f"%struct.{tag}"
        if ty.endswith("*") or ty in ("ptr", "char*"):
            return "ptr"
        if ty == "char":
            return "i8"
        if ty in ("short", "unsigned short"):
            return "i32"  # promote short to i32 for simplicity
        if ty in ("long", "unsigned long"):
            return "i32"  # use i32 for simplicity (matches most operations)
        if ty == "float":
            return "float"
        if ty == "double":
            return "double"
        return "i32"

    def struct_field(self, struct_ty: str, field: str) -> tuple[int, str]:
        if ":" not in struct_ty:
            raise CompileError(f"codegen error: {struct_ty!r} is not a struct type")
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            raise CompileError(f"codegen error: unknown struct type {struct_ty!r}")
        # Check union aliases first (for anonymous unions inlined into parent)
        if s.union_aliases and field in s.union_aliases:
            idx = s.union_aliases[field]
            # Find the field type - search all original fields by name
            for uf in s.fields:
                if uf.name == field:
                    return idx, uf.typ
            # Fallback: return the representative field's type
            if idx < len(s.fields):
                return idx, s.fields[idx].typ
            return idx, "int"
        for i, f in enumerate(s.fields):
            if f.name == field:
                # For unions, all fields are at index 0
                # For array fields, encode array size in type string
                if f.array_size is not None:
                    arr_ty = f"arr_field:{f.array_size}:{f.typ}"
                    return 0 if s.is_union else i, arr_ty
                return 0 if s.is_union else i, f.typ
        raise CompileError(f"codegen error: unknown field {field!r} in {struct_ty!r}")

    def is_struct_type(self, ty: str) -> bool:
        return ty.startswith("struct:")

    def ptr_elem_llvm_ty(self, ptr_ty: str) -> str:
        """Return LLVM element type for pointer arithmetic GEP stride."""
        if ptr_ty == "char*":
            return "i8"
        if ptr_ty == "double*":
            return "double"
        if ptr_ty.endswith("*"):
            elem = ptr_ty[:-1]
            return self.llvm_ty(elem)
        return "i32"  # default

    def array_elem_llvm_ty(self, name: str) -> str:
        """Return LLVM element type for array GEP stride."""
        elem_ty = self.local_array_elem_types.get(name) or self.global_array_elem_types.get(name, "int")
        return self.llvm_ty(elem_ty)

    def tmp(self) -> str:
        t = f"%t{self.tmp_idx}"
        self.tmp_idx += 1
        return t

    def emit(self, s: str) -> None:
        self.lines.append(s)

    def new_label(self, prefix: str) -> str:
        lbl = f"{prefix}{self.label_idx}"
        self.label_idx += 1
        return lbl

    def emit_label(self, label: str) -> None:
        self.emit(f"{label}:")
        self.flow_terminated = False  # a new basic block is reachable (via label)

    def promote_to_i32(self, v: str, ty: str) -> str:
        if ty == "char":
            t = self.tmp()
            self.emit(f"  {t} = sext i8 {v} to i32")
            return t
        if ty == "double":
            t = self.tmp()
            self.emit(f"  {t} = fptosi double {v} to i32")
            return t
        if ty == "float":
            t = self.tmp()
            self.emit(f"  {t} = fptosi float {v} to i32")
            return t
        return v

    def emit_convert(self, v: str, src_ty: str, dst_ty: str) -> str:
        """Emit type conversion from src_ty to dst_ty. Returns the new value."""
        if src_ty == dst_ty:
            return v
        src_ll = self.llvm_ty(src_ty)
        dst_ll = self.llvm_ty(dst_ty)
        if src_ll == dst_ll:
            return v
        if dst_ll == "void":
            return v
        # pointer conversions
        if dst_ll == "ptr":
            # integer 0 → null pointer
            if v in ("0", "null"):
                return "null"
            return v
        if src_ll == "ptr":
            return v
        t = self.tmp()
        if src_ll == "i8" and dst_ll == "i32":
            self.emit(f"  {t} = sext i8 {v} to i32")
            return t
        if src_ll == "i32" and dst_ll == "i8":
            self.emit(f"  {t} = trunc i32 {v} to i8")
            return t
        if dst_ll == "double" and src_ll == "i32":
            self.emit(f"  {t} = sitofp i32 {v} to double")
            return t
        if dst_ll == "double" and src_ll == "i8":
            t2 = self.tmp()
            self.emit(f"  {t2} = sext i8 {v} to i32")
            self.emit(f"  {t} = sitofp i32 {t2} to double")
            return t
        if src_ll == "double" and dst_ll == "i32":
            self.emit(f"  {t} = fptosi double {v} to i32")
            return t
        if src_ll == "double" and dst_ll == "i8":
            t2 = self.tmp()
            self.emit(f"  {t2} = fptosi double {v} to i32")
            self.emit(f"  {t} = trunc i32 {t2} to i8")
            return t
        # float (f32) conversions
        if dst_ll == "float" and src_ll == "i32":
            self.emit(f"  {t} = sitofp i32 {v} to float")
            return t
        if dst_ll == "float" and src_ll == "i8":
            t2 = self.tmp()
            self.emit(f"  {t2} = sext i8 {v} to i32")
            self.emit(f"  {t} = sitofp i32 {t2} to float")
            return t
        if dst_ll == "float" and src_ll == "double":
            self.emit(f"  {t} = fptrunc double {v} to float")
            return t
        if dst_ll == "double" and src_ll == "float":
            self.emit(f"  {t} = fpext float {v} to double")
            return t
        if src_ll == "float" and dst_ll == "i32":
            self.emit(f"  {t} = fptosi float {v} to i32")
            return t
        if src_ll == "float" and dst_ll == "i8":
            t2 = self.tmp()
            self.emit(f"  {t2} = fptosi float {v} to i32")
            self.emit(f"  {t} = trunc i32 {t2} to i8")
            return t
        return v

    def resolve_var_ptr(self, name: str) -> str:
        if name in self.slots:
            return self.slots[name]
        if name in self.global_vars:
            return f"@{name}"
        raise CompileError(f"codegen error: unknown variable {name!r}")

    def resolve_var_type(self, name: str) -> str:
        if name in self.local_types:
            return self.local_types[name]
        if name in self.global_types:
            return self.global_types[name]
        raise CompileError(f"codegen error: unknown variable {name!r}")

    def expr_type(self, n: Node) -> str:
        match n:
            case IntLit(suffix=suffix):
                if suffix in ("LL", "ULL"):
                    return "long long"
                if suffix in ("L", "UL"):
                    return "long"
                return "int"
            case FloatLit():
                return "double"
            case StringLit():
                return "char*"
            case Var(name=name):
                if name in self.enum_consts:
                    return "int"
                # Function name used as function pointer value (not shadowed by local/global var)
                if name in self.func_sigs and name not in self.local_types and name not in self.global_types:
                    return "ptr"
                vty = self.resolve_var_type(name)
                if vty in ("array", "char_array"):
                    # Return pointer to element type for array-to-pointer decay
                    elem = self.local_array_elem_types.get(name) or self.global_array_elem_types.get(name, "int")
                    return elem + "*"
                return vty
            case Index(base=base, index=index):
                # Determine element type from base
                bt = self.expr_type(base)
                if bt == "array":
                    # Should not happen since Var now returns elem*
                    return "int"
                if bt.endswith("*"):
                    return bt[:-1]
                if bt == "ptr":
                    return "ptr"  # indexing into a generic/function-pointer array
                if bt == "char":
                    return "char"  # indexing into a char row (e.g. arr[i][j] for char arr[][])
                # Fallback: check if base is Var with known elem type
                if isinstance(base, Var):
                    elem = self.local_array_elem_types.get(base.name) or self.global_array_elem_types.get(base.name)
                    if elem:
                        return elem
                return "int"
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.expr_type(base)
                if through_ptr and bt.endswith("*"):
                    bt = bt[:-1]
                elif through_ptr and bt in ("ptr", "int"):
                    # Unknown pointer type (e.g. from IndirectCall) - scan structs for field
                    for sname, sdef in self.struct_defs.items():
                        for f in sdef.fields:
                            if f.name == field:
                                return f.typ
                    return "int"
                try:
                    _, fty = self.struct_field(bt, field)
                except CompileError:
                    return "int"
                # Array fields: return element pointer type (array-to-pointer decay)
                if fty.startswith("arr_field:"):
                    parts = fty.split(":", 2)
                    return parts[2] + "*"
                return fty
            case UnaryOp(op="&"):
                et = self.expr_type(n.expr)
                if et == "array":
                    return "int*"
                return et + "*"
            case UnaryOp(op="*"):
                et = self.expr_type(n.expr)
                if et.endswith("*"):
                    return et[:-1]
                return "int"
            case UnaryOp(op=op):
                if op in ("+", "-"):
                    inner_ty = self.expr_type(n.expr)
                    if inner_ty in ("double", "float"):
                        return inner_ty
                return "int"
            case Cast(typ=typ):
                return typ
            case TernaryOp(then_expr=then_expr, else_expr=else_expr):
                tyt = self.expr_type(then_expr)
                tye = self.expr_type(else_expr)
                if tyt == tye:
                    return tyt
                if tyt.endswith("*") and isinstance(else_expr, IntLit) and else_expr.value == 0:
                    return tyt
                if tye.endswith("*") and isinstance(then_expr, IntLit) and then_expr.value == 0:
                    return tye
                return "int"
            case SizeofExpr():
                return "int"
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                lt = self.expr_type(lhs)
                rt = self.expr_type(rhs)
                # Pointer arithmetic: ptr+int or int+ptr yields ptr; ptr-ptr yields int
                if lt.endswith("*") and not rt.endswith("*"):
                    return lt  # ptr + int or ptr - int → ptr
                if rt.endswith("*") and not lt.endswith("*") and op == "+":
                    return rt  # int + ptr → ptr
                # Float/double arithmetic: double wins over float
                if (lt == "double" or rt == "double") and op in {"+", "-", "*", "/"}:
                    return "double"
                if (lt == "float" or rt == "float") and op in {"+", "-", "*", "/"}:
                    return "float"
                # Shift operators: result type = promoted left operand only (C99 §6.5.7)
                if op in {"<<", ">>"}:
                    if lt in ("char", "short", "unsigned short"):
                        return "int"
                    return lt
                # Integer arithmetic: long long > long > int (usual arithmetic conversions)
                if lt == "long long" or rt == "long long":
                    return "long long"
                if lt == "long" or rt == "long":
                    return "long"
                return "int"
            case AssignExpr(target=target, op=op):
                if op == "=":
                    if isinstance(target, Var):
                        return self.resolve_var_type(target.name)
                    if isinstance(target, (Index, Member)) or (
                        isinstance(target, UnaryOp) and target.op == "*"
                    ):
                        return self.expr_type(target)
                return "int"
            case IncDec():
                return "int"
            case Call(name=name):
                # Apply math builtin remapping to find the right signature
                actual_name = _MATH_BUILTIN_MAP.get(name, name)
                sig = self.func_sigs.get(actual_name) or self.func_sigs.get(name)
                if sig is None:
                    return "int"
                ret = sig.ret_type
                if ret == "void":
                    return "void"
                return ret
            case IndirectCall(func=func):
                # Try to determine the return type from the function pointer
                f = func
                if isinstance(f, UnaryOp) and f.op == "*":
                    f = f.expr
                if isinstance(f, Var):
                    # Direct call to a known function (e.g. via function pointer that is itself a func)
                    sig = self.func_sigs.get(f.name)
                    if sig is not None:
                        return sig.ret_type if sig.ret_type != "void" else "void"
                    # Variable initialized from a known function
                    ret = self.local_fptr_rets.get(f.name)
                    if ret is not None:
                        return ret
                return "int"  # assume int return for indirect calls
            case Generic(ctrl=ctrl, assocs=assocs):
                selected = self._generic_select(ctrl, assocs)
                if selected is not None:
                    return self.expr_type(selected)
                return "int"
            case _:
                return "int"

    def sizeof_type(self, ty: str) -> int:
        if ty == "char":
            return 1
        if ty in ("short", "unsigned short"):
            return 2
        if ty in ("int", "float", "unsigned", "unsigned int"):
            return 4
        if ty in ("long", "unsigned long"):
            return 8  # LP64: long is 8 bytes on 64-bit platforms
        if ty == "double":
            return 8
        if ty.endswith("*") or ty in ("ptr", "char*"):
            return 8
        if ty.startswith("arr_field:"):
            parts = ty.split(":", 2)
            return int(parts[1]) * self.sizeof_type(parts[2])
        if self.is_struct_type(ty):
            tag = ty.split(":", 1)[1]
            s = self.struct_defs.get(tag)
            if s is None:
                return 8  # unknown struct default
            total = 0
            for f in s.fields:
                if f.array_size is not None:
                    total += f.array_size * self.sizeof_type(f.typ)
                else:
                    total += self.sizeof_type(f.typ)
            return total
        return 4  # default

    def sizeof_expr(self, n: Node) -> int:
        if isinstance(n, Var):
            name = n.name
            if name in self.local_arrays:
                elem = self.local_array_elem_types.get(name, "int")
                return self.local_arrays[name] * self.sizeof_type(elem)
            if name in self.global_arrays:
                elem = self.global_array_elem_types.get(name, "int")
                return self.global_arrays[name] * self.sizeof_type(elem)
        # For member access on array fields, sizeof returns the full array size (no decay)
        if isinstance(n, Member):
            try:
                mb_ty = self.expr_type(n.base)
                if n.through_ptr and mb_ty.endswith("*"):
                    mb_ty = mb_ty[:-1]
                _, fty = self.struct_field(mb_ty, n.field)
                if fty.startswith("arr_field:"):
                    return self.sizeof_type(fty)
            except Exception:
                pass
        return self.sizeof_type(self.expr_type(n))

    def _generic_normalize_type(self, ty: str) -> str:
        """Normalize type for _Generic matching: strip top-level const/volatile, normalize names."""
        # Remove trailing spaces, normalize
        ty = ty.strip()
        # Map unsigned → unsigned int, signed → int
        if ty == "unsigned":
            ty = "unsigned int"
        # Strip const/volatile from non-pointer types (const int → int)
        # For pointer types, strip TRAILING const (int * const → int *)
        # This matches C11 semantics where controlling expression type is unqualified
        return ty

    def _generic_types_match(self, ctrl_ty: str, assoc_ty: str) -> bool:
        """Check if _Generic association type matches the controlling expression type."""
        ctrl = self._generic_normalize_type(ctrl_ty)
        assoc = self._generic_normalize_type(assoc_ty)
        if ctrl == assoc:
            return True
        # "int" matches "int", "const int", "signed int"
        int_types = {"int", "signed int", "signed"}
        if ctrl in int_types and assoc in int_types:
            return True
        # Pointer type matching: ptr* and its const variants
        if ctrl.endswith("*") and assoc.endswith("*"):
            # Strip trailing " const" from both for comparison
            c = ctrl.rstrip().rstrip("const").rstrip().rstrip("volatile").rstrip()
            a = assoc.rstrip().rstrip("const").rstrip().rstrip("volatile").rstrip()
            return c == a
        return False

    def _generic_select(self, ctrl: Node, assocs) -> Optional["Node"]:
        """Select the matching arm from _Generic associations."""
        try:
            # Use full const-qualified type for Var if available (for _Generic matching)
            if isinstance(ctrl, Var) and ctrl.name in self.local_full_types:
                ctrl_ty = self.local_full_types[ctrl.name]
            else:
                ctrl_ty = self.expr_type(ctrl)
        except Exception:
            ctrl_ty = "int"
        default_expr = None
        for assoc_ty, expr in assocs:
            if assoc_ty is None:
                default_expr = expr
            elif self._generic_types_match(ctrl_ty, assoc_ty):
                return expr
        return default_expr

    def codegen_lvalue_ptr(self, n: Node) -> str:
        match n:
            case Var(name=name):
                # For arrays, &arr is the same as arr (pointer to first element)
                return self.resolve_var_ptr(name)
            case UnaryOp(op="*", expr=expr):
                pv = self.codegen_expr(expr)
                if pv is None:
                    raise CompileError("codegen error: invalid pointer dereference")
                return pv
            case Index(base=base, index=index):
                idx = self.codegen_expr(index)
                if idx is None:
                    raise CompileError("codegen error: invalid array index")
                # Determine element LLVM type for GEP stride
                base_type_str = self.expr_type(base)
                if base_type_str.endswith("*"):
                    elem_ll = self.ptr_elem_llvm_ty(base_type_str)
                else:
                    elem_ll = "i32"  # fallback
                match base:
                    case Var(name=name):
                        t = self.tmp()
                        if name in self.local_arrays:
                            n_elem = self.local_arrays[name]
                            arr_elem_ll = self.array_elem_llvm_ty(name)
                            arr_ptr = self.slots.get(name, f"%{name}")
                            stride = self.local_array_strides.get(name)
                            if stride is not None and stride > 1:
                                # Multi-dim array: multiply index by stride
                                scaled_idx = self.tmp()
                                self.emit(f"  {scaled_idx} = mul i32 {idx}, {stride}")
                                self.emit(
                                    f"  {t} = getelementptr inbounds [{n_elem} x {arr_elem_ll}], ptr {arr_ptr}, i32 0, i32 {scaled_idx}"
                                )
                            else:
                                self.emit(
                                    f"  {t} = getelementptr inbounds [{n_elem} x {arr_elem_ll}], ptr {arr_ptr}, i32 0, i32 {idx}"
                                )
                            return t
                        if name in self.global_arrays:
                            n_elem = self.global_arrays[name]
                            arr_elem_ll = self.array_elem_llvm_ty(name)
                            self.emit(
                                f"  {t} = getelementptr inbounds [{n_elem} x {arr_elem_ll}], ptr @{name}, i32 0, i32 {idx}"
                            )
                            return t
                        vty = self.resolve_var_type(name)
                        if vty.endswith("*") or vty == "ptr":
                            base_ptr = self.codegen_expr(base)
                            if base_ptr is None:
                                raise CompileError("codegen error: invalid pointer base")
                            stride_ty = self.ptr_elem_llvm_ty(vty) if vty.endswith("*") else "i8"
                            ptr_stride = self.local_array_strides.get(name)
                            if ptr_stride is not None and ptr_stride > 1:
                                scaled_idx = self.tmp()
                                self.emit(f"  {scaled_idx} = mul i32 {idx}, {ptr_stride}")
                                self.emit(f"  {t} = getelementptr inbounds {stride_ty}, ptr {base_ptr}, i32 {scaled_idx}")
                            else:
                                self.emit(f"  {t} = getelementptr inbounds {stride_ty}, ptr {base_ptr}, i32 {idx}")
                            return t
                        raise CompileError("codegen error: index base must be array variable")
                    case _:
                        # Non-variable base (e.g., member access result, array[i][j])
                        t = self.tmp()
                        # If base is an Index itself, use its lvalue pointer to avoid
                        # loading intermediate elements (handles arr[i][j] correctly)
                        if isinstance(base, Index):
                            base_ptr = self.codegen_lvalue_ptr(base)
                            # Use element type of the innermost array if available
                            inner = base
                            while isinstance(inner, Index):
                                inner = inner.base
                            if isinstance(inner, Var):
                                inner_elem_ll = self.array_elem_llvm_ty(inner.name)
                            else:
                                inner_elem_ll = elem_ll
                            self.emit(f"  {t} = getelementptr inbounds {inner_elem_ll}, ptr {base_ptr}, i32 {idx}")
                        else:
                            base_ptr = self.codegen_expr(base)
                            if base_ptr is None:
                                raise CompileError("codegen error: invalid array base expression")
                            self.emit(f"  {t} = getelementptr inbounds {elem_ll}, ptr {base_ptr}, i32 {idx}")
                        return t
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.expr_type(base)
                if through_ptr:
                    if bt.endswith("*"):
                        struct_ty = bt[:-1]
                        struct_ptr = self.codegen_expr(base)
                    elif bt in ("ptr", "int"):
                        # Unknown pointer type (e.g. from IndirectCall returning ptr) -
                        # emit call with ptr return if base is IndirectCall, then scan for struct
                        if isinstance(base, IndirectCall):
                            inner_func = base.func
                            if isinstance(inner_func, UnaryOp) and inner_func.op == "*":
                                inner_func = inner_func.expr
                            inner_fptr = self.codegen_expr(inner_func)
                            inner_args_text = []
                            for iarg in base.args:
                                iv = self.codegen_expr(iarg)
                                iat = self.expr_type(iarg)
                                inner_args_text.append(f"{self.llvm_ty(iat)} {iv}")
                            param_tys_inner = ", ".join(a.rsplit(" ", 1)[0] for a in inner_args_text)
                            inner_args_str = ", ".join(inner_args_text)
                            sp_t = self.tmp()
                            self.emit(f"  {sp_t} = call ptr ({param_tys_inner}) {inner_fptr}({inner_args_str})")
                            struct_ptr = sp_t
                        else:
                            struct_ptr = self.codegen_expr(base)
                        # Find struct type by scanning for field name
                        struct_ty = None
                        for sname_scan, sdef_scan in self.struct_defs.items():
                            for sf in sdef_scan.fields:
                                if sf.name == field:
                                    struct_ty = f"struct:{sname_scan}"
                                    break
                            if struct_ty:
                                break
                        if struct_ty is None:
                            raise CompileError("codegen error: '->' requires pointer base")
                    else:
                        raise CompileError("codegen error: '->' requires pointer base")
                else:
                    struct_ty = bt
                    struct_ptr = self.codegen_lvalue_ptr(base)
                if struct_ptr is None:
                    raise CompileError("codegen error: invalid struct base")
                field_idx, _ = self.struct_field(struct_ty, field)
                t = self.tmp()
                self.emit(
                    f"  {t} = getelementptr inbounds {self.llvm_ty(struct_ty)}, ptr {struct_ptr}, i32 0, i32 {field_idx}"
                )
                return t
            case _:
                raise CompileError("codegen error: invalid assignment target")

    def eval_global_const(self, n: Node) -> int:
        match n:
            case IntLit(value=value):
                return value
            case FloatLit(value=value):
                return value
            case UnaryOp(op=op, expr=expr):
                v = self.eval_global_const(expr)
                if op == "+":
                    return v
                if op == "-":
                    return -v
                if op == "!":
                    return 0 if v else 1
                raise CompileError(f"codegen error: unsupported global initializer op {op!r}")
            case Cast(expr=expr):
                return self.eval_global_const(expr)
            case SizeofExpr(typ=typ, expr=expr):
                if typ is not None:
                    return self.sizeof_type(typ)
                if expr is None:
                    raise CompileError("codegen error: invalid sizeof expression")
                return self.sizeof_expr(expr)
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                c = self.eval_global_const(cond)
                if c != 0:
                    return self.eval_global_const(then_expr)
                return self.eval_global_const(else_expr)
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.eval_global_const(lhs)
                r = self.eval_global_const(rhs)
                if op == "+":
                    return l + r
                if op == "-":
                    return l - r
                if op == "*":
                    return l * r
                if op == "/":
                    return int(l / r)
                if op == "%":
                    return l % r
                if op == "&":
                    return l & r
                if op == "|":
                    return l | r
                if op == "^":
                    return l ^ r
                if op == "&&":
                    return 1 if (l != 0 and r != 0) else 0
                if op == "||":
                    return 1 if (l != 0 or r != 0) else 0
                if op == "==":
                    return 1 if l == r else 0
                if op == "!=":
                    return 1 if l != r else 0
                if op == "<":
                    return 1 if l < r else 0
                if op == "<=":
                    return 1 if l <= r else 0
                if op == ">":
                    return 1 if l > r else 0
                if op == ">=":
                    return 1 if l >= r else 0
                raise CompileError(f"codegen error: unsupported global initializer op {op!r}")
            case Var(name=name):
                # Enum constant reference
                if name in self.enum_consts:
                    return self.enum_consts[name]
                raise CompileError(
                    f"codegen error: '{name}' is not a constant expression"
                )
            case _:
                raise CompileError(
                    "codegen error: global initializer must be an integer constant expression"
                )

    def get_string_global(self, value: str) -> str:
        """Get or create a global constant for a string literal."""
        if value in self.string_consts:
            return self.string_consts[value]
        gname = f"@.str.{self.string_idx}"
        self.string_idx += 1
        self.string_consts[value] = gname
        # Encode the string
        encoded = []
        for ch in value:
            c = ord(ch)
            if 32 <= c <= 126 and ch not in ('"', '\\'):
                encoded.append(ch)
            else:
                encoded.append(f"\\{c:02X}")
        encoded.append("\\00")  # null terminator
        length = len(value) + 1
        self.preamble_lines.append(
            f'{gname} = private unnamed_addr constant [{length} x i8] c"{"".join(encoded)}"'
        )
        return gname

    def codegen_expr(self, n: Node) -> Optional[str]:
        match n:
            case IntLit(value=value):
                return str(value)
            case FloatLit(value=value):
                # LLVM requires IEEE 754 hex for exact double constants, but decimal works too
                return repr(value)
            case StringLit(value=value):
                gname = self.get_string_global(value)
                length = len(value) + 1
                t = self.tmp()
                self.emit(f"  {t} = getelementptr inbounds [{length} x i8], ptr {gname}, i32 0, i32 0")
                return t
            case Var(name=name):
                # Check enum constants
                if name in self.enum_consts:
                    return str(self.enum_consts[name])
                # Check if it's a function name used as function pointer
                # (only if not shadowed by a local or global variable)
                if name in self.func_sigs and name not in self.local_types and name not in self.global_types:
                    return f"@{name}"
                vty = self.resolve_var_type(name)
                if vty in ("array", "char_array"):
                    # Return pointer to first element (array-to-pointer decay)
                    slot = self.resolve_var_ptr(name)
                    sz = self.local_arrays.get(name) or self.global_arrays.get(name, 1)
                    elem_ll = self.array_elem_llvm_ty(name)
                    t = self.tmp()
                    self.emit(f"  {t} = getelementptr inbounds [{sz} x {elem_ll}], ptr {slot}, i32 0, i32 0")
                    return t
                slot = self.resolve_var_ptr(name)
                t = self.tmp()
                self.emit(f"  {t} = load {self.llvm_ty(vty)}, ptr {slot}")
                return t
            case Index():
                ptr = self.codegen_lvalue_ptr(n)
                t = self.tmp()
                elem_ty = self.expr_type(n)
                elem_ll = self.llvm_ty(elem_ty)
                self.emit(f"  {t} = load {elem_ll}, ptr {ptr}")
                return t
            case Member(base=mb, field=mf, through_ptr=mtp):
                # Get struct type to check if field is an array field
                mb_ty = self.expr_type(mb)
                if mtp and mb_ty.endswith("*"):
                    mb_ty = mb_ty[:-1]
                try:
                    _, raw_fty = self.struct_field(mb_ty, mf)
                except Exception:
                    raw_fty = "int"
                if raw_fty.startswith("arr_field:"):
                    # Array field: return pointer to first element (array-to-pointer decay)
                    ptr = self.codegen_lvalue_ptr(n)
                    parts = raw_fty.split(":", 2)
                    n_elem = int(parts[1])
                    elem_ll = self.llvm_ty(parts[2])
                    t = self.tmp()
                    self.emit(f"  {t} = getelementptr inbounds [{n_elem} x {elem_ll}], ptr {ptr}, i32 0, i32 0")
                    return t
                ptr = self.codegen_lvalue_ptr(n)
                t = self.tmp()
                mty = self.expr_type(n)
                self.emit(f"  {t} = load {self.llvm_ty(mty)}, ptr {ptr}")
                return t
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.codegen_expr(lhs)
                if l is None:
                    raise CompileError("codegen error: void value used in binary operation")
                t = self.tmp()
                if op in {"&&", "||"}:
                    lty = self.expr_type(lhs)
                    lb = self.tmp()
                    if lty.endswith("*"):
                        self.emit(f"  {lb} = icmp ne ptr {l}, null")
                    else:
                        l = self.promote_to_i32(l, lty)
                        self.emit(f"  {lb} = icmp ne i32 {l}, 0")
                    rhs_lbl = self.new_label("logic_rhs")
                    short_lbl = self.new_label("logic_short")
                    end_lbl = self.new_label("logic_end")
                    if op == "&&":
                        self.emit(f"  br i1 {lb}, label %{rhs_lbl}, label %{short_lbl}")
                    else:
                        self.emit(f"  br i1 {lb}, label %{short_lbl}, label %{rhs_lbl}")
                    self.emit_label(rhs_lbl)
                    rbv = self.codegen_expr(rhs)
                    if rbv is None:
                        raise CompileError("codegen error: void value used in logical operation")
                    rty = self.expr_type(rhs)
                    rb = self.tmp()
                    if rty.endswith("*"):
                        self.emit(f"  {rb} = icmp ne ptr {rbv}, null")
                    else:
                        rbv = self.promote_to_i32(rbv, rty)
                        self.emit(f"  {rb} = icmp ne i32 {rbv}, 0")
                    self.emit(f"  br label %{end_lbl}")
                    self.emit_label(short_lbl)
                    short_v = "0" if op == "&&" else "1"
                    self.emit(f"  br label %{end_lbl}")
                    self.emit_label(end_lbl)
                    b = self.tmp()
                    self.emit(f"  {b} = phi i1 [{short_v}, %{short_lbl}], [{rb}, %{rhs_lbl}]")
                    self.emit(f"  {t} = zext i1 {b} to i32")
                    return t

                r = self.codegen_expr(rhs)
                if r is None:
                    raise CompileError("codegen error: void value used in binary operation")
                lty = self.expr_type(lhs)
                rty = self.expr_type(rhs)
                # Check if float/double arithmetic is needed
                is_double = lty == "double" or rty == "double"
                is_float = not is_double and (lty == "float" or rty == "float")
                if (is_double or is_float) and op in {"+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">="}:
                    fp_ty = "double" if is_double else "float"
                    ld = self.emit_convert(l, lty, fp_ty)
                    rd = self.emit_convert(r, rty, fp_ty)
                    if op in {"+", "-", "*", "/"}:
                        dop = {"+": "fadd", "-": "fsub", "*": "fmul", "/": "fdiv"}[op]
                        self.emit(f"  {t} = {dop} {fp_ty} {ld}, {rd}")
                        return t
                    fcmp_map = {"==": "oeq", "!=": "one", "<": "olt", "<=": "ole", ">": "ogt", ">=": "oge"}
                    b = self.tmp()
                    self.emit(f"  {b} = fcmp {fcmp_map[op]} {fp_ty} {ld}, {rd}")
                    self.emit(f"  {t} = zext i1 {b} to i32")
                    return t
                l = self.promote_to_i32(l, lty)
                r = self.promote_to_i32(r, rty)
                if op in {"+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>"}:
                    if op in {"+", "-"} and (lty.endswith("*") or rty.endswith("*")):
                        if op == "+" and lty.endswith("*") and rty in ("int", "char"):
                            stride = self.ptr_elem_llvm_ty(lty)
                            self.emit(f"  {t} = getelementptr inbounds {stride}, ptr {l}, i32 {r}")
                            return t
                        if op == "+" and lty in ("int", "char") and rty.endswith("*"):
                            stride = self.ptr_elem_llvm_ty(rty)
                            self.emit(f"  {t} = getelementptr inbounds {stride}, ptr {r}, i32 {l}")
                            return t
                        if op == "-" and lty.endswith("*") and rty in ("int", "char"):
                            neg = self.tmp()
                            self.emit(f"  {neg} = sub i32 0, {r}")
                            stride = self.ptr_elem_llvm_ty(lty)
                            self.emit(f"  {t} = getelementptr inbounds {stride}, ptr {l}, i32 {neg}")
                            return t
                        if op == "-" and lty.endswith("*") and rty.endswith("*"):
                            li = self.tmp()
                            ri = self.tmp()
                            delta = self.tmp()
                            self.emit(f"  {li} = ptrtoint ptr {l} to i64")
                            self.emit(f"  {ri} = ptrtoint ptr {r} to i64")
                            self.emit(f"  {delta} = sub i64 {li}, {ri}")
                            self.emit(f"  {t} = trunc i64 {delta} to i32")
                            s = self.tmp()
                            elem_size = 1 if lty == "char*" else 4
                            self.emit(f"  {s} = sdiv i32 {t}, {elem_size}")
                            return s
                    op_map = {
                        "+": "add",
                        "-": "sub",
                        "*": "mul",
                        "/": "sdiv",
                        "%": "srem",
                        "&": "and",
                        "|": "or",
                        "^": "xor",
                        "<<": "shl",
                        ">>": "ashr",
                    }
                    self.emit(f"  {t} = {op_map[op]} i32 {l}, {r}")
                    return t
                if op in {"==", "!=", "<", "<=", ">", ">="}:
                    if lty.endswith("*") or rty.endswith("*"):
                        if op not in {"==", "!="}:
                            raise CompileError("codegen error: pointer relational op is not supported")
                        b = self.tmp()
                        pred = "eq" if op == "==" else "ne"
                        if rty == "int":
                            self.emit(f"  {b} = icmp {pred} ptr {l}, null")
                        elif lty == "int":
                            self.emit(f"  {b} = icmp {pred} ptr {r}, null")
                        else:
                            self.emit(f"  {b} = icmp {pred} ptr {l}, {r}")
                        self.emit(f"  {t} = zext i1 {b} to i32")
                        return t
                    icmp_map = {
                        "==": "eq",
                        "!=": "ne",
                        "<": "slt",
                        "<=": "sle",
                        ">": "sgt",
                        ">=": "sge",
                    }
                    b = self.tmp()
                    self.emit(f"  {b} = icmp {icmp_map[op]} i32 {l}, {r}")
                    self.emit(f"  {t} = zext i1 {b} to i32")
                    return t
                raise CompileError(f"codegen error: unsupported binary operator {op!r}")
            case UnaryOp(op=op, expr=expr):
                if op == "&":
                    # &func_name == pointer to function
                    if isinstance(expr, Var) and expr.name in self.func_sigs and expr.name not in self.local_types and expr.name not in self.global_types:
                        return f"@{expr.name}"
                    return self.codegen_lvalue_ptr(expr)
                if op == "*":
                    pv = self.codegen_expr(expr)
                    if pv is None:
                        raise CompileError("codegen error: invalid pointer dereference")
                    et = self.expr_type(expr)
                    load_ty = "i32"
                    if et.endswith("*"):
                        load_ty = self.llvm_ty(et[:-1])
                    elif et == "ptr":
                        load_ty = "ptr"
                    t = self.tmp()
                    self.emit(f"  {t} = load {load_ty}, ptr {pv}")
                    return t
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: void value used in unary operation")
                if op == "+":
                    return v
                vty = self.expr_type(expr)
                t = self.tmp()
                if op == "-" and vty in ("double", "float"):
                    fp_ty = self.llvm_ty(vty)
                    self.emit(f"  {t} = fneg {fp_ty} {v}")
                    return t
                # Promote char/i8 values to i32 before arithmetic
                v = self.promote_to_i32(v, vty)
                if op == "-":
                    self.emit(f"  {t} = sub i32 0, {v}")
                    return t
                if op == "!":
                    b = self.tmp()
                    if vty.endswith("*"):
                        self.emit(f"  {b} = icmp eq ptr {v}, null")
                    else:
                        self.emit(f"  {b} = icmp eq i32 {v}, 0")
                    self.emit(f"  {t} = zext i1 {b} to i32")
                    return t
                if op == "~":
                    self.emit(f"  {t} = xor i32 {v}, -1")
                    return t
                raise CompileError(f"codegen error: unsupported unary operator {op!r}")
            case Cast(typ=typ, expr=expr):
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: cast from void is not allowed")
                src = self.expr_type(expr)
                dst = typ
                if src == dst:
                    return v
                if src == "int" and dst == "char":
                    t = self.tmp()
                    self.emit(f"  {t} = trunc i32 {v} to i8")
                    return t
                if src == "char" and dst == "int":
                    t = self.tmp()
                    self.emit(f"  {t} = sext i8 {v} to i32")
                    return t
                if src.endswith("*") and dst.endswith("*"):
                    return v
                if src.endswith("*") and dst == "int":
                    p = self.tmp()
                    q = self.tmp()
                    self.emit(f"  {p} = ptrtoint ptr {v} to i64")
                    self.emit(f"  {q} = trunc i64 {p} to i32")
                    return q
                if src == "int" and dst.endswith("*"):
                    p = self.tmp()
                    q = self.tmp()
                    self.emit(f"  {p} = sext i32 {v} to i64")
                    self.emit(f"  {q} = inttoptr i64 {p} to ptr")
                    return q
                if src == "int" and dst == "int":
                    return v
                # Integer type casts (short, unsigned, etc.) - promote/demote via i32
                int_types = {"int", "short", "unsigned", "unsigned int", "unsigned short", "long", "unsigned long", "long long", "unsigned long long"}
                if src in int_types and dst in int_types:
                    return v  # both are i32 in our IR
                if src in int_types and dst == "char":
                    t = self.tmp()
                    self.emit(f"  {t} = trunc i32 {v} to i8")
                    return t
                if src == "char" and dst in int_types:
                    t = self.tmp()
                    self.emit(f"  {t} = sext i8 {v} to i32")
                    return t
                if src in int_types and dst == "double":
                    t = self.tmp()
                    self.emit(f"  {t} = sitofp i32 {v} to double")
                    return t
                if src == "double" and dst in int_types:
                    t = self.tmp()
                    self.emit(f"  {t} = fptosi double {v} to i32")
                    return t
                if src in int_types and dst.endswith("*"):
                    p = self.tmp()
                    q = self.tmp()
                    self.emit(f"  {p} = sext i32 {v} to i64")
                    self.emit(f"  {q} = inttoptr i64 {p} to ptr")
                    return q
                if dst == "void":
                    return v  # (void)expr - just evaluate and discard
                if dst == "ptr":
                    # Cast to function pointer type — treated as opaque ptr (inttoptr or bitcast)
                    if src in ("int", "long", "unsigned", "unsigned long", "short", "long long", "unsigned long long"):
                        p = self.tmp()
                        q = self.tmp()
                        self.emit(f"  {p} = sext i32 {v} to i64")
                        self.emit(f"  {q} = inttoptr i64 {p} to ptr")
                        return q
                    return v  # already a pointer
                raise CompileError(f"codegen error: unsupported cast from {src!r} to {dst!r}")
            case SizeofExpr(typ=typ, expr=expr):
                if typ is not None:
                    return str(self.sizeof_type(typ))
                if expr is None:
                    raise CompileError("codegen error: invalid sizeof expression")
                return str(self.sizeof_expr(expr))
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                cv = self.codegen_expr(cond)
                if cv is None:
                    raise CompileError("codegen error: void condition in ternary")
                cb = self.tmp()
                cty = self.expr_type(cond)
                if cty.endswith("*"):
                    self.emit(f"  {cb} = icmp ne ptr {cv}, null")
                else:
                    self.emit(f"  {cb} = icmp ne i32 {cv}, 0")
                then_lbl = self.new_label("ternary_then")
                else_lbl = self.new_label("ternary_else")
                end_lbl = self.new_label("ternary_end")
                self.emit(f"  br i1 {cb}, label %{then_lbl}, label %{else_lbl}")
                # --- then branch ---
                self.emit_label(then_lbl)
                tv = self.codegen_expr(then_expr) or "0"
                then_pred = then_lbl  # block that will branch to end_lbl
                then_alive = not self.flow_terminated
                if then_alive:
                    self.emit(f"  br label %{end_lbl}")
                # --- else branch ---
                self.emit_label(else_lbl)
                ev = self.codegen_expr(else_expr) or "0"
                else_pred = else_lbl
                else_alive = not self.flow_terminated
                if else_alive:
                    self.emit(f"  br label %{end_lbl}")
                # --- merge ---
                self.emit_label(end_lbl)
                rty = self.expr_type(n)
                t = self.tmp()
                ll_rty = self.llvm_ty(rty)
                if ll_rty == "ptr":
                    tv = "null" if tv in ("0", "null") else tv
                    ev = "null" if ev in ("0", "null") else ev
                if then_alive and else_alive:
                    self.emit(f"  {t} = phi {ll_rty} [{tv}, %{then_pred}], [{ev}, %{else_pred}]")
                elif then_alive:
                    # only then branch reaches end
                    self.emit(f"  {t} = phi {ll_rty} [{tv}, %{then_pred}]")
                elif else_alive:
                    # only else branch reaches end
                    self.emit(f"  {t} = phi {ll_rty} [{ev}, %{else_pred}]")
                else:
                    # neither branch reaches end (both terminate) — emit undef
                    self.emit(f"  {t} = add {ll_rty} 0, 0")
                return t
            case AssignExpr(target=target, op=op, expr=expr):
                rv = self.codegen_expr(expr)
                if rv is None:
                    raise CompileError("codegen error: cannot assign void to int")
                slot = self.codegen_lvalue_ptr(target)
                target_ty = self.expr_type(target)
                if op == "=":
                    src_ty = self.expr_type(expr)
                    rv = self.emit_convert(rv, src_ty, target_ty)
                    self.emit(f"  store {self.llvm_ty(target_ty)} {rv}, ptr {slot}")
                    return rv
                if target_ty.endswith("*") and op in {"+=", "-="}:
                    step = rv
                    if op == "-=":
                        neg = self.tmp()
                        self.emit(f"  {neg} = sub i32 0, {rv}")
                        step = neg
                    t = self.tmp()
                    curp = self.tmp()
                    self.emit(f"  {curp} = load ptr, ptr {slot}")
                    stride = self.ptr_elem_llvm_ty(target_ty)
                    self.emit(f"  {t} = getelementptr inbounds {stride}, ptr {curp}, i32 {step}")
                    self.emit(f"  store ptr {t}, ptr {slot}")
                    return t
                cur = self.tmp()
                t = self.tmp()
                if target_ty in ("double", "float"):
                    # Float/double compound assignment
                    dop_map = {"+=": "fadd", "-=": "fsub", "*=": "fmul", "/=": "fdiv"}
                    dop = dop_map.get(op)
                    if dop is None:
                        raise CompileError(f"codegen error: unsupported assignment operator {op!r}")
                    fp_ty = self.llvm_ty(target_ty)
                    src_ty = self.expr_type(expr)
                    self.emit(f"  {cur} = load {fp_ty}, ptr {slot}")
                    # For float op= double: promote float to double, compute, store as float
                    arith_ty = "double" if (target_ty == "float" and src_ty == "double") else target_ty
                    arith_ll = self.llvm_ty(arith_ty)
                    lhs_v = self.emit_convert(cur, target_ty, arith_ty)
                    rhs_v = self.emit_convert(rv, src_ty, arith_ty)
                    res = self.tmp()
                    self.emit(f"  {res} = {dop} {arith_ll} {lhs_v}, {rhs_v}")
                    result_v = self.emit_convert(res, arith_ty, target_ty)
                    self.emit(f"  store {fp_ty} {result_v}, ptr {slot}")
                    t = result_v
                else:
                    self.emit(f"  {cur} = load i32, ptr {slot}")
                    op_map = {
                        "+=": "add", "-=": "sub", "*=": "mul", "/=": "sdiv", "%=": "srem",
                        "&=": "and", "|=": "or", "^=": "xor",
                        "<<=": "shl", ">>=": "ashr",
                    }
                    llvm_op = op_map.get(op)
                    if llvm_op is None:
                        raise CompileError(f"codegen error: unsupported assignment operator {op!r}")
                    self.emit(f"  {t} = {llvm_op} i32 {cur}, {rv}")
                    self.emit(f"  store i32 {t}, ptr {slot}")
                return t
            case IncDec(name=name, op=op, prefix=prefix):
                vty = self.resolve_var_type(name)
                slot = self.resolve_var_ptr(name)
                cur = self.tmp()
                self.emit(f"  {cur} = load {self.llvm_ty(vty)}, ptr {slot}")
                nxt = self.tmp()
                if vty.endswith("*"):
                    step = "1" if op == "++" else "-1"
                    stride = self.ptr_elem_llvm_ty(vty)
                    self.emit(f"  {nxt} = getelementptr inbounds {stride}, ptr {cur}, i32 {step}")
                else:
                    llvm_op = "add" if op == "++" else "sub"
                    self.emit(f"  {nxt} = {llvm_op} i32 {cur}, 1")
                self.emit(f"  store {self.llvm_ty(vty)} {nxt}, ptr {slot}")
                return nxt if prefix else cur
            case Call(name=name, args=args):
                # Map __builtin_chk functions to safe alternatives
                builtin_chk_map = {
                    "__builtin___memcpy_chk": ("memcpy", [0, 1, 2]),
                    "__builtin___memset_chk": ("memset", [0, 1, 2]),
                    "__builtin___memmove_chk": ("memmove", [0, 1, 2]),
                    "__builtin___strcpy_chk": ("strcpy", [0, 1]),
                    "__builtin___strncpy_chk": ("strncpy", [0, 1, 2]),
                    "__builtin___strcat_chk": ("strcat", [0, 1]),
                    "__builtin___strncat_chk": ("strncat", [0, 1, 2]),
                    "__builtin___sprintf_chk": ("sprintf", [0, 3]),
                    "__builtin___snprintf_chk": ("snprintf", [0, 1, 4]),
                    "__builtin___vsprintf_chk": ("vsprintf", [0, 3]),
                    "__builtin___vsnprintf_chk": ("vsnprintf", [0, 1, 4]),
                    "__builtin___fprintf_chk": ("fprintf", [0, 2]),
                    "__builtin___vfprintf_chk": ("vfprintf", [0, 2]),
                    "__builtin___vprintf_chk": ("printf", [1]),
                    "__builtin___printf_chk": ("printf", [1]),
                }
                # Map math builtins to standard library equivalents
                if name in _MATH_BUILTIN_MAP:
                    name = _MATH_BUILTIN_MAP[name]
                # Handle __builtin_expect(expr, expected) — just return expr
                if name == "__builtin_expect":
                    if args:
                        v = self.codegen_expr(args[0])
                        return v if v is not None else "0"
                    return "0"
                # Handle __builtin_object_size - return -1 (unknown size)
                if name in ("__builtin_object_size", "__builtin_dynamic_object_size"):
                    # Evaluate args (they may have side effects) but return -1
                    for a in args:
                        try:
                            self.codegen_expr(a)
                        except Exception:
                            pass
                    return "-1"
                if name in builtin_chk_map:
                    real_name, arg_indices = builtin_chk_map[name]
                    # Remap: extract only the needed args by index, rest as variadic
                    new_args = [args[i] for i in arg_indices if i < len(args)]
                    # For sprintf/fprintf etc with variadic, include remaining args after last index
                    last_idx = max(arg_indices) if arg_indices else -1
                    if last_idx + 1 < len(args):
                        new_args.extend(args[last_idx + 1:])
                    name = real_name
                    args = new_args
                # Map __builtin_bswap32/16/64 to LLVM intrinsics
                bswap_map = {
                    "__builtin_bswap16": "llvm.bswap.i16",
                    "__builtin_bswap32": "llvm.bswap.i32",
                    "__builtin_bswap64": "llvm.bswap.i64",
                }
                if name in bswap_map:
                    intrinsic = bswap_map[name]
                    self.used_intrinsics.add(intrinsic)
                    v = self.codegen_expr(args[0]) if args else "0"
                    bw = intrinsic.split(".")[-1]  # i16, i32, i64
                    t = self.tmp()
                    if bw == "i32":
                        self.emit(f"  {t} = call i32 @{intrinsic}(i32 {v})")
                    elif bw == "i64":
                        # extend i32 to i64, bswap, truncate back
                        ext = self.tmp()
                        res64 = self.tmp()
                        self.emit(f"  {ext} = zext i32 {v} to i64")
                        self.emit(f"  {res64} = call i64 @{intrinsic}(i64 {ext})")
                        self.emit(f"  {t} = trunc i64 {res64} to i32")
                    elif bw == "i16":
                        trunc = self.tmp()
                        res16 = self.tmp()
                        self.emit(f"  {trunc} = trunc i32 {v} to i16")
                        self.emit(f"  {res16} = call i16 @{intrinsic}(i16 {trunc})")
                        self.emit(f"  {t} = zext i16 {res16} to i32")
                    else:
                        self.emit(f"  {t} = call {bw} @{intrinsic}({bw} {v})")
                    return t
                sig = self.func_sigs.get(name)
                args_text: List[str] = []
                for i, arg in enumerate(args):
                    v = self.codegen_expr(arg)
                    if v is None:
                        raise CompileError("codegen error: void argument is not allowed")
                    aty = self.expr_type(arg)
                    if sig is not None and i < len(sig.param_types):
                        param_ty = sig.param_types[i]
                        arg_ty = self.llvm_ty(param_ty)
                        v = self.emit_convert(v, aty, param_ty)
                    else:
                        # variadic extra arg or undeclared func - apply C default promotions
                        # char is promoted to int in variadic/unprototyped calls
                        if aty == "char":
                            v = self.promote_to_i32(v, aty)
                            arg_ty = "i32"
                        elif aty in ("long", "unsigned long"):
                            # LP64: long is 64-bit; sign/zero-extend i32 → i64 for variadic
                            t = self.tmp()
                            ext_op = "zext" if aty == "unsigned long" else "sext"
                            self.emit(f"  {t} = {ext_op} i32 {v} to i64")
                            v = t
                            arg_ty = "i64"
                        elif aty == "float":
                            # C default promotion: float → double in variadic calls
                            t = self.tmp()
                            self.emit(f"  {t} = fpext float {v} to double")
                            v = t
                            arg_ty = "double"
                        else:
                            arg_ty = self.llvm_ty(aty)
                    args_text.append(f"{arg_ty} {v}")
                if sig is None:
                    # Check if name is a function pointer variable rather than a function
                    is_fptr_var = (name in self.global_types or name in self.local_types) and name not in self.func_sigs
                    if is_fptr_var:
                        slot = self.slots.get(name) or (f"@{name}" if name in self.global_types else f"%{name}")
                        fptr = self.tmp()
                        self.emit(f"  {fptr} = load ptr, ptr {slot}")
                        t = self.tmp()
                        # Normalize arg types: struct pointers → ptr for indirect calls
                        norm_args = []
                        for at in args_text:
                            parts = at.split(" ", 1)
                            ty, val = (parts[0], parts[1]) if len(parts) == 2 else (at, "")
                            if ty.endswith("*") and ty != "ptr":
                                at = f"ptr {val}"
                            norm_args.append(at)
                        # Try to use the target function's signature for type annotation
                        target_name = self.global_fp_target.get(name)
                        target_sig = self.func_sigs.get(target_name) if target_name else None
                        if target_sig is not None:
                            llvm_ret_ty = self.llvm_ty(target_sig.ret_type)
                            known_tys = ", ".join(self.llvm_ty(pt) for pt in target_sig.param_types)
                            if target_sig.is_variadic:
                                fn_ty = f"{llvm_ret_ty} ({known_tys}, ...)"
                            else:
                                fn_ty = f"{llvm_ret_ty} ({known_tys})"
                            call_instr = f"call {fn_ty} {fptr}({', '.join(norm_args)})"
                        else:
                            call_instr = f"call i32 (...) {fptr}({', '.join(norm_args)})"
                        if target_sig is not None and target_sig.ret_type == "void":
                            self.emit(f"  {call_instr}")
                            return None
                        self.emit(f"  {t} = {call_instr}")
                        return t
                    # undeclared function - assume returns int, auto-declare it
                    if name not in self.auto_declared_fns:
                        self.auto_declared_fns[name] = f"declare i32 @{name}(...)"
                    t = self.tmp()
                    self.emit(f"  {t} = call i32 (...) @{name}({', '.join(args_text)})")
                    return t
                llvm_ret_ty = self.llvm_ty(sig.ret_type)
                if sig.is_variadic:
                    known_tys = ", ".join(self.llvm_ty(pt) for pt in sig.param_types)
                    var_sig = f"{llvm_ret_ty} ({known_tys}, ...)"
                    call_text = f"call {var_sig} @{name}({', '.join(args_text)})"
                else:
                    call_text = f"call {llvm_ret_ty} @{name}({', '.join(args_text)})"
                if sig.ret_type == "void":
                    self.emit(f"  {call_text}")
                    return None
                t = self.tmp()
                self.emit(f"  {t} = {call_text}")
                return t
            case IndirectCall(func=func, args=args):
                # Call through function pointer: fp(args)
                # Strip (*fp) — dereferencing a function pointer is a no-op in C
                if isinstance(func, UnaryOp) and func.op == "*":
                    func = func.expr
                fptr = self.codegen_expr(func)
                if fptr is None:
                    raise CompileError("codegen error: void function pointer")
                args_text = []
                for arg in args:
                    v = self.codegen_expr(arg)
                    if v is None:
                        raise CompileError("codegen error: void argument in indirect call")
                    aty = self.expr_type(arg)
                    if aty == "char":
                        v = self.promote_to_i32(v, aty)
                        arg_ty = "i32"
                    else:
                        arg_ty = self.llvm_ty(aty)
                    args_text.append(f"{arg_ty} {v}")
                t = self.tmp()
                args_str = ", ".join(args_text)
                # Add function type annotation for correct ARM64 ABI
                param_tys = ", ".join(a.rsplit(" ", 1)[0] for a in args_text)
                # Determine return type (may be ptr for functions returning function pointers)
                ret_ty_str = self.expr_type(n)
                ll_ret_ty = self.llvm_ty(ret_ty_str) if ret_ty_str != "void" else "void"
                self.emit(f"  {t} = call {ll_ret_ty} ({param_tys}) {fptr}({args_str})")
                return t
            case CompoundLit(typ=cl_typ, init=cl_init):
                # Compound literal: create an anonymous alloca, init it, return pointer
                gname = f"__cl_{self._current_fn}_{self.tmp_idx}"
                self.tmp_idx += 1
                if cl_typ.startswith("struct:"):
                    ll_ty = self.llvm_ty(cl_typ)
                    slot = f"%{gname}"
                    if gname not in self.slots:
                        self.slots[gname] = slot
                        self.local_types[gname] = cl_typ
                        # alloca was not hoisted, emit it now
                        self.emit(f"  {slot} = alloca {ll_ty}")
                    if isinstance(cl_init, StructInit):
                        self._codegen_local_struct_init(slot, cl_typ, cl_init)
                    return slot
                return "0"
            case StmtExpr(stmts=stmts):
                result: Optional[str] = "0"
                fn_ret = self._current_fn_ret_type
                for idx, stmt in enumerate(stmts):
                    is_last = (idx == len(stmts) - 1)
                    if is_last and isinstance(stmt, ExprStmt):
                        result = self.codegen_expr(stmt.expr) or "0"
                    else:
                        try:
                            self.codegen_stmt(stmt, fn_ret)
                        except Exception:
                            pass
                return result
            case Generic(ctrl=ctrl, assocs=assocs):
                selected = self._generic_select(ctrl, assocs)
                if selected is not None:
                    return self.codegen_expr(selected)
                return "0"
            case _:
                raise CompileError(f"codegen error: unsupported expr {type(n).__name__}")

    def has_label(self, n: Node) -> bool:
        """Recursively check if a node tree contains any LabelStmt."""
        match n:
            case LabelStmt():
                return True
            case Block(body=body):
                return any(self.has_label(st) for st in body)
            case _:
                return False

    def codegen_stmt(self, n: Node, fn_ret_type: str) -> bool:
        match n:
            case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, init=init, is_static=is_static) as decl:
                # Static local variables: use a global with a mangled name
                if is_static:
                    gname = f"__static_{self._current_fn}_{name}"
                    slot = f"@{gname}"
                    if name not in self.slots:
                        self.slots[name] = slot
                        if size is not None:
                            # Static local array
                            self.local_arrays[name] = size
                            self.local_types[name] = "array"
                            self.local_array_elem_types[name] = base_type
                            elem_ll = self.llvm_ty(base_type)
                            ll_ty = f"[{size} x {elem_ll}]"
                            self.preamble_lines.append(f"@{gname} = internal global {ll_ty} zeroinitializer")
                        else:
                            vty = base_type + ("*" * ptr_level)
                            self.local_types[name] = vty
                            # Emit as preamble global
                            ll_ty = self.llvm_ty(vty)
                            # Choose a safe zero-value: structs/arrays need zeroinitializer
                            is_aggregate = ll_ty.startswith("%struct.") or ll_ty.startswith("[")
                            zero_val = "zeroinitializer" if is_aggregate else "0"
                            if init is not None:
                                try:
                                    iv = self.eval_global_const(init)
                                    zero_val = str(iv)
                                except Exception:
                                    pass  # keep zeroinitializer/0 as default
                            self.preamble_lines.append(f"@{gname} = internal global {ll_ty} {zero_val}")
                    return False
                slot = f"%{name}"
                already_hoisted = name in self.slots
                if not already_hoisted:
                    self.slots[name] = slot
                if size is not None:
                    if not already_hoisted:
                        self.local_arrays[name] = size
                        self.local_types[name] = "array"
                        self.emit(f"  {slot} = alloca [{size} x i32]")
                    # Initialize array if an initializer is present
                    if init is not None:
                        elem_ty = self.local_array_elem_types.get(name, base_type)
                        elem_ll = self.llvm_ty(elem_ty)
                        if isinstance(init, StringLit) and elem_ty == "char":
                            raw = init.value
                            for idx, ch in enumerate(raw):
                                c = ord(ch) if isinstance(ch, str) else ch
                                ep = self.tmp()
                                self.emit(f"  {ep} = getelementptr inbounds [{size} x i8], ptr {slot}, i32 0, i32 {idx}")
                                self.emit(f"  store i8 {c}, ptr {ep}")
                            # null terminator
                            ep = self.tmp()
                            self.emit(f"  {ep} = getelementptr inbounds [{size} x i8], ptr {slot}, i32 0, i32 {len(raw)}")
                            self.emit(f"  store i8 0, ptr {ep}")
                        elif isinstance(init, ArrayInit):
                            for idx, val in enumerate(init.values):
                                ep = self.tmp()
                                self.emit(f"  {ep} = getelementptr inbounds [{size} x {elem_ll}], ptr {slot}, i32 0, i32 {idx}")
                                self.emit(f"  store {elem_ll} {val}, ptr {ep}")
                    return False
                if not already_hoisted:
                    self.local_types[name] = base_type + ("*" * ptr_level)
                    if decl.full_type:
                        self.local_full_types[name] = decl.full_type
                    self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[name])}")
                if init is not None:
                    if isinstance(init, StructInit):
                        # Initialize struct fields one by one
                        self._codegen_local_struct_init(slot, self.local_types[name], init)
                    else:
                        v = self.codegen_expr(init)
                        if v is None:
                            raise CompileError("codegen error: cannot initialize variable with void")
                        v = self.emit_convert(v, self.expr_type(init), self.local_types[name])
                        self.emit(f"  store {self.llvm_ty(self.local_types[name])} {v}, ptr {slot}")
                        # Track return type if this fptr is initialized from a known function
                        if isinstance(init, Var) and init.name in self.func_sigs:
                            self.local_fptr_rets[name] = self.func_sigs[init.name].ret_type
                return False
            case Block(body=body):
                terminated = False
                for st in body:
                    if terminated:
                        if isinstance(st, (LabelStmt, Case, Default)):
                            terminated = self.codegen_stmt(st, fn_ret_type)
                        elif isinstance(st, Block):
                            # Recurse into blocks containing labels; skip label-free blocks
                            if self.has_label(st):
                                terminated = self.codegen_stmt(st, fn_ret_type)
                        elif isinstance(st, Decl):
                            # Always emit alloca even in dead code
                            self.codegen_stmt(st, fn_ret_type)
                        continue
                    terminated = self.codegen_stmt(st, fn_ret_type)
                if terminated:
                    return True
                return False
            case If(cond=cond, then_stmt=then_stmt, else_stmt=else_stmt):
                cond_v = self.codegen_expr(cond)
                if cond_v is None:
                    raise CompileError("codegen error: void condition in if")
                cond_b = self.tmp()
                cty = self.expr_type(cond)
                if cty.endswith("*"):
                    self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                else:
                    self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                then_lbl = self.new_label("if_then")
                end_lbl = self.new_label("if_end")
                if else_stmt is None:
                    self.emit(f"  br i1 {cond_b}, label %{then_lbl}, label %{end_lbl}")
                    self.emit_label(then_lbl)
                    then_ret = self.codegen_stmt(then_stmt, fn_ret_type)
                    if not then_ret:
                        self.emit(f"  br label %{end_lbl}")
                    self.emit_label(end_lbl)
                    return False
                else_lbl = self.new_label("if_else")
                self.emit(f"  br i1 {cond_b}, label %{then_lbl}, label %{else_lbl}")
                self.emit_label(then_lbl)
                then_ret = self.codegen_stmt(then_stmt, fn_ret_type)
                if not then_ret:
                    self.emit(f"  br label %{end_lbl}")
                self.emit_label(else_lbl)
                else_ret = self.codegen_stmt(else_stmt, fn_ret_type)
                if not else_ret:
                    self.emit(f"  br label %{end_lbl}")
                if then_ret and else_ret:
                    return True
                self.emit_label(end_lbl)
                return False
            case While(cond=cond, body=body):
                cond_lbl = self.new_label("while_cond")
                body_lbl = self.new_label("while_body")
                end_lbl = self.new_label("while_end")
                self.emit(f"  br label %{cond_lbl}")
                self.emit_label(cond_lbl)
                cond_v = self.codegen_expr(cond)
                if cond_v is None:
                    raise CompileError("codegen error: void condition in while")
                cond_b = self.tmp()
                cty = self.expr_type(cond)
                if cty.endswith("*"):
                    self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                else:
                    self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(body_lbl)
                self.loop_stack.append((end_lbl, cond_lbl))
                self.break_stack.append(end_lbl)
                body_ret = self.codegen_stmt(body, fn_ret_type)
                self.break_stack.pop()
                self.loop_stack.pop()
                if not body_ret:
                    self.emit(f"  br label %{cond_lbl}")
                self.emit_label(end_lbl)
                return False
            case For(init=init, cond=cond, post=post, body=body):
                if init is not None:
                    self.codegen_expr(init)
                cond_lbl = self.new_label("for_cond")
                body_lbl = self.new_label("for_body")
                post_lbl = self.new_label("for_post")
                end_lbl = self.new_label("for_end")
                self.emit(f"  br label %{cond_lbl}")
                self.emit_label(cond_lbl)
                if cond is None:
                    self.emit(f"  br label %{body_lbl}")
                else:
                    cond_v = self.codegen_expr(cond)
                    if cond_v is None:
                        raise CompileError("codegen error: void condition in for")
                    cond_b = self.tmp()
                    cty = self.expr_type(cond)
                    if cty.endswith("*"):
                        self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                    else:
                        self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                    self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(body_lbl)
                self.loop_stack.append((end_lbl, post_lbl))
                self.break_stack.append(end_lbl)
                body_ret = self.codegen_stmt(body, fn_ret_type)
                self.break_stack.pop()
                self.loop_stack.pop()
                if not body_ret:
                    self.emit(f"  br label %{post_lbl}")
                self.emit_label(post_lbl)
                if post is not None:
                    self.codegen_expr(post)
                self.emit(f"  br label %{cond_lbl}")
                self.emit_label(end_lbl)
                return False
            case DoWhile(body=body, cond=cond):
                body_lbl = self.new_label("do_body")
                cond_lbl = self.new_label("do_cond")
                end_lbl = self.new_label("do_end")
                self.emit(f"  br label %{body_lbl}")
                self.emit_label(body_lbl)
                self.loop_stack.append((end_lbl, cond_lbl))
                self.break_stack.append(end_lbl)
                body_ret = self.codegen_stmt(body, fn_ret_type)
                self.break_stack.pop()
                self.loop_stack.pop()
                if not body_ret:
                    self.emit(f"  br label %{cond_lbl}")
                self.emit_label(cond_lbl)
                cond_v = self.codegen_expr(cond)
                if cond_v is None:
                    raise CompileError("codegen error: void condition in do-while")
                cond_b = self.tmp()
                cty = self.expr_type(cond)
                if cty.endswith("*"):
                    self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                else:
                    self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(end_lbl)
                return False
            case Switch(expr=expr, body=body):
                return self.codegen_switch_stmt(expr, body, fn_ret_type)
            case Case(value=value, stmt=stmt):
                if not self.switch_case_stack:
                    raise CompileError("codegen error: case used outside switch")
                cv = self.eval_global_const(value)
                lbl = self.switch_case_stack[-1].get(cv)
                if lbl is None:
                    raise CompileError("codegen error: unknown case label")
                self.emit(f"  br label %{lbl}")
                self.emit_label(lbl)
                return self.codegen_stmt(stmt, fn_ret_type)
            case Default(stmt=stmt):
                if not self.switch_default_stack:
                    raise CompileError("codegen error: default used outside switch")
                lbl = self.switch_default_stack[-1]
                if lbl is None:
                    raise CompileError("codegen error: unknown default label")
                self.emit(f"  br label %{lbl}")
                self.emit_label(lbl)
                return self.codegen_stmt(stmt, fn_ret_type)
            case Break():
                if not self.break_stack:
                    raise CompileError("codegen error: break used outside loop")
                break_lbl = self.break_stack[-1]
                self.emit(f"  br label %{break_lbl}")
                return True
            case Continue():
                if not self.loop_stack:
                    raise CompileError("codegen error: continue used outside loop")
                _, cont_lbl = self.loop_stack[-1]
                self.emit(f"  br label %{cont_lbl}")
                return True
            case Goto(name=name):
                lbl = self.user_labels.get(name)
                if lbl is None:
                    raise CompileError(f"codegen error: goto to unknown label {name!r}")
                self.emit(f"  br label %{lbl}")
                self.flow_terminated = True
                return True
            case EmptyStmt():
                return False
            case LabelStmt(name=name, stmt=stmt):
                return self.codegen_label_stmt(name, stmt, fn_ret_type, False)
            case Assign(name=name, expr=expr):
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: cannot assign void to int")
                target_ty = self.resolve_var_type(name)
                src_ty = self.expr_type(expr)
                v = self.emit_convert(v, src_ty, target_ty)
                self.emit(
                    f"  store {self.llvm_ty(target_ty)} {v}, ptr {self.resolve_var_ptr(name)}"
                )
                return False
            case ExprStmt(expr=expr):
                self.codegen_expr(expr)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    self.emit("  ret void")
                else:
                    if expr is None:
                        raise CompileError("codegen error: non-void function must return value")
                    v = self.codegen_expr(expr)
                    if v is None:
                        raise CompileError("codegen error: non-void function cannot return void")
                    v = self.emit_convert(v, self.expr_type(expr), fn_ret_type)
                    self.emit(f"  ret {self.llvm_ty(fn_ret_type)} {v}")
                return True
            case _:
                raise CompileError(f"codegen error: unsupported stmt {type(n).__name__}")

    def codegen_label_stmt(
        self,
        name: str,
        stmt: Node,
        fn_ret_type: str,
        from_terminated: bool,
    ) -> bool:
        lbl = self.user_labels.get(name)
        if lbl is None:
            raise CompileError(f"codegen error: unknown label {name!r}")
        # Emit fall-through branch only if current flow is not already terminated
        if not from_terminated and not self.flow_terminated:
            self.emit(f"  br label %{lbl}")
        self.flow_terminated = False
        self.emit_label(lbl)
        return self.codegen_stmt(stmt, fn_ret_type)

    def collect_switch_labels(
        self, n: Node, case_map: Dict[int, str], default_ref: List[Optional[str]]
    ) -> None:
        match n:
            case Case(value=value, stmt=stmt):
                cv = self.eval_global_const(value)
                if cv in case_map:
                    raise CompileError(f"codegen error: duplicate case label {cv}")
                case_map[cv] = self.new_label(f"switch_case_{cv}_")
                self.collect_switch_labels(stmt, case_map, default_ref)
            case Default(stmt=stmt):
                if default_ref[0] is not None:
                    raise CompileError("codegen error: duplicate default label")
                default_ref[0] = self.new_label("switch_default_")
                self.collect_switch_labels(stmt, case_map, default_ref)
            case LabelStmt(stmt=stmt):
                self.collect_switch_labels(stmt, case_map, default_ref)
            case Block(body=body):
                for st in body:
                    self.collect_switch_labels(st, case_map, default_ref)
            case If(then_stmt=then_stmt, else_stmt=else_stmt):
                self.collect_switch_labels(then_stmt, case_map, default_ref)
                if else_stmt is not None:
                    self.collect_switch_labels(else_stmt, case_map, default_ref)
            case While(body=body):
                self.collect_switch_labels(body, case_map, default_ref)
            case For(body=body):
                self.collect_switch_labels(body, case_map, default_ref)
            case DoWhile(body=body):
                self.collect_switch_labels(body, case_map, default_ref)

    def codegen_switch_stmt(self, expr: Node, body: Node, fn_ret_type: str) -> bool:
        sv = self.codegen_expr(expr)
        if sv is None:
            raise CompileError("codegen error: void switch expression")
        end_lbl = self.new_label("switch_end")
        dispatch_lbl = self.new_label("switch_dispatch")
        self.emit(f"  br label %{dispatch_lbl}")

        case_map: Dict[int, str] = {}
        default_ref: List[Optional[str]] = [None]
        self.collect_switch_labels(body, case_map, default_ref)

        self.emit_label(dispatch_lbl)
        cur_lbl = dispatch_lbl
        for cv, target in case_map.items():
            cmp = self.tmp()
            self.emit(f"  {cmp} = icmp eq i32 {sv}, {cv}")
            next_lbl = self.new_label("switch_next")
            self.emit(f"  br i1 {cmp}, label %{target}, label %{next_lbl}")
            self.emit_label(next_lbl)
            cur_lbl = next_lbl
        fall_lbl = default_ref[0] if default_ref[0] is not None else end_lbl
        if cur_lbl != dispatch_lbl:
            self.emit(f"  br label %{fall_lbl}")
        else:
            self.emit(f"  br label %{fall_lbl}")

        self.break_stack.append(end_lbl)
        self.switch_case_stack.append(case_map)
        self.switch_default_stack.append(default_ref[0])
        body_ret = self.codegen_stmt(body, fn_ret_type)
        self.switch_default_stack.pop()
        self.switch_case_stack.pop()
        self.break_stack.pop()
        if not body_ret:
            self.emit(f"  br label %{end_lbl}")
        self.emit_label(end_lbl)
        return False

    def emit_declarations(self) -> None:
        def collect_struct_tag(ty: str) -> Optional[str]:
            base = ty
            while base.endswith("*"):
                base = base[:-1]
            if base.startswith("struct:"):
                return base.split(":", 1)[1]
            return None

        referenced_tags = set()
        for gty in self.global_types.values():
            tag = collect_struct_tag(gty)
            if tag is not None:
                referenced_tags.add(tag)
        for sig in self.func_sigs.values():
            tag = collect_struct_tag(sig.ret_type)
            if tag is not None:
                referenced_tags.add(tag)
            for pty in sig.param_types:
                tag = collect_struct_tag(pty)
                if tag is not None:
                    referenced_tags.add(tag)
        for s in self.struct_defs.values():
            for f in s.fields:
                tag = collect_struct_tag(f.typ)
                if tag is not None:
                    referenced_tags.add(tag)

        defined_tags = set(self.struct_defs.keys())
        for tag, s in self.struct_defs.items():
            if s.is_union:
                # For unions, emit only the first (largest) field to overlay all members
                if s.fields:
                    field_tys = self.llvm_ty(s.fields[0].typ)
                else:
                    field_tys = "i32"
                self.emit(f"%struct.{tag} = type {{{field_tys}}}")
            else:
                field_tys = ", ".join(
                    f"[{f.array_size} x {self.llvm_ty(f.typ)}]" if f.array_size is not None else self.llvm_ty(f.typ)
                    for f in s.fields
                )
                self.emit(f"%struct.{tag} = type {{{field_tys}}}")
        for tag in sorted(referenced_tags - defined_tags):
            self.emit(f"%struct.{tag} = type opaque")
        if self.struct_defs:
            self.emit("")

        for name, init in self.global_vars.items():
            # extern-declared globals with no explicit init → emit as external reference
            if self.global_is_extern.get(name) and init is None:
                gty = self.resolve_var_type(name)
                if name in self.global_arrays:
                    pass  # extern arrays: skip (let linker resolve)
                elif gty.endswith("*"):
                    self.emit(f"@{name} = external global ptr")
                elif self.is_struct_type(gty):
                    self.emit(f"@{name} = external global {self.llvm_ty(gty)}")
                else:
                    self.emit(f"@{name} = external global {self.llvm_ty(gty)}")
                continue
            if name in self.global_arrays:
                n_elem = self.global_arrays[name]
                elem_ty = self.global_types.get(name, "int")
                if elem_ty == "char_array":
                    # char array initialized from string literal
                    str_node = init
                    if str_node is not None and isinstance(str_node, StringLit):
                        raw = str_node.value  # already unescaped bytes
                        # Encode as LLVM c"..." literal
                        encoded = ""
                        for ch in raw:
                            c = ord(ch) if isinstance(ch, str) else ch
                            if 32 <= c < 127 and c not in (ord('"'), ord('\\'), ord('\n'), ord('\r'), ord('\t')):
                                encoded += chr(c)
                            else:
                                encoded += f"\\{c:02X}"
                        encoded += "\\00"
                        n_bytes = len(raw) + 1
                        self.emit(f"@{name} = global [{n_bytes} x i8] c\"{encoded}\"")
                    elif str_node is not None and isinstance(str_node, ArrayInit):
                        # char array with integer initializer list
                        vals_str = ", ".join(f"i8 {v}" for v in str_node.values)
                        self.emit(f"@{name} = global [{n_elem} x i8] [{vals_str}]")
                    else:
                        self.emit(f"@{name} = global [{n_elem} x i8] zeroinitializer")
                else:
                    # int/struct array
                    arr_elem = self.global_array_elem_types.get(name, "int")
                    arr_elem_ll = self.llvm_ty(arr_elem)
                    is_struct_elem = self.is_struct_type(arr_elem)
                    if init is not None and isinstance(init, StructArrayInit) and is_struct_elem:
                        # Array of structs with struct initializers
                        entry_map = {idx: node for idx, node in init.entries if node is not None}
                        elems = []
                        for i in range(n_elem):
                            node = entry_map.get(i)
                            if node is not None and isinstance(node, StructInit):
                                elems.append(f"{arr_elem_ll} {self._emit_struct_init_const(arr_elem, node)}")
                            else:
                                elems.append(f"{arr_elem_ll} zeroinitializer")
                        self.emit(f"@{name} = global [{n_elem} x {arr_elem_ll}] [{', '.join(elems)}]")
                    elif init is not None and isinstance(init, ArrayInit) and not is_struct_elem:
                        vals_str = ", ".join(f"{arr_elem_ll} {v}" for v in init.values)
                        self.emit(f"@{name} = global [{n_elem} x {arr_elem_ll}] [{vals_str}]")
                    else:
                        self.emit(f"@{name} = global [{n_elem} x {arr_elem_ll}] zeroinitializer")
                continue
            gty = self.resolve_var_type(name)
            if gty.endswith("*"):
                # Check for address-of initializer (pointer init)
                if init is not None and not isinstance(init, IntLit):
                    try:
                        init_val_str = self._global_ptr_init(init)
                        self.emit(f"@{name} = global ptr {init_val_str}")
                    except Exception:
                        self.emit(f"@{name} = global ptr null")
                else:
                    self.emit(f"@{name} = global ptr null")
                continue
            if self.is_struct_type(gty):
                if init is not None and isinstance(init, StructInit):
                    init_str = self._emit_struct_init_const(gty, init)
                    self.emit(f"@{name} = global {self.llvm_ty(gty)} {init_str}")
                else:
                    self.emit(f"@{name} = global {self.llvm_ty(gty)} zeroinitializer")
                continue
            if gty in ("double", "float"):
                init_val = 0 if init is None else self.eval_global_const(init)
                fp_ty = self.llvm_ty(gty)
                self.emit(f"@{name} = global {fp_ty} {float(init_val)}")
                continue
            init_val = 0 if init is None else self.eval_global_const(init)
            self.emit(f"@{name} = global i32 {init_val}")
        if self.global_vars:
            self.emit("")

        defined = {fn.name for fn in self.prog.funcs}
        seen: Dict[str, bool] = {}
        for d in self.prog.decls:
            if d.name in defined or d.name in seen:
                continue
            seen[d.name] = True
            ret_ty = self.llvm_ty(d.ret_type)
            non_var_params = [p for p in d.params if p.typ != "..."]
            is_var_d = any(p.typ == "..." for p in d.params)
            parts = [self.llvm_ty(p.typ) for p in non_var_params]
            if is_var_d:
                parts.append("...")
            self.emit(f"declare {ret_ty} @{d.name}({', '.join(parts)})")

    def codegen_function(self, fn: Function) -> None:
        self.tmp_idx = 0
        self.label_idx = 0
        self.slots = {}
        self.local_arrays = {}
        self.local_types = {}
        self.local_full_types: Dict[str, str] = {}  # const-qualified types for _Generic matching
        self.local_array_elem_types = {}
        self.local_fptr_rets: Dict[str, str] = {}  # fptr var → return type of the pointed-to function
        self._current_fn = fn.name
        self._current_fn_ret_type = fn.ret_type
        self.flow_terminated = False  # track if current flow is terminated (dead code)
        self.loop_stack = []
        self.break_stack = []
        self.switch_case_stack = []
        self.switch_default_stack = []
        self.user_labels = {}
        self.collect_codegen_labels(fn.body)

        ret_ty = self.llvm_ty(fn.ret_type)
        params_sig: List[str] = []
        is_variadic_fn = any(p.typ == "..." for p in fn.params)
        for i, p in enumerate(fn.params):
            if p.typ == "...":
                continue
            pname = p.name if p.name is not None else f"arg{i}"
            params_sig.append(f"{self.llvm_ty(p.typ)} %{pname}.arg")
        if is_variadic_fn:
            params_sig.append("...")
        self.emit(f"define {ret_ty} @{fn.name}({', '.join(params_sig)}) {{")
        self.emit("entry:")

        for i, p in enumerate(fn.params):
            if p.typ == "...":
                continue
            pname = p.name if p.name is not None else f"arg{i}"
            slot = f"%{pname}"
            self.slots[pname] = slot
            self.local_types[pname] = p.typ
            self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[pname])}")
            self.emit(
                f"  store {self.llvm_ty(self.local_types[pname])} %{pname}.arg, ptr {slot}"
            )

        # Hoist all allocas to entry block (required for correct goto semantics)
        self.hoist_allocas(fn.body)

        terminated = False
        for st in fn.body:
            if terminated:
                if isinstance(st, LabelStmt):
                    terminated = self.codegen_label_stmt(
                        st.name, st.stmt, fn.ret_type, True
                    )
                elif isinstance(st, Block) and self.has_label(st):
                    # Recurse into blocks that contain labels
                    terminated = self.codegen_stmt(st, fn.ret_type)
                # skip all other dead code
                continue
            terminated = self.codegen_stmt(st, fn.ret_type)

        if not terminated:
            if fn.ret_type == "void":
                self.emit("  ret void")
            else:
                default_ret = "null" if fn.ret_type.endswith("*") else "0"
                self.emit(f"  ret {self.llvm_ty(fn.ret_type)} {default_ret}")

        self.emit("}")

    def hoist_allocas(self, body: List[Node]) -> None:
        """Pre-scan function body and emit all alloca instructions up front."""
        for st in body:
            match st:
                case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, is_static=is_static_d, extra_dims=extra_dims, full_type=full_type):
                    if is_static_d:
                        continue  # static locals are handled as globals, not alloca
                    if name in self.slots:
                        continue  # already hoisted
                    slot = f"%{name}"
                    self.slots[name] = slot
                    if size is not None:
                        # Multi-dimensional: flatten total size and track stride
                        total_size = size
                        stride = 1
                        if extra_dims:
                            for d in extra_dims:
                                stride *= d
                            total_size = size * stride
                            self.local_array_strides[name] = stride
                        self.local_arrays[name] = total_size
                        self.local_types[name] = "array"
                        self.local_array_elem_types[name] = base_type
                        elem_ll = self.llvm_ty(base_type)
                        self.emit(f"  {slot} = alloca [{total_size} x {elem_ll}]")
                    else:
                        self.local_types[name] = base_type + ("*" * ptr_level)
                        if full_type:
                            self.local_full_types[name] = full_type
                        # pointer-to-array: record stride from extra_dims (e.g. (*p)[4] → stride=4)
                        if extra_dims and ptr_level > 0:
                            stride = 1
                            for d in extra_dims:
                                stride *= d
                            self.local_array_strides[name] = stride
                            self.local_array_elem_types[name] = base_type
                        self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[name])}")
                case Block(body=inner_body):
                    self.hoist_allocas(inner_body)
                case If(then_stmt=then_stmt, else_stmt=else_stmt):
                    self.hoist_allocas([then_stmt])
                    if else_stmt:
                        self.hoist_allocas([else_stmt])
                case For(init=init, body=for_body):
                    if init:
                        self.hoist_allocas([init])
                    self.hoist_allocas([for_body])
                case While(body=while_body):
                    self.hoist_allocas([while_body])
                case DoWhile(body=do_body):
                    self.hoist_allocas([do_body])
                case Switch(body=sw_body):
                    self.hoist_allocas([sw_body])
                case LabelStmt(stmt=lbl_stmt):
                    self.hoist_allocas([lbl_stmt])
                case ExprStmt(expr=expr_inner):
                    self._hoist_allocas_from_expr(expr_inner)
                case Assign(expr=assign_expr):
                    self._hoist_allocas_from_expr(assign_expr)
                case StmtExpr(stmts=se_stmts):
                    self.hoist_allocas(se_stmts)

    def _hoist_allocas_from_expr(self, n: Node) -> None:
        """Hoist allocas from expressions (e.g. statement expressions inside ternary)."""
        match n:
            case StmtExpr(stmts=stmts):
                self.hoist_allocas(stmts)
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                self._hoist_allocas_from_expr(cond)
                self._hoist_allocas_from_expr(then_expr)
                self._hoist_allocas_from_expr(else_expr)
            case BinOp(lhs=lhs, rhs=rhs):
                self._hoist_allocas_from_expr(lhs)
                self._hoist_allocas_from_expr(rhs)
            case UnaryOp(expr=expr):
                self._hoist_allocas_from_expr(expr)
            case Call(args=args):
                for a in args:
                    self._hoist_allocas_from_expr(a)
            case AssignExpr(target=target, expr=expr):
                self._hoist_allocas_from_expr(target)
                self._hoist_allocas_from_expr(expr)
            case Assign(expr=expr):
                self._hoist_allocas_from_expr(expr)

    def collect_codegen_labels(self, body: List[Node]) -> None:
        for st in body:
            self.collect_codegen_label_stmt(st)

    def collect_codegen_label_stmt(self, n: Node) -> None:
        match n:
            case LabelStmt(name=name, stmt=stmt):
                if name in self.user_labels:
                    raise CompileError(f"codegen error: duplicate label {name!r}")
                self.user_labels[name] = self.new_label(f"user_{name}_")
                self.collect_codegen_label_stmt(stmt)
            case Block(body=body):
                for st in body:
                    self.collect_codegen_label_stmt(st)
            case If(then_stmt=then_stmt, else_stmt=else_stmt):
                self.collect_codegen_label_stmt(then_stmt)
                if else_stmt is not None:
                    self.collect_codegen_label_stmt(else_stmt)
            case While(body=body):
                self.collect_codegen_label_stmt(body)
            case For(body=body):
                self.collect_codegen_label_stmt(body)
            case DoWhile(body=body):
                self.collect_codegen_label_stmt(body)
            case Switch(body=body):
                self.collect_codegen_label_stmt(body)
            case Case(stmt=stmt):
                self.collect_codegen_label_stmt(stmt)
            case Default(stmt=stmt):
                self.collect_codegen_label_stmt(stmt)
            case ExprStmt(expr=expr_inner):
                self._collect_labels_from_expr(expr_inner)
            case Assign(expr=assign_expr):
                self._collect_labels_from_expr(assign_expr)
            case StmtExpr(stmts=se_stmts):
                for se_st in se_stmts:
                    self.collect_codegen_label_stmt(se_st)

    def _collect_labels_from_expr(self, n: Node) -> None:
        """Recursively collect user labels from within expressions (e.g. statement expressions)."""
        match n:
            case StmtExpr(stmts=stmts):
                for st in stmts:
                    self.collect_codegen_label_stmt(st)
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                self._collect_labels_from_expr(cond)
                self._collect_labels_from_expr(then_expr)
                self._collect_labels_from_expr(else_expr)
            case BinOp(lhs=lhs, rhs=rhs):
                self._collect_labels_from_expr(lhs)
                self._collect_labels_from_expr(rhs)
            case UnaryOp(expr=expr):
                self._collect_labels_from_expr(expr)
            case Call(args=args):
                for a in args:
                    self._collect_labels_from_expr(a)
            case AssignExpr(target=target, expr=expr):
                self._collect_labels_from_expr(target)
                self._collect_labels_from_expr(expr)
            case Cast(expr=expr):
                self._collect_labels_from_expr(expr)

    def _global_ptr_init(self, init: Node) -> str:
        """Return LLVM constant expression string for a global pointer initializer."""
        match init:
            case IntLit(value=0):
                return "null"
            case UnaryOp(op="&", expr=Var(name=vname)):
                return f"@{vname}"
            case UnaryOp(op="&", expr=CompoundLit(typ=cl_typ, init=cl_init)):
                # Compound literal in global context: emit anonymous global inline
                gname = f"__cl_{self.string_idx}"
                self.string_idx += 1
                ll_ty = self.llvm_ty(cl_typ)
                if cl_typ.startswith("struct:") and isinstance(cl_init, StructInit):
                    const_val = self._emit_struct_init_const(cl_typ, cl_init)
                else:
                    const_val = "zeroinitializer" if ll_ty.startswith("%struct.") else "0"
                self.emit(f"@{gname} = internal global {ll_ty} {const_val}")
                return f"@{gname}"
            case CompoundLit(typ=cl_typ, init=cl_init):
                # Compound literal value (not &) used as global init — emit anonymous global
                gname = f"__cl_{self.string_idx}"
                self.string_idx += 1
                ll_ty = self.llvm_ty(cl_typ)
                if cl_typ.startswith("struct:") and isinstance(cl_init, StructInit):
                    const_val = self._emit_struct_init_const(cl_typ, cl_init)
                    self.emit(f"@{gname} = internal global {ll_ty} {const_val}")
                    return f"@{gname}"
                return "null"
            case _:
                try:
                    v = self.eval_global_const(init)
                    if v == 0:
                        return "null"
                    return "null"  # can't represent non-null non-address constant
                except Exception:
                    return "null"

    def _codegen_local_struct_init(self, slot: str, struct_ty: str, init: "StructInit") -> None:
        """Generate stores to initialize a local struct variable from a StructInit node."""
        if ":" not in struct_ty:
            return
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            return
        ll_ty = self.llvm_ty(struct_ty)
        named = any(fname is not None for fname, _ in init.entries)
        if named:
            field_map: dict = {}
            cur_pos = 0
            for fname, val_node in init.entries:
                if fname is not None:
                    field_map[fname] = val_node
                else:
                    if cur_pos < len(s.fields):
                        field_map[s.fields[cur_pos].name] = val_node
                    cur_pos += 1
            for i, f in enumerate(s.fields):
                v_node = field_map.get(f.name)
                if v_node is None:
                    continue
                t = self.tmp()
                self.emit(f"  {t} = getelementptr inbounds {ll_ty}, ptr {slot}, i32 0, i32 {i}")
                if isinstance(v_node, StructInit):
                    self._codegen_local_struct_init(t, f.typ, v_node)
                else:
                    v = self.codegen_expr(v_node)
                    if v is not None:
                        fll = self.llvm_ty(f.typ)
                        self.emit(f"  store {fll} {v}, ptr {t}")
        else:
            for i, f in enumerate(s.fields):
                if i >= len(init.entries):
                    break
                _, v_node = init.entries[i]
                t = self.tmp()
                self.emit(f"  {t} = getelementptr inbounds {ll_ty}, ptr {slot}, i32 0, i32 {i}")
                if isinstance(v_node, StructInit):
                    self._codegen_local_struct_init(t, f.typ, v_node)
                else:
                    v = self.codegen_expr(v_node)
                    if v is not None:
                        fll = self.llvm_ty(f.typ)
                        self.emit(f"  store {fll} {v}, ptr {t}")

    def _zero_const(self, typ: str) -> str:
        """Return LLVM zero constant string (with type prefix) for a given C type."""
        ll_ty = self.llvm_ty(typ)
        if ll_ty == "ptr":
            return "ptr null"
        if ll_ty == "i8":
            return "i8 0"
        if ll_ty == "double":
            return "double 0.0"
        if self.is_struct_type(typ):
            return f"{ll_ty} zeroinitializer"
        # array types, etc.
        if ll_ty.startswith("["):
            return f"{ll_ty} zeroinitializer"
        return f"{ll_ty} 0"

    def _emit_struct_init_const(self, struct_ty: str, init: "StructInit") -> str:
        """Emit LLVM struct constant initializer string."""
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            return "zeroinitializer"
        fields = s.fields
        # Build field_name -> value mapping
        # StructInit entries: (Optional[str], Node) pairs
        # Positional if no field names
        named = any(fname is not None for fname, _ in init.entries)
        def field_ty(f: "StructField") -> str:
            """Return effective C type for struct field (handles array fields)."""
            if f.array_size is not None:
                return f"arr_field:{f.array_size}:{f.typ}"
            return f.typ

        if named:
            # Map field name -> node (designated initializers)
            # Positional entries continue from after the last designated field
            field_map: dict = {}
            field_names = [f.name for f in fields]
            cur_pos = 0
            for fname, val_node in init.entries:
                if fname is not None:
                    field_map[fname] = val_node
                    # Advance cur_pos to field after fname
                    if fname in field_names:
                        cur_pos = field_names.index(fname) + 1
                else:
                    # Positional: continue from cur_pos
                    if cur_pos < len(fields):
                        field_map[fields[cur_pos].name] = val_node
                        cur_pos += 1
            vals = []
            for f in fields:
                fty = field_ty(f)
                v_node = field_map.get(f.name)
                if v_node is None:
                    vals.append(self._zero_const(fty))
                else:
                    vals.append(self._const_node_to_llvm(v_node, fty))
        else:
            # Positional - entries may be flat (one per scalar) or one per field
            vals = []
            flat_idx = 0  # index into init.entries for flat initialization
            for f in fields:
                fty = field_ty(f)
                if flat_idx >= len(init.entries):
                    vals.append(self._zero_const(fty))
                elif fty.startswith("arr_field:"):
                    # Array field: check if the next entry is a StructInit/ArrayInit (nested)
                    # or if we have flat scalars to collect
                    parts = fty.split(":", 2)
                    arr_n = int(parts[1])
                    elem_ty = parts[2]
                    elem_ll = self.llvm_ty(elem_ty)
                    _, v_node = init.entries[flat_idx]
                    if isinstance(v_node, (StructInit, ArrayInit)):
                        # Nested brace initializer for the array
                        vals.append(self._const_node_to_llvm(v_node, fty))
                        flat_idx += 1
                    else:
                        # Flat scalars: consume arr_n entries for this array field
                        arr_vals = []
                        for _ in range(arr_n):
                            if flat_idx < len(init.entries):
                                _, sv = init.entries[flat_idx]
                                flat_idx += 1
                                try:
                                    arr_vals.append(self.eval_global_const(sv))
                                except Exception:
                                    arr_vals.append(0)
                            else:
                                arr_vals.append(0)
                        while len(arr_vals) < arr_n:
                            arr_vals.append(0)
                        vals_str = ", ".join(f"{elem_ll} {v}" for v in arr_vals[:arr_n])
                        vals.append(f"[{arr_n} x {elem_ll}] [{vals_str}]")
                else:
                    _, v_node = init.entries[flat_idx]
                    flat_idx += 1
                    vals.append(self._const_node_to_llvm(v_node, fty))
        return "{" + ", ".join(vals) + "}"

    def _const_node_to_llvm(self, node: Node, typ: str) -> str:
        """Convert a constant node to LLVM constant string for global initializers."""
        ll_ty = self.llvm_ty(typ)
        try:
            # For array field types, brace initializers (StructInit) are treated as array inits
            if typ.startswith("arr_field:") and isinstance(node, StructInit):
                parts = typ.split(":", 2)
                total_n = int(parts[1])
                elem_ty = parts[2]
                elem_ll = self.llvm_ty(elem_ty)
                vals = []
                for _, v_node in node.entries:
                    try:
                        vals.append(self.eval_global_const(v_node))
                    except Exception:
                        vals.append(0)
                while len(vals) < total_n:
                    vals.append(0)
                vals_str = ", ".join(f"{elem_ll} {v}" for v in vals[:total_n])
                return f"[{total_n} x {elem_ll}] [{vals_str}]"
            if isinstance(node, StructInit):
                return f"{ll_ty} {self._emit_struct_init_const(typ, node)}"
            if isinstance(node, ArrayInit):
                # Handle arr_field:N:elem or plain array types
                if typ.startswith("arr_field:"):
                    parts = typ.split(":", 2)
                    total_n = int(parts[1])
                    elem_ty = parts[2]
                    elem_ll = self.llvm_ty(elem_ty)
                    # Pad or truncate to total_n elements
                    vals = [self.eval_global_const(v) for v in node.values]
                    while len(vals) < total_n:
                        vals.append(0)
                    vals_str = ", ".join(f"{elem_ll} {v}" for v in vals[:total_n])
                    return f"[{total_n} x {elem_ll}] [{vals_str}]"
                elem_ll = "i32"  # default
                vals_str = ", ".join(f"{elem_ll} {v}" for v in node.values)
                return f"[{len(node.values)} x {elem_ll}] [{vals_str}]"
            # Handle address-of for pointer fields
            if isinstance(node, UnaryOp) and node.op == "&" and isinstance(node.expr, Var):
                return f"ptr @{node.expr.name}"
            v = self.eval_global_const(node)
            if ll_ty == "ptr":
                return "ptr null"
            return f"{ll_ty} {v}"
        except Exception:
            if ll_ty == "ptr":
                return "ptr null"
            if self.is_struct_type(typ):
                return f"{ll_ty} zeroinitializer"
            return f"{ll_ty} 0"

    def codegen_program(self) -> str:
        self.emit_declarations()
        if self.lines:
            self.emit("")
        for idx, fn in enumerate(self.prog.funcs):
            # Skip stdlib-internal functions (names starting with __) defined in
            # system headers; they use unsupported patterns and are never called directly.
            if fn.name.startswith("__"):
                # Emit a declare so calls to these functions are valid LLVM IR
                params = [p.typ for p in fn.params if p.typ != "..."]
                is_var = any(p.typ == "..." for p in fn.params)
                ret_ty = self.llvm_ty(fn.ret_type)
                parts = [self.llvm_ty(pt) for pt in params]
                if is_var:
                    parts.append("...")
                self.emit(f"declare {ret_ty} @{fn.name}({', '.join(parts)})")
                continue
            self.codegen_function(fn)
            if idx != len(self.prog.funcs) - 1:
                self.emit("")
        # Emit intrinsic declarations after functions (so we know which were used)
        intrinsic_decls = []
        bw_map = {"llvm.bswap.i16": "i16", "llvm.bswap.i32": "i32", "llvm.bswap.i64": "i64"}
        for intr in sorted(self.used_intrinsics):
            if intr in bw_map:
                bw = bw_map[intr]
                intrinsic_decls.append(f"declare {bw} @{intr}({bw})")
        # Prepend global string constants (preamble) before everything else
        if self.preamble_lines:
            all_lines = self.preamble_lines + [""] + self.lines
        else:
            all_lines = self.lines
        if intrinsic_decls:
            all_lines = intrinsic_decls + [""] + all_lines
        # Emit auto-declarations for called-but-undeclared functions
        if self.auto_declared_fns:
            auto_decl_lines = sorted(self.auto_declared_fns.values())
            all_lines = auto_decl_lines + [""] + all_lines
        return "\n".join(all_lines) + "\n"


def compile_source(src: str) -> str:
    tokens = Lexer(src).scan()
    ast = Parser(tokens).parse_program()
    SemanticAnalyzer().analyze_program(ast)
    return IRBuilder(ast).codegen_program()


def preprocess_source(src_path: str, clang_path: str) -> str:
    cmd = [clang_path, "-E", "-P", src_path]
    try:
        res = subprocess.run(cmd, text=True, capture_output=True)
    except OSError as e:
        raise CompileError(
            f"preprocess failed: cannot execute {clang_path!r}: {e}"
        ) from e
    if res.returncode != 0:
        detail = res.stderr.strip() or res.stdout.strip() or "(no diagnostic output)"
        raise CompileError(f"preprocess failed with {clang_path!r}:\n{detail}")
    return res.stdout


def main() -> int:
    ap = argparse.ArgumentParser(description="Tiny C-subset to LLVM IR compiler")
    ap.add_argument("input", help="Path to input .c file")
    ap.add_argument("-o", "--output", help="Path to output .ll file")
    ap.add_argument(
        "--no-preprocess",
        action="store_true",
        help="Disable external preprocessing and parse input source directly",
    )
    ap.add_argument(
        "--clang",
        default="",
        help="Path to clang used for preprocessing (default: auto-detect clang)",
    )
    args = ap.parse_args()

    try:
        if args.no_preprocess:
            with open(args.input, "r", encoding="utf-8") as f:
                src = f.read()
        else:
            clang_path = args.clang or shutil.which("clang")
            if clang_path is None:
                raise CompileError(
                    "preprocess requested but clang was not found; "
                    "pass --no-preprocess to disable it"
                )
            src = preprocess_source(args.input, clang_path)
    except OSError as e:
        print(f"compile error: failed to read input: {e}", file=sys.stderr)
        return 1

    try:
        ir = compile_source(src)
    except CompileError as e:
        print(f"compile error: {e}", file=sys.stderr)
        return 1

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(ir)
    else:
        sys.stdout.write(ir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
