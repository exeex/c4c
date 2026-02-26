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
        self.user_labels: Dict[str, str] = {}
        self.switch_case_stack: List[Dict[int, str]] = []
        self.switch_default_stack: List[Optional[str]] = []
        self.string_consts: Dict[str, str] = {}  # value -> global name
        self.string_idx = 0
        self.used_intrinsics: set = set()  # track LLVM intrinsics used
        self.enum_consts: Dict[str, int] = dict(prog.enum_consts) if prog.enum_consts else {}
        for d in prog.decls:
            params = [p.typ for p in d.params if p.typ != "..."]
            is_var = any(p.typ == "..." for p in d.params)
            sig = FunctionSig(d.ret_type, params, is_var)
            self.func_sigs[d.name] = sig
        for g in prog.globals:
            self.global_vars[g.name] = g.init
            if g.size is not None:
                self.global_arrays[g.name] = g.size
                if g.base_type == "char" and g.ptr_level == 0:
                    self.global_types[g.name] = "char_array"
                else:
                    self.global_types[g.name] = "array"
            elif g.ptr_level == 0 and g.base_type == "char" and isinstance(g.init, StringLit):
                # char name[] = "..." - infer array size from string
                str_len = len(g.init.value) + 1  # +1 for null terminator
                self.global_arrays[g.name] = str_len
                self.global_types[g.name] = "char_array"
            elif g.ptr_level > 0:
                self.global_types[g.name] = g.base_type + ("*" * g.ptr_level)
            else:
                self.global_types[g.name] = g.base_type
        for fn in prog.funcs:
            params = [p.typ for p in fn.params if p.typ != "..."]
            is_var = any(p.typ == "..." for p in fn.params)
            self.func_sigs[fn.name] = FunctionSig(fn.ret_type, params, is_var)

    def llvm_ty(self, ty: str) -> str:
        if ty == "void":
            return "void"
        if ty.startswith("struct:"):
            tag = ty.split(":", 1)[1]
            return f"%struct.{tag}"
        if ty.endswith("*") or ty in ("ptr", "char*"):
            return "ptr"
        if ty == "char":
            return "i8"
        if ty == "double":
            return "double"
        return "i32"

    def struct_field(self, struct_ty: str, field: str) -> tuple[int, str]:
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            raise CompileError(f"codegen error: unknown struct type {struct_ty!r}")
        for i, f in enumerate(s.fields):
            if f.name == field:
                return i, f.typ
        raise CompileError(f"codegen error: unknown field {field!r} in {struct_ty!r}")

    def is_struct_type(self, ty: str) -> bool:
        return ty.startswith("struct:")

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

    def promote_to_i32(self, v: str, ty: str) -> str:
        if ty == "char":
            t = self.tmp()
            self.emit(f"  {t} = sext i8 {v} to i32")
            return t
        if ty == "double":
            t = self.tmp()
            self.emit(f"  {t} = fptosi double {v} to i32")
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
        # don't convert ptr <-> non-ptr (would need inttoptr/ptrtoint - skip)
        if src_ll == "ptr" or dst_ll == "ptr":
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
            case IntLit():
                return "int"
            case StringLit():
                return "char*"
            case Var(name=name):
                if name in self.enum_consts:
                    return "int"
                vty = self.resolve_var_type(name)
                if vty == "array":
                    return "int*"
                return vty
            case Index():
                return "int"
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.expr_type(base)
                if through_ptr and bt.endswith("*"):
                    bt = bt[:-1]
                _, fty = self.struct_field(bt, field)
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
            case UnaryOp():
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
            case BinOp():
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
                sig = self.func_sigs.get(name)
                if sig is None:
                    return "int"
                ret = sig.ret_type
                if ret == "void":
                    return "void"
                return ret
            case _:
                return "int"

    def sizeof_type(self, ty: str) -> int:
        if ty == "char":
            return 1
        if ty in ("int", "float"):
            return 4
        if ty == "double":
            return 8
        if ty.endswith("*") or ty in ("ptr", "char*"):
            return 8
        if self.is_struct_type(ty):
            tag = ty.split(":", 1)[1]
            s = self.struct_defs.get(tag)
            if s is None:
                return 8  # unknown struct default
            return sum(self.sizeof_type(f.typ) for f in s.fields)
        return 4  # default

    def sizeof_expr(self, n: Node) -> int:
        if isinstance(n, Var):
            name = n.name
            if name in self.local_arrays:
                return self.local_arrays[name] * 4
            if name in self.global_arrays:
                return self.global_arrays[name] * 4
        return self.sizeof_type(self.expr_type(n))

    def codegen_lvalue_ptr(self, n: Node) -> str:
        match n:
            case Var(name=name):
                if self.resolve_var_type(name) == "array":
                    raise CompileError("codegen error: cannot assign to array object")
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
                match base:
                    case Var(name=name):
                        t = self.tmp()
                        if name in self.local_arrays:
                            n_elem = self.local_arrays[name]
                            self.emit(
                                f"  {t} = getelementptr inbounds [{n_elem} x i32], ptr %{name}, i32 0, i32 {idx}"
                            )
                            return t
                        if name in self.global_arrays:
                            n_elem = self.global_arrays[name]
                            self.emit(
                                f"  {t} = getelementptr inbounds [{n_elem} x i32], ptr @{name}, i32 0, i32 {idx}"
                            )
                            return t
                        if self.resolve_var_type(name).endswith("*"):
                            base_ptr = self.codegen_expr(base)
                            if base_ptr is None:
                                raise CompileError("codegen error: invalid pointer base")
                            self.emit(f"  {t} = getelementptr inbounds i32, ptr {base_ptr}, i32 {idx}")
                            return t
                        raise CompileError("codegen error: index base must be array variable")
                    case _:
                        raise CompileError("codegen error: index base must be a variable")
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.expr_type(base)
                if through_ptr:
                    if not bt.endswith("*"):
                        raise CompileError("codegen error: '->' requires pointer base")
                    struct_ty = bt[:-1]
                    struct_ptr = self.codegen_expr(base)
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
                vty = self.resolve_var_type(name)
                if vty == "array":
                    # Return pointer to first element
                    slot = self.resolve_var_ptr(name)
                    sz = self.local_arrays.get(name) or self.global_arrays.get(name, 1)
                    t = self.tmp()
                    self.emit(f"  {t} = getelementptr inbounds [{sz} x i32], ptr {slot}, i32 0, i32 0")
                    return t
                slot = self.resolve_var_ptr(name)
                t = self.tmp()
                self.emit(f"  {t} = load {self.llvm_ty(vty)}, ptr {slot}")
                return t
            case Index():
                ptr = self.codegen_lvalue_ptr(n)
                t = self.tmp()
                self.emit(f"  {t} = load i32, ptr {ptr}")
                return t
            case Member():
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
                # Check if double arithmetic is needed
                is_double = lty == "double" or rty == "double"
                if is_double and op in {"+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">="}:
                    ld = self.emit_convert(l, lty, "double")
                    rd = self.emit_convert(r, rty, "double")
                    if op in {"+", "-", "*", "/"}:
                        dop = {"+": "fadd", "-": "fsub", "*": "fmul", "/": "fdiv"}[op]
                        self.emit(f"  {t} = {dop} double {ld}, {rd}")
                        return t
                    fcmp_map = {"==": "oeq", "!=": "one", "<": "olt", "<=": "ole", ">": "ogt", ">=": "oge"}
                    b = self.tmp()
                    self.emit(f"  {b} = fcmp {fcmp_map[op]} double {ld}, {rd}")
                    self.emit(f"  {t} = zext i1 {b} to i32")
                    return t
                l = self.promote_to_i32(l, lty)
                r = self.promote_to_i32(r, rty)
                if op in {"+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>"}:
                    if op in {"+", "-"} and (lty.endswith("*") or rty.endswith("*")):
                        if op == "+" and lty.endswith("*") and rty == "int":
                            self.emit(f"  {t} = getelementptr inbounds i32, ptr {l}, i32 {r}")
                            return t
                        if op == "+" and lty == "int" and rty.endswith("*"):
                            self.emit(f"  {t} = getelementptr inbounds i32, ptr {r}, i32 {l}")
                            return t
                        if op == "-" and lty.endswith("*") and rty == "int":
                            neg = self.tmp()
                            self.emit(f"  {neg} = sub i32 0, {r}")
                            self.emit(f"  {t} = getelementptr inbounds i32, ptr {l}, i32 {neg}")
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
                            self.emit(f"  {s} = sdiv i32 {t}, 4")
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
                    return self.codegen_lvalue_ptr(expr)
                if op == "*":
                    pv = self.codegen_expr(expr)
                    if pv is None:
                        raise CompileError("codegen error: invalid pointer dereference")
                    et = self.expr_type(expr)
                    load_ty = "i32"
                    if et.endswith("*"):
                        load_ty = self.llvm_ty(et[:-1])
                    t = self.tmp()
                    self.emit(f"  {t} = load {load_ty}, ptr {pv}")
                    return t
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: void value used in unary operation")
                if op == "+":
                    return v
                t = self.tmp()
                if op == "-":
                    self.emit(f"  {t} = sub i32 0, {v}")
                    return t
                if op == "!":
                    b = self.tmp()
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
                self.emit_label(then_lbl)
                tv = self.codegen_expr(then_expr)
                if tv is None:
                    raise CompileError("codegen error: void value in ternary")
                self.emit(f"  br label %{end_lbl}")
                self.emit_label(else_lbl)
                ev = self.codegen_expr(else_expr)
                if ev is None:
                    raise CompileError("codegen error: void value in ternary")
                self.emit(f"  br label %{end_lbl}")
                self.emit_label(end_lbl)
                rty = self.expr_type(n)
                t = self.tmp()
                ll_rty = self.llvm_ty(rty)
                # For pointer phi, replace 0 with null
                if ll_rty == "ptr":
                    tv = "null" if tv == "0" else tv
                    ev = "null" if ev == "0" else ev
                self.emit(
                    f"  {t} = phi {ll_rty} [{tv}, %{then_lbl}], [{ev}, %{else_lbl}]"
                )
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
                    self.emit(f"  {t} = getelementptr inbounds i32, ptr {curp}, i32 {step}")
                    self.emit(f"  store ptr {t}, ptr {slot}")
                    return t
                cur = self.tmp()
                self.emit(f"  {cur} = load i32, ptr {slot}")
                t = self.tmp()
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
                    self.emit(f"  {nxt} = getelementptr inbounds i32, ptr {cur}, i32 {step}")
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
                        # variadic extra arg or undeclared func - infer type
                        arg_ty = self.llvm_ty(aty)
                    args_text.append(f"{arg_ty} {v}")
                if sig is None:
                    # undeclared function - assume returns int
                    t = self.tmp()
                    self.emit(f"  {t} = call i32 @{name}({', '.join(args_text)})")
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
            case _:
                raise CompileError(f"codegen error: unsupported expr {type(n).__name__}")

    def codegen_stmt(self, n: Node, fn_ret_type: str) -> bool:
        match n:
            case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, init=init):
                slot = f"%{name}"
                already_hoisted = name in self.slots
                if not already_hoisted:
                    self.slots[name] = slot
                if size is not None:
                    if not already_hoisted:
                        self.local_arrays[name] = size
                        self.local_types[name] = "array"
                        self.emit(f"  {slot} = alloca [{size} x i32]")
                    return False
                if not already_hoisted:
                    self.local_types[name] = base_type + ("*" * ptr_level)
                    self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[name])}")
                if init is not None:
                    v = self.codegen_expr(init)
                    if v is None:
                        raise CompileError("codegen error: cannot initialize variable with void")
                    v = self.emit_convert(v, self.expr_type(init), self.local_types[name])
                    self.emit(f"  store {self.llvm_ty(self.local_types[name])} {v}, ptr {slot}")
                return False
            case Block(body=body):
                terminated = False
                for st in body:
                    if terminated:
                        if isinstance(st, (LabelStmt, Case, Default)):
                            terminated = self.codegen_stmt(st, fn_ret_type)
                        elif isinstance(st, Block):
                            # Recurse into blocks even when terminated - they may contain labels
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
            field_tys = ", ".join(self.llvm_ty(f.typ) for f in s.fields)
            self.emit(f"%struct.{tag} = type {{{field_tys}}}")
        for tag in sorted(referenced_tags - defined_tags):
            self.emit(f"%struct.{tag} = type opaque")
        if self.struct_defs:
            self.emit("")

        for name, init in self.global_vars.items():
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
                    else:
                        self.emit(f"@{name} = global [{n_elem} x i8] zeroinitializer")
                else:
                    self.emit(f"@{name} = global [{n_elem} x i32] zeroinitializer")
                continue
            gty = self.resolve_var_type(name)
            if gty.endswith("*"):
                self.emit(f"@{name} = global ptr null")
                continue
            if self.is_struct_type(gty):
                self.emit(f"@{name} = global {self.llvm_ty(gty)} zeroinitializer")
                continue
            if gty == "double":
                init_val = 0 if init is None else self.eval_global_const(init)
                # LLVM requires double constants in float notation
                self.emit(f"@{name} = global double {float(init_val)}")
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
                elif isinstance(st, Block):
                    terminated = self.codegen_stmt(st, fn.ret_type)
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
                case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size):
                    if name in self.slots:
                        continue  # already hoisted
                    slot = f"%{name}"
                    self.slots[name] = slot
                    if size is not None:
                        self.local_arrays[name] = size
                        self.local_types[name] = "array"
                        self.emit(f"  {slot} = alloca [{size} x i32]")
                    else:
                        self.local_types[name] = base_type + ("*" * ptr_level)
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

    def codegen_program(self) -> str:
        self.emit_declarations()
        if self.lines:
            self.emit("")
        for idx, fn in enumerate(self.prog.funcs):
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
