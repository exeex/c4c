#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, List, Optional


class CompileError(Exception):
    pass


class TokenType(Enum):
    KW_INT = auto()
    KW_VOID = auto()
    KW_EXTERN = auto()
    KW_RETURN = auto()
    KW_IF = auto()
    KW_ELSE = auto()
    KW_WHILE = auto()
    KW_FOR = auto()
    KW_DO = auto()
    KW_BREAK = auto()
    KW_CONTINUE = auto()
    ID = auto()
    NUM = auto()
    LBRACE = auto()
    RBRACE = auto()
    LPAREN = auto()
    RPAREN = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    SEMI = auto()
    COMMA = auto()
    DOT = auto()
    COLON = auto()
    QUESTION = auto()
    ASSIGN = auto()
    EQ = auto()
    NE = auto()
    LT = auto()
    LE = auto()
    GT = auto()
    GE = auto()
    PLUS = auto()
    PLUSEQ = auto()
    PLUSPLUS = auto()
    MINUS = auto()
    MINUSEQ = auto()
    MINUSMINUS = auto()
    ARROW = auto()
    STAR = auto()
    STAREQ = auto()
    SLASH = auto()
    SLASHEQ = auto()
    PERCENT = auto()
    PERCENTEQ = auto()
    AMP = auto()
    AMPAMP = auto()
    PIPE = auto()
    PIPEPIPE = auto()
    CARET = auto()
    BANG = auto()
    EOF = auto()


@dataclass(frozen=True)
class Token:
    typ: TokenType
    text: str
    line: int
    col: int


SINGLE_CHAR_TOKENS = {
    "{": TokenType.LBRACE,
    "}": TokenType.RBRACE,
    "(": TokenType.LPAREN,
    ")": TokenType.RPAREN,
    "[": TokenType.LBRACKET,
    "]": TokenType.RBRACKET,
    ";": TokenType.SEMI,
    ",": TokenType.COMMA,
    ".": TokenType.DOT,
    ":": TokenType.COLON,
    "?": TokenType.QUESTION,
    "=": TokenType.ASSIGN,
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
    "*": TokenType.STAR,
    "/": TokenType.SLASH,
    "%": TokenType.PERCENT,
    "&": TokenType.AMP,
    "|": TokenType.PIPE,
    "^": TokenType.CARET,
    "!": TokenType.BANG,
    "<": TokenType.LT,
    ">": TokenType.GT,
}


MULTI_CHAR_TOKENS = {
    "==": TokenType.EQ,
    "!=": TokenType.NE,
    "<=": TokenType.LE,
    ">=": TokenType.GE,
    "+=": TokenType.PLUSEQ,
    "-=": TokenType.MINUSEQ,
    "*=": TokenType.STAREQ,
    "/=": TokenType.SLASHEQ,
    "%=": TokenType.PERCENTEQ,
    "++": TokenType.PLUSPLUS,
    "--": TokenType.MINUSMINUS,
    "&&": TokenType.AMPAMP,
    "||": TokenType.PIPEPIPE,
    "->": TokenType.ARROW,
}


class Lexer:
    def __init__(self, src: str):
        self.src = src
        self.i = 0
        self.line = 1
        self.col = 1

    def cur(self) -> str:
        if self.i >= len(self.src):
            return ""
        return self.src[self.i]

    def peek(self, n: int = 1) -> str:
        j = self.i + n
        if j >= len(self.src):
            return ""
        return self.src[j]

    def advance(self) -> str:
        ch = self.cur()
        if not ch:
            return ""
        self.i += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def scan_identifier(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = [self.advance()]
        while True:
            ch = self.cur()
            if ch.isalnum() or ch == "_":
                buf.append(self.advance())
            else:
                break
        text = "".join(buf)
        kw_map = {
            "int": TokenType.KW_INT,
            "void": TokenType.KW_VOID,
            "extern": TokenType.KW_EXTERN,
            "return": TokenType.KW_RETURN,
            "if": TokenType.KW_IF,
            "else": TokenType.KW_ELSE,
            "while": TokenType.KW_WHILE,
            "for": TokenType.KW_FOR,
            "do": TokenType.KW_DO,
            "break": TokenType.KW_BREAK,
            "continue": TokenType.KW_CONTINUE,
        }
        return Token(kw_map.get(text, TokenType.ID), text, start_line, start_col)

    def scan_number(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = [self.advance()]
        while self.cur().isdigit():
            buf.append(self.advance())
        return Token(TokenType.NUM, "".join(buf), start_line, start_col)

    def scan(self) -> List[Token]:
        tokens: List[Token] = []
        while True:
            ch = self.cur()
            if not ch:
                break

            if ch.isspace():
                self.advance()
                continue

            if ch == "/" and self.peek() == "/":
                while self.cur() and self.cur() != "\n":
                    self.advance()
                continue

            if ch == "/" and self.peek() == "*":
                start_line, start_col = self.line, self.col
                self.advance()
                self.advance()
                while True:
                    if not self.cur():
                        raise CompileError(
                            f"{start_line}:{start_col}: unterminated block comment"
                        )
                    if self.cur() == "*" and self.peek() == "/":
                        self.advance()
                        self.advance()
                        break
                    self.advance()
                continue

            if ch.isalpha() or ch == "_":
                tokens.append(self.scan_identifier())
                continue

            if ch.isdigit():
                tokens.append(self.scan_number())
                continue

            two = ch + self.peek()
            tok_type = MULTI_CHAR_TOKENS.get(two)
            if tok_type is not None:
                tokens.append(Token(tok_type, two, self.line, self.col))
                self.advance()
                self.advance()
                continue

            tok_type = SINGLE_CHAR_TOKENS.get(ch)
            if tok_type is not None:
                tokens.append(Token(tok_type, ch, self.line, self.col))
                self.advance()
                continue

            raise CompileError(f"{self.line}:{self.col}: unexpected character {ch!r}")

        tokens.append(Token(TokenType.EOF, "", self.line, self.col))
        return tokens


class Node:
    pass


@dataclass
class Param(Node):
    typ: str
    name: Optional[str]


@dataclass
class FunctionDecl(Node):
    ret_type: str
    name: str
    params: List[Param]


@dataclass
class Function(Node):
    ret_type: str
    name: str
    params: List[Param]
    body: List[Node]


@dataclass
class Program(Node):
    decls: List[FunctionDecl]
    globals: List["GlobalVar"]
    funcs: List[Function]


@dataclass
class Decl(Node):
    name: str
    init: Optional[Node]


@dataclass
class Assign(Node):
    name: str
    expr: Node


@dataclass
class GlobalVar(Node):
    name: str
    init: Optional[Node]


@dataclass
class AssignExpr(Node):
    name: str
    op: str
    expr: Node


@dataclass
class ExprStmt(Node):
    expr: Node


@dataclass
class Return(Node):
    expr: Optional[Node]


@dataclass
class IntLit(Node):
    value: int


@dataclass
class Var(Node):
    name: str


@dataclass
class Call(Node):
    name: str
    args: List[Node]


@dataclass
class BinOp(Node):
    op: str
    lhs: Node
    rhs: Node


@dataclass
class UnaryOp(Node):
    op: str
    expr: Node


@dataclass
class IncDec(Node):
    name: str
    op: str
    prefix: bool


@dataclass
class Block(Node):
    body: List[Node]


@dataclass
class If(Node):
    cond: Node
    then_stmt: Node
    else_stmt: Optional[Node]


@dataclass
class While(Node):
    cond: Node
    body: Node


@dataclass
class For(Node):
    init: Optional[Node]
    cond: Optional[Node]
    post: Optional[Node]
    body: Node


@dataclass
class DoWhile(Node):
    body: Node
    cond: Node


@dataclass
class Break(Node):
    pass


@dataclass
class Continue(Node):
    pass


@dataclass
class EmptyStmt(Node):
    pass


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.i = 0

    def cur(self) -> Token:
        return self.tokens[self.i]

    def peek(self, n: int = 1) -> Token:
        j = self.i + n
        if j >= len(self.tokens):
            return self.tokens[-1]
        return self.tokens[j]

    def advance(self) -> None:
        self.i += 1

    def error(self, msg: str) -> CompileError:
        t = self.cur()
        return CompileError(f"{t.line}:{t.col}: {msg} (got {t.text!r})")

    def eat(self, typ: TokenType) -> Token:
        t = self.cur()
        if t.typ != typ:
            raise self.error(f"expected {typ.name}")
        self.advance()
        return t

    def parse_type(self) -> str:
        match self.cur().typ:
            case TokenType.KW_INT:
                self.advance()
                return "int"
            case TokenType.KW_VOID:
                self.advance()
                return "void"
            case _:
                raise self.error("expected type")

    def parse_params(self) -> List[Param]:
        if self.cur().typ == TokenType.RPAREN:
            return []

        if self.cur().typ == TokenType.KW_VOID and self.peek().typ == TokenType.RPAREN:
            self.advance()
            return []

        params: List[Param] = []
        while True:
            typ = self.parse_type()
            name: Optional[str] = None
            if self.cur().typ == TokenType.ID:
                name = self.eat(TokenType.ID).text
            params.append(Param(typ, name))
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return params

    def parse_program(self) -> Program:
        decls: List[FunctionDecl] = []
        globals_: List[GlobalVar] = []
        funcs: List[Function] = []
        while self.cur().typ != TokenType.EOF:
            ext = self.parse_external()
            if isinstance(ext, FunctionDecl):
                decls.append(ext)
            elif isinstance(ext, list):
                globals_.extend(ext)
            else:
                funcs.append(ext)
        self.eat(TokenType.EOF)
        return Program(decls, globals_, funcs)

    def parse_external(self) -> FunctionDecl | list[GlobalVar] | Function:
        if self.cur().typ == TokenType.KW_EXTERN:
            self.advance()
        ret_type = self.parse_type()
        while self.cur().typ == TokenType.STAR:
            self.advance()
        name = self.eat(TokenType.ID).text

        if self.cur().typ != TokenType.LPAREN:
            if ret_type == "void":
                raise self.error("void object type is not supported")
            decls: List[GlobalVar] = []
            init: Optional[Node] = None
            if self.cur().typ == TokenType.ASSIGN:
                self.advance()
                init = self.parse_expr()
            decls.append(GlobalVar(name, init))
            while self.cur().typ == TokenType.COMMA:
                self.advance()
                while self.cur().typ == TokenType.STAR:
                    self.advance()
                gname = self.eat(TokenType.ID).text
                ginit: Optional[Node] = None
                if self.cur().typ == TokenType.ASSIGN:
                    self.advance()
                    ginit = self.parse_expr()
                decls.append(GlobalVar(gname, ginit))
            self.eat(TokenType.SEMI)
            return decls

        self.eat(TokenType.LPAREN)
        params = self.parse_params()
        self.eat(TokenType.RPAREN)

        if self.cur().typ == TokenType.SEMI:
            self.advance()
            return FunctionDecl(ret_type, name, params)

        self.eat(TokenType.LBRACE)
        body: List[Node] = []
        while self.cur().typ != TokenType.RBRACE:
            body.append(self.parse_stmt())
        self.eat(TokenType.RBRACE)
        return Function(ret_type, name, params, body)

    def parse_stmt(self) -> Node:
        match self.cur().typ:
            case TokenType.KW_INT:
                self.advance()
                decls: List[Node] = []
                while True:
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                    name = self.eat(TokenType.ID).text
                    init: Optional[Node] = None
                    if self.cur().typ == TokenType.ASSIGN:
                        self.advance()
                        init = self.parse_expr()
                    decls.append(Decl(name, init))
                    if self.cur().typ != TokenType.COMMA:
                        break
                    self.advance()
                self.eat(TokenType.SEMI)
                if len(decls) == 1:
                    return decls[0]
                return Block(decls)
            case TokenType.LBRACE:
                self.advance()
                body: List[Node] = []
                while self.cur().typ != TokenType.RBRACE:
                    body.append(self.parse_stmt())
                self.eat(TokenType.RBRACE)
                return Block(body)
            case TokenType.KW_IF:
                self.advance()
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                then_stmt = self.parse_stmt()
                else_stmt: Optional[Node] = None
                if self.cur().typ == TokenType.KW_ELSE:
                    self.advance()
                    else_stmt = self.parse_stmt()
                return If(cond, then_stmt, else_stmt)
            case TokenType.KW_WHILE:
                self.advance()
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                body = self.parse_stmt()
                return While(cond, body)
            case TokenType.KW_FOR:
                self.advance()
                self.eat(TokenType.LPAREN)
                init: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    init = self.parse_expr()
                self.eat(TokenType.SEMI)
                cond: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    cond = self.parse_expr()
                self.eat(TokenType.SEMI)
                post: Optional[Node] = None
                if self.cur().typ != TokenType.RPAREN:
                    post = self.parse_expr()
                self.eat(TokenType.RPAREN)
                body = self.parse_stmt()
                return For(init, cond, post, body)
            case TokenType.KW_DO:
                self.advance()
                body = self.parse_stmt()
                self.eat(TokenType.KW_WHILE)
                self.eat(TokenType.LPAREN)
                cond = self.parse_expr()
                self.eat(TokenType.RPAREN)
                self.eat(TokenType.SEMI)
                return DoWhile(body, cond)
            case TokenType.KW_BREAK:
                self.advance()
                self.eat(TokenType.SEMI)
                return Break()
            case TokenType.KW_CONTINUE:
                self.advance()
                self.eat(TokenType.SEMI)
                return Continue()
            case TokenType.SEMI:
                self.advance()
                return EmptyStmt()
            case TokenType.KW_RETURN:
                self.advance()
                expr: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Return(expr)
            case TokenType.ID if self.peek().typ == TokenType.ASSIGN:
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.ASSIGN)
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Assign(name, expr)
            case _:
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return ExprStmt(expr)

    def parse_expr(self) -> Node:
        return self.parse_assignment()

    def parse_assignment(self) -> Node:
        lhs = self.parse_logical_or()
        assign_ops = {
            TokenType.ASSIGN: "=",
            TokenType.PLUSEQ: "+=",
            TokenType.MINUSEQ: "-=",
            TokenType.STAREQ: "*=",
            TokenType.SLASHEQ: "/=",
            TokenType.PERCENTEQ: "%=",
        }
        op = assign_ops.get(self.cur().typ)
        if op is None:
            return lhs
        self.advance()
        rhs = self.parse_assignment()
        if not isinstance(lhs, Var):
            raise self.error("left-hand side of assignment must be a variable")
        return AssignExpr(lhs.name, op, rhs)

    def parse_logical_or(self) -> Node:
        node = self.parse_logical_and()
        while self.cur().typ == TokenType.PIPEPIPE:
            self.advance()
            node = BinOp("||", node, self.parse_logical_and())
        return node

    def parse_logical_and(self) -> Node:
        node = self.parse_bit_or()
        while self.cur().typ == TokenType.AMPAMP:
            self.advance()
            node = BinOp("&&", node, self.parse_bit_or())
        return node

    def parse_bit_or(self) -> Node:
        node = self.parse_bit_xor()
        while self.cur().typ == TokenType.PIPE:
            self.advance()
            node = BinOp("|", node, self.parse_bit_xor())
        return node

    def parse_bit_xor(self) -> Node:
        node = self.parse_bit_and()
        while self.cur().typ == TokenType.CARET:
            self.advance()
            node = BinOp("^", node, self.parse_bit_and())
        return node

    def parse_bit_and(self) -> Node:
        node = self.parse_equality()
        while self.cur().typ == TokenType.AMP:
            self.advance()
            node = BinOp("&", node, self.parse_equality())
        return node

    def parse_equality(self) -> Node:
        node = self.parse_relational()
        while self.cur().typ in (TokenType.EQ, TokenType.NE):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_relational())
        return node

    def parse_relational(self) -> Node:
        node = self.parse_add()
        while self.cur().typ in (TokenType.LT, TokenType.LE, TokenType.GT, TokenType.GE):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_add())
        return node

    def parse_add(self) -> Node:
        node = self.parse_term()
        while self.cur().typ in (TokenType.PLUS, TokenType.MINUS):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_term())
        return node

    def parse_term(self) -> Node:
        node = self.parse_unary()
        while self.cur().typ in (TokenType.STAR, TokenType.SLASH, TokenType.PERCENT):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_unary())
        return node

    def parse_unary(self) -> Node:
        if self.cur().typ in (TokenType.PLUS, TokenType.MINUS, TokenType.BANG, TokenType.STAR):
            op = self.cur().text
            self.advance()
            return UnaryOp(op, self.parse_unary())
        if self.cur().typ == TokenType.AMP:
            self.advance()
            return UnaryOp("&", self.parse_unary())
        if self.cur().typ in (TokenType.PLUSPLUS, TokenType.MINUSMINUS):
            op = self.cur().text
            self.advance()
            node = self.parse_unary()
            if not isinstance(node, Var):
                raise self.error("increment/decrement target must be a variable")
            return IncDec(node.name, op, True)
        return self.parse_postfix()

    def parse_postfix(self) -> Node:
        node = self.parse_primary()
        while self.cur().typ in (TokenType.PLUSPLUS, TokenType.MINUSMINUS):
            if not isinstance(node, Var):
                raise self.error("increment/decrement target must be a variable")
            op = self.cur().text
            self.advance()
            node = IncDec(node.name, op, False)
        return node

    def parse_args(self) -> List[Node]:
        if self.cur().typ == TokenType.RPAREN:
            return []
        args: List[Node] = []
        while True:
            args.append(self.parse_expr())
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return args

    def parse_primary(self) -> Node:
        match self.cur().typ:
            case TokenType.NUM:
                return IntLit(int(self.eat(TokenType.NUM).text))
            case TokenType.ID:
                name = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LPAREN:
                    self.advance()
                    args = self.parse_args()
                    self.eat(TokenType.RPAREN)
                    return Call(name, args)
                return Var(name)
            case TokenType.LPAREN:
                self.advance()
                n = self.parse_expr()
                self.eat(TokenType.RPAREN)
                return n
            case _:
                raise self.error("expected expression")


@dataclass
class FunctionSig:
    ret_type: str
    param_types: List[str]


class SemanticAnalyzer:
    def __init__(self):
        self.func_sigs: Dict[str, FunctionSig] = {}
        self.global_vars: Dict[str, bool] = {}

    def ensure_sig(self, name: str, sig: FunctionSig) -> None:
        prev = self.func_sigs.get(name)
        if prev is None:
            self.func_sigs[name] = sig
            return
        if prev != sig:
            raise CompileError(f"semantic error: conflicting declaration for function {name!r}")

    def analyze_program(self, p: Program) -> None:
        for g in p.globals:
            if g.name in self.global_vars:
                raise CompileError(f"semantic error: duplicate global variable {g.name!r}")
            self.global_vars[g.name] = True
        for d in p.decls:
            self.ensure_sig(d.name, FunctionSig(d.ret_type, [x.typ for x in d.params]))
        for fn in p.funcs:
            self.ensure_sig(fn.name, FunctionSig(fn.ret_type, [x.typ for x in fn.params]))

        for g in p.globals:
            if g.name in self.func_sigs:
                raise CompileError(f"semantic error: name conflict between global and function {g.name!r}")
            if g.init is not None:
                self.analyze_const_expr(g.init)

        seen_defs: Dict[str, bool] = {}
        for fn in p.funcs:
            if fn.name in seen_defs:
                raise CompileError(f"semantic error: duplicate function definition {fn.name!r}")
            seen_defs[fn.name] = True
            self.analyze_function(fn)

    def analyze_function(self, fn: Function) -> None:
        vars_init: Dict[str, bool] = {}
        for param in fn.params:
            if param.name is None:
                raise CompileError(
                    f"semantic error: parameter name required in function definition {fn.name!r}"
                )
            if param.name in vars_init:
                raise CompileError(
                    f"semantic error: duplicate parameter name {param.name!r} in {fn.name!r}"
                )
            vars_init[param.name] = True

        saw_return = False
        for st in fn.body:
            saw_return = self.analyze_stmt(st, vars_init, fn.ret_type, 0) or saw_return
            if saw_return:
                break

    def analyze_stmt(
        self,
        n: Node,
        vars_init: Dict[str, bool],
        fn_ret_type: str,
        loop_depth: int,
    ) -> bool:
        match n:
            case Decl(name=name, init=init):
                if name in vars_init:
                    raise CompileError(f"semantic error: variable redeclared: {name!r}")
                vars_init[name] = False
                if init is not None:
                    self.analyze_expr(init, vars_init)
                    vars_init[name] = True
                return False
            case Block(body=body):
                for st in body:
                    if self.analyze_stmt(st, vars_init, fn_ret_type, loop_depth):
                        return True
                return False
            case If(cond=cond, then_stmt=then_stmt, else_stmt=else_stmt):
                self.analyze_expr(cond, vars_init)
                then_state = dict(vars_init)
                then_ret = self.analyze_stmt(then_stmt, then_state, fn_ret_type, loop_depth)
                if else_stmt is None:
                    for k in vars_init:
                        vars_init[k] = vars_init[k] and then_state.get(k, False)
                    return False
                else_state = dict(vars_init)
                else_ret = self.analyze_stmt(else_stmt, else_state, fn_ret_type, loop_depth)
                for k in vars_init:
                    vars_init[k] = then_state.get(k, False) and else_state.get(k, False)
                return then_ret and else_ret
            case While(cond=cond, body=body):
                self.analyze_expr(cond, vars_init)
                body_state = dict(vars_init)
                self.analyze_stmt(body, body_state, fn_ret_type, loop_depth + 1)
                return False
            case For(init=init, cond=cond, post=post, body=body):
                if init is not None:
                    self.analyze_expr(init, vars_init)
                if cond is not None:
                    self.analyze_expr(cond, vars_init)
                body_state = dict(vars_init)
                self.analyze_stmt(body, body_state, fn_ret_type, loop_depth + 1)
                if post is not None:
                    self.analyze_expr(post, body_state)
                return False
            case DoWhile(body=body, cond=cond):
                body_state = dict(vars_init)
                self.analyze_stmt(body, body_state, fn_ret_type, loop_depth + 1)
                self.analyze_expr(cond, body_state)
                return False
            case Break():
                if loop_depth <= 0:
                    raise CompileError("semantic error: break used outside loop")
                return False
            case Continue():
                if loop_depth <= 0:
                    raise CompileError("semantic error: continue used outside loop")
                return False
            case EmptyStmt():
                return False
            case Assign(name=name, expr=expr):
                if name not in vars_init and name not in self.global_vars:
                    raise CompileError(
                        f"semantic error: assignment to undeclared variable {name!r}"
                    )
                self.analyze_expr(expr, vars_init)
                if name in vars_init:
                    vars_init[name] = True
                return False
            case ExprStmt(expr=expr):
                self.analyze_expr(expr, vars_init)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    if expr is not None:
                        raise CompileError("semantic error: void function cannot return a value")
                else:
                    if expr is None:
                        raise CompileError("semantic error: int function must return a value")
                    self.analyze_expr(expr, vars_init)
                return True
            case _:
                raise CompileError(
                    f"semantic error: unsupported stmt node {type(n).__name__}"
                )

    def analyze_expr(self, n: Node, vars_init: Dict[str, bool]) -> str:
        match n:
            case IntLit():
                return "int"
            case Var(name=name):
                if name not in vars_init:
                    if name in self.global_vars:
                        return "int"
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                if not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                return "int"
            case BinOp(lhs=lhs, rhs=rhs):
                lt = self.analyze_expr(lhs, vars_init)
                rt = self.analyze_expr(rhs, vars_init)
                if lt != "int" or rt != "int":
                    raise CompileError("semantic error: binary op currently supports int only")
                return "int"
            case UnaryOp(op=op, expr=expr):
                et = self.analyze_expr(expr, vars_init)
                if et != "int":
                    raise CompileError(
                        f"semantic error: unary op {op!r} currently supports int only"
                    )
                if op in ("&", "*"):
                    raise CompileError(
                        f"semantic error: unary op {op!r} is not supported yet"
                    )
                return "int"
            case IncDec(name=name):
                if name not in vars_init and name not in self.global_vars:
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                if name in vars_init and not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                if name in vars_init:
                    vars_init[name] = True
                return "int"
            case AssignExpr(name=name, op=op, expr=expr):
                if name not in vars_init and name not in self.global_vars:
                    raise CompileError(
                        f"semantic error: assignment to undeclared variable {name!r}"
                    )
                et = self.analyze_expr(expr, vars_init)
                if et != "int":
                    raise CompileError("semantic error: assignment supports int only")
                if op != "=" and name in vars_init and not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                if name in vars_init:
                    vars_init[name] = True
                return "int"
            case Call(name=name, args=args):
                sig = self.func_sigs.get(name)
                if sig is None:
                    raise CompileError(f"semantic error: call to undeclared function {name!r}")
                if len(args) != len(sig.param_types):
                    raise CompileError(
                        f"semantic error: function {name!r} expects {len(sig.param_types)} args, got {len(args)}"
                    )
                for i, arg in enumerate(args):
                    at = self.analyze_expr(arg, vars_init)
                    if at != sig.param_types[i]:
                        raise CompileError(
                            f"semantic error: argument type mismatch for {name!r} at index {i}"
                        )
                return sig.ret_type
            case _:
                raise CompileError(
                    f"semantic error: unsupported expr node {type(n).__name__}"
                )

    def analyze_const_expr(self, n: Node) -> int:
        match n:
            case IntLit(value=value):
                return value
            case UnaryOp(op=op, expr=expr):
                v = self.analyze_const_expr(expr)
                if op == "+":
                    return v
                if op == "-":
                    return -v
                if op == "!":
                    return 0 if v else 1
                raise CompileError(f"semantic error: unsupported global initializer op {op!r}")
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.analyze_const_expr(lhs)
                r = self.analyze_const_expr(rhs)
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
                raise CompileError(f"semantic error: unsupported global initializer op {op!r}")
            case _:
                raise CompileError(
                    "semantic error: global initializer must be an integer constant expression"
                )


class IRBuilder:
    def __init__(self, prog: Program):
        self.prog = prog
        self.tmp_idx = 0
        self.label_idx = 0
        self.lines: List[str] = []
        self.slots: Dict[str, str] = {}
        self.loop_stack: List[tuple[str, str]] = []
        self.func_sigs: Dict[str, FunctionSig] = {}
        self.global_vars: Dict[str, Optional[Node]] = {}
        for d in prog.decls:
            self.func_sigs[d.name] = FunctionSig(d.ret_type, [p.typ for p in d.params])
        for g in prog.globals:
            self.global_vars[g.name] = g.init
        for fn in prog.funcs:
            self.func_sigs[fn.name] = FunctionSig(fn.ret_type, [p.typ for p in fn.params])

    def llvm_ty(self, ty: str) -> str:
        return "void" if ty == "void" else "i32"

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

    def resolve_var_ptr(self, name: str) -> str:
        if name in self.slots:
            return self.slots[name]
        if name in self.global_vars:
            return f"@{name}"
        raise CompileError(f"codegen error: unknown variable {name!r}")

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

    def codegen_expr(self, n: Node) -> Optional[str]:
        match n:
            case IntLit(value=value):
                return str(value)
            case Var(name=name):
                slot = self.resolve_var_ptr(name)
                t = self.tmp()
                self.emit(f"  {t} = load i32, ptr {slot}")
                return t
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.codegen_expr(lhs)
                if l is None:
                    raise CompileError("codegen error: void value used in binary operation")
                t = self.tmp()
                if op in {"&&", "||"}:
                    lb = self.tmp()
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
                    rb = self.tmp()
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
                if op in {"+", "-", "*", "/", "%", "&", "|", "^"}:
                    op_map = {
                        "+": "add",
                        "-": "sub",
                        "*": "mul",
                        "/": "sdiv",
                        "%": "srem",
                        "&": "and",
                        "|": "or",
                        "^": "xor",
                    }
                    self.emit(f"  {t} = {op_map[op]} i32 {l}, {r}")
                    return t
                if op in {"==", "!=", "<", "<=", ">", ">="}:
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
                raise CompileError(f"codegen error: unsupported unary operator {op!r}")
            case AssignExpr(name=name, op=op, expr=expr):
                rv = self.codegen_expr(expr)
                if rv is None:
                    raise CompileError("codegen error: cannot assign void to int")
                slot = self.resolve_var_ptr(name)
                if op == "=":
                    self.emit(f"  store i32 {rv}, ptr {slot}")
                    return rv
                cur = self.tmp()
                self.emit(f"  {cur} = load i32, ptr {slot}")
                t = self.tmp()
                op_map = {"+=": "add", "-=": "sub", "*=": "mul", "/=": "sdiv", "%=": "srem"}
                llvm_op = op_map.get(op)
                if llvm_op is None:
                    raise CompileError(f"codegen error: unsupported assignment operator {op!r}")
                self.emit(f"  {t} = {llvm_op} i32 {cur}, {rv}")
                self.emit(f"  store i32 {t}, ptr {slot}")
                return t
            case IncDec(name=name, op=op, prefix=prefix):
                slot = self.resolve_var_ptr(name)
                cur = self.tmp()
                self.emit(f"  {cur} = load i32, ptr {slot}")
                nxt = self.tmp()
                llvm_op = "add" if op == "++" else "sub"
                self.emit(f"  {nxt} = {llvm_op} i32 {cur}, 1")
                self.emit(f"  store i32 {nxt}, ptr {slot}")
                return nxt if prefix else cur
            case Call(name=name, args=args):
                sig = self.func_sigs[name]
                args_text: List[str] = []
                for arg in args:
                    v = self.codegen_expr(arg)
                    if v is None:
                        raise CompileError("codegen error: void argument is not allowed")
                    args_text.append(f"i32 {v}")
                llvm_ret_ty = self.llvm_ty(sig.ret_type)
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
            case Decl(name=name, init=init):
                slot = f"%{name}"
                self.slots[name] = slot
                self.emit(f"  {slot} = alloca i32")
                if init is not None:
                    v = self.codegen_expr(init)
                    if v is None:
                        raise CompileError("codegen error: cannot initialize int with void")
                    self.emit(f"  store i32 {v}, ptr {slot}")
                return False
            case Block(body=body):
                for st in body:
                    if self.codegen_stmt(st, fn_ret_type):
                        return True
                return False
            case If(cond=cond, then_stmt=then_stmt, else_stmt=else_stmt):
                cond_v = self.codegen_expr(cond)
                if cond_v is None:
                    raise CompileError("codegen error: void condition in if")
                cond_b = self.tmp()
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
                self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(body_lbl)
                self.loop_stack.append((end_lbl, cond_lbl))
                body_ret = self.codegen_stmt(body, fn_ret_type)
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
                    self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                    self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(body_lbl)
                self.loop_stack.append((end_lbl, post_lbl))
                body_ret = self.codegen_stmt(body, fn_ret_type)
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
                body_ret = self.codegen_stmt(body, fn_ret_type)
                self.loop_stack.pop()
                if not body_ret:
                    self.emit(f"  br label %{cond_lbl}")
                self.emit_label(cond_lbl)
                cond_v = self.codegen_expr(cond)
                if cond_v is None:
                    raise CompileError("codegen error: void condition in do-while")
                cond_b = self.tmp()
                self.emit(f"  {cond_b} = icmp ne i32 {cond_v}, 0")
                self.emit(f"  br i1 {cond_b}, label %{body_lbl}, label %{end_lbl}")
                self.emit_label(end_lbl)
                return False
            case Break():
                if not self.loop_stack:
                    raise CompileError("codegen error: break used outside loop")
                break_lbl, _ = self.loop_stack[-1]
                self.emit(f"  br label %{break_lbl}")
                return True
            case Continue():
                if not self.loop_stack:
                    raise CompileError("codegen error: continue used outside loop")
                _, cont_lbl = self.loop_stack[-1]
                self.emit(f"  br label %{cont_lbl}")
                return True
            case EmptyStmt():
                return False
            case Assign(name=name, expr=expr):
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: cannot assign void to int")
                self.emit(f"  store i32 {v}, ptr {self.resolve_var_ptr(name)}")
                return False
            case ExprStmt(expr=expr):
                self.codegen_expr(expr)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    self.emit("  ret void")
                else:
                    if expr is None:
                        raise CompileError("codegen error: int function must return value")
                    v = self.codegen_expr(expr)
                    if v is None:
                        raise CompileError("codegen error: int function cannot return void")
                    self.emit(f"  ret i32 {v}")
                return True
            case _:
                raise CompileError(f"codegen error: unsupported stmt {type(n).__name__}")

    def emit_declarations(self) -> None:
        for name, init in self.global_vars.items():
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
            params = ", ".join(self.llvm_ty(p.typ) for p in d.params)
            self.emit(f"declare {ret_ty} @{d.name}({params})")

    def codegen_function(self, fn: Function) -> None:
        self.tmp_idx = 0
        self.label_idx = 0
        self.slots = {}
        self.loop_stack = []

        ret_ty = self.llvm_ty(fn.ret_type)
        params_sig: List[str] = []
        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            params_sig.append(f"i32 %{pname}.arg")
        self.emit(f"define {ret_ty} @{fn.name}({', '.join(params_sig)}) {{")
        self.emit("entry:")

        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            slot = f"%{pname}"
            self.slots[pname] = slot
            self.emit(f"  {slot} = alloca i32")
            self.emit(f"  store i32 %{pname}.arg, ptr {slot}")

        saw_return = False
        for st in fn.body:
            saw_return = self.codegen_stmt(st, fn.ret_type)
            if saw_return:
                break

        if not saw_return:
            if fn.ret_type == "void":
                self.emit("  ret void")
            else:
                self.emit("  ret i32 0")

        self.emit("}")

    def codegen_program(self) -> str:
        self.emit_declarations()
        if self.lines:
            self.emit("")
        for idx, fn in enumerate(self.prog.funcs):
            self.codegen_function(fn)
            if idx != len(self.prog.funcs) - 1:
                self.emit("")
        return "\n".join(self.lines) + "\n"


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
