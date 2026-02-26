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
    KW_STRUCT = auto()
    KW_TYPEDEF = auto()
    KW_STATIC = auto()
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
            "struct": TokenType.KW_STRUCT,
            "typedef": TokenType.KW_TYPEDEF,
            "static": TokenType.KW_STATIC,
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
    struct_defs: List["StructDef"]
    decls: List[FunctionDecl]
    globals: List["GlobalVar"]
    funcs: List[Function]


@dataclass
class Decl(Node):
    name: str
    base_type: str
    ptr_level: int
    size: Optional[int]
    init: Optional[Node]


@dataclass
class Assign(Node):
    name: str
    expr: Node


@dataclass
class GlobalVar(Node):
    name: str
    base_type: str
    ptr_level: int
    size: Optional[int]
    init: Optional[Node]


@dataclass
class AssignExpr(Node):
    target: Node
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
class Index(Node):
    base: Node
    index: Node


@dataclass
class Member(Node):
    base: Node
    field: str
    through_ptr: bool


@dataclass
class StructField(Node):
    typ: str
    name: str


@dataclass
class StructDef(Node):
    name: str
    fields: List[StructField]


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
class Cast(Node):
    typ: str
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
        self.struct_defs: Dict[str, StructDef] = {}
        self.anon_struct_idx = 0
        self.typedef_names: Dict[str, str] = {}

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
            case TokenType.KW_STRUCT:
                self.advance()
                tag: Optional[str] = None
                if self.cur().typ == TokenType.ID:
                    tag = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LBRACE:
                    self.advance()
                    fields: List[StructField] = []
                    while self.cur().typ != TokenType.RBRACE:
                        field_base = self.parse_type()
                        field_ptr = 0
                        while self.cur().typ == TokenType.STAR:
                            self.advance()
                            field_ptr += 1
                        field_name = self.eat(TokenType.ID).text
                        self.eat(TokenType.SEMI)
                        fields.append(
                            StructField(field_base + ("*" * field_ptr), field_name)
                        )
                    self.eat(TokenType.RBRACE)
                    if tag is None:
                        tag = f"__anon{self.anon_struct_idx}"
                        self.anon_struct_idx += 1
                    self.struct_defs[tag] = StructDef(tag, fields)
                    return f"struct:{tag}"
                if tag is None:
                    raise self.error("expected struct tag or definition")
                return f"struct:{tag}"
            case TokenType.ID:
                t = self.cur()
                ty = self.typedef_names.get(t.text)
                if ty is None:
                    raise self.error("expected type")
                self.advance()
                return ty
            case _:
                raise self.error("expected type")

    def is_type_start(self) -> bool:
        if self.cur().typ in (TokenType.KW_INT, TokenType.KW_VOID, TokenType.KW_STRUCT):
            return True
        return self.cur().typ == TokenType.ID and self.cur().text in self.typedef_names

    def parse_params(self) -> List[Param]:
        if self.cur().typ == TokenType.RPAREN:
            return []

        if self.cur().typ == TokenType.KW_VOID and self.peek().typ == TokenType.RPAREN:
            self.advance()
            return []

        params: List[Param] = []
        while True:
            typ = self.parse_type()
            ptr_level = 0
            while self.cur().typ == TokenType.STAR:
                self.advance()
                ptr_level += 1
            typ = typ + ("*" * ptr_level)
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
            if ext is None:
                continue
            if isinstance(ext, FunctionDecl):
                decls.append(ext)
            elif isinstance(ext, list):
                globals_.extend(ext)
            else:
                funcs.append(ext)
        self.eat(TokenType.EOF)
        return Program(list(self.struct_defs.values()), decls, globals_, funcs)

    def parse_external(self) -> FunctionDecl | list[GlobalVar] | Function | None:
        is_typedef = False
        if self.cur().typ == TokenType.KW_TYPEDEF:
            is_typedef = True
            self.advance()
        if self.cur().typ in (TokenType.KW_EXTERN, TokenType.KW_STATIC):
            self.advance()
        base_type = self.parse_type()
        ret_ptr_level = 0
        while self.cur().typ == TokenType.STAR:
            self.advance()
            ret_ptr_level += 1
        ret_type = base_type + ("*" * ret_ptr_level)
        name = self.eat(TokenType.ID).text

        if is_typedef:
            self.typedef_names[name] = ret_type
            while self.cur().typ == TokenType.COMMA:
                self.advance()
                ptr = 0
                while self.cur().typ == TokenType.STAR:
                    self.advance()
                    ptr += 1
                alias = self.eat(TokenType.ID).text
                self.typedef_names[alias] = base_type + ("*" * ptr)
            self.eat(TokenType.SEMI)
            return None

        if self.cur().typ != TokenType.LPAREN:
            if base_type == "void":
                raise self.error("void object type is not supported")
            decls: List[GlobalVar] = []
            size: Optional[int] = None
            if self.cur().typ == TokenType.LBRACKET:
                self.advance()
                size_tok = self.eat(TokenType.NUM)
                size = int(size_tok.text)
                self.eat(TokenType.RBRACKET)
            init: Optional[Node] = None
            if self.cur().typ == TokenType.ASSIGN:
                if size is not None:
                    raise self.error("array initializer is not supported yet")
                self.advance()
                init = self.parse_expr()
            decls.append(GlobalVar(name, base_type, ret_ptr_level, size, init))
            while self.cur().typ == TokenType.COMMA:
                self.advance()
                g_ptr_level = 0
                while self.cur().typ == TokenType.STAR:
                    self.advance()
                    g_ptr_level += 1
                gname = self.eat(TokenType.ID).text
                gsize: Optional[int] = None
                if self.cur().typ == TokenType.LBRACKET:
                    self.advance()
                    size_tok = self.eat(TokenType.NUM)
                    gsize = int(size_tok.text)
                    self.eat(TokenType.RBRACKET)
                ginit: Optional[Node] = None
                if self.cur().typ == TokenType.ASSIGN:
                    if gsize is not None:
                        raise self.error("array initializer is not supported yet")
                    self.advance()
                    ginit = self.parse_expr()
                decls.append(GlobalVar(gname, base_type, g_ptr_level, gsize, ginit))
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
            case _ if self.is_type_start():
                base_type = self.parse_type()
                if base_type == "void":
                    raise self.error("void object type is not supported")
                decls: List[Node] = []
                while True:
                    ptr_level = 0
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                        ptr_level += 1
                    name = self.eat(TokenType.ID).text
                    size: Optional[int] = None
                    if self.cur().typ == TokenType.LBRACKET:
                        self.advance()
                        size_tok = self.eat(TokenType.NUM)
                        size = int(size_tok.text)
                        self.eat(TokenType.RBRACKET)
                    init: Optional[Node] = None
                    if self.cur().typ == TokenType.ASSIGN:
                        if size is not None:
                            raise self.error("array initializer is not supported yet")
                        self.advance()
                        init = self.parse_expr()
                    decls.append(Decl(name, base_type, ptr_level, size, init))
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
        if not (
            isinstance(lhs, (Var, Index, Member))
            or (isinstance(lhs, UnaryOp) and lhs.op == "*")
        ):
            raise self.error("left-hand side of assignment must be a variable")
        return AssignExpr(lhs, op, rhs)

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
        if self.cur().typ == TokenType.LPAREN and self.peek().typ in (
            TokenType.KW_INT,
            TokenType.KW_VOID,
            TokenType.KW_STRUCT,
            TokenType.ID,
        ):
            save = self.i
            self.advance()
            if self.is_type_start():
                try:
                    cast_ty = self.parse_type()
                    ptr_level = 0
                    while self.cur().typ == TokenType.STAR:
                        self.advance()
                        ptr_level += 1
                    cast_ty = cast_ty + ("*" * ptr_level)
                    if self.cur().typ == TokenType.RPAREN:
                        self.advance()
                        return Cast(cast_ty, self.parse_unary())
                except CompileError:
                    pass
            self.i = save

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
        while True:
            if self.cur().typ == TokenType.LBRACKET:
                self.advance()
                idx = self.parse_expr()
                self.eat(TokenType.RBRACKET)
                node = Index(node, idx)
                continue
            if self.cur().typ == TokenType.DOT:
                self.advance()
                field = self.eat(TokenType.ID).text
                node = Member(node, field, False)
                continue
            if self.cur().typ == TokenType.ARROW:
                self.advance()
                field = self.eat(TokenType.ID).text
                node = Member(node, field, True)
                continue
            if self.cur().typ in (TokenType.PLUSPLUS, TokenType.MINUSMINUS):
                if not isinstance(node, Var):
                    raise self.error("increment/decrement target must be a variable")
                op = self.cur().text
                self.advance()
                node = IncDec(node.name, op, False)
                continue
            break
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
        self.struct_defs: Dict[str, StructDef] = {}
        self.global_vars: Dict[str, bool] = {}
        self.global_arrays: Dict[str, int] = {}
        self.local_arrays: Dict[str, int] = {}
        self.global_types: Dict[str, str] = {}
        self.local_types: Dict[str, str] = {}

    def ensure_sig(self, name: str, sig: FunctionSig) -> None:
        prev = self.func_sigs.get(name)
        if prev is None:
            self.func_sigs[name] = sig
            return
        if prev != sig:
            raise CompileError(f"semantic error: conflicting declaration for function {name!r}")

    def analyze_program(self, p: Program) -> None:
        self.struct_defs = {s.name: s for s in p.struct_defs}
        for g in p.globals:
            if g.name in self.global_vars:
                raise CompileError(f"semantic error: duplicate global variable {g.name!r}")
            if g.size is not None:
                if g.size <= 0:
                    raise CompileError(f"semantic error: array size must be positive for {g.name!r}")
                self.global_arrays[g.name] = g.size
                self.global_types[g.name] = "array"
            elif g.ptr_level > 0:
                self.global_types[g.name] = g.base_type + ("*" * g.ptr_level)
            else:
                self.global_types[g.name] = g.base_type
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
        self.local_arrays = {}
        self.local_types = {}
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
            self.local_types[param.name] = param.typ

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
            case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, init=init):
                if name in vars_init:
                    raise CompileError(f"semantic error: variable redeclared: {name!r}")
                if size is not None:
                    if size <= 0:
                        raise CompileError(
                            f"semantic error: array size must be positive for {name!r}"
                        )
                    self.local_arrays[name] = size
                    self.local_types[name] = "array"
                    vars_init[name] = True
                    if init is not None:
                        raise CompileError(
                            "semantic error: array initializer is not supported yet"
                        )
                    return False
                self.local_types[name] = base_type + ("*" * ptr_level)
                vars_init[name] = False
                if init is not None:
                    init_ty = self.analyze_expr(init, vars_init)
                    if init_ty != self.local_types[name]:
                        raise CompileError(
                            f"semantic error: initializer type mismatch for {name!r}"
                        )
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
                if name in self.local_arrays or name in self.global_arrays:
                    raise CompileError("semantic error: cannot assign to array object")
                et = self.analyze_expr(expr, vars_init)
                vt = self.var_type(name)
                if et != vt:
                    raise CompileError("semantic error: assignment type mismatch")
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
                    rt = self.analyze_expr(expr, vars_init)
                    expected = "ptr" if fn_ret_type.endswith("*") else "int"
                    if rt != expected:
                        raise CompileError("semantic error: return type mismatch")
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
                        return self.var_type(name)
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                vt = self.var_type(name)
                if vt == "array":
                    return "array"
                if self.is_struct_type(vt):
                    return vt
                if not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                return vt
            case Index(base=base, index=index):
                bt = self.analyze_expr(base, vars_init)
                it = self.analyze_expr(index, vars_init)
                if bt not in ("array", "ptr"):
                    if not self.is_ptr_type(bt):
                        raise CompileError("semantic error: index base must be an array or pointer")
                if it != "int":
                    raise CompileError("semantic error: index must be int")
                if bt == "array":
                    return "int"
                if self.is_ptr_type(bt):
                    return bt[:-1]
                return "int"
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.analyze_expr(base, vars_init)
                if through_ptr:
                    if not self.is_ptr_type(bt):
                        raise CompileError("semantic error: '->' requires pointer base")
                    bt = bt[:-1]
                if not self.is_struct_type(bt):
                    raise CompileError("semantic error: member access requires struct type")
                return self.lookup_struct_field_type(bt, field)
            case BinOp(lhs=lhs, rhs=rhs):
                lt = self.analyze_expr(lhs, vars_init)
                rt = self.analyze_expr(rhs, vars_init)
                if n.op in ("+", "-"):
                    if lt == "int" and rt == "int":
                        return "int"
                    if self.is_ptr_type(lt) and rt == "int":
                        return lt
                    if n.op == "+" and lt == "int" and self.is_ptr_type(rt):
                        return rt
                    if n.op == "-" and self.is_ptr_type(lt) and lt == rt:
                        return "int"
                    raise CompileError("semantic error: invalid pointer arithmetic")
                if n.op in ("*", "/", "%", "&", "|", "^"):
                    if lt != "int" or rt != "int":
                        raise CompileError("semantic error: binary op currently supports int only")
                    return "int"
                if n.op in ("==", "!="):
                    if lt == rt:
                        return "int"
                    if (self.is_ptr_type(lt) and self.is_nullptr_constant(rhs)) or (
                        self.is_ptr_type(rt) and self.is_nullptr_constant(lhs)
                    ):
                        return "int"
                    raise CompileError("semantic error: incompatible types in equality comparison")
                if n.op in ("<", "<=", ">", ">="):
                    if lt == "int" and rt == "int":
                        return "int"
                    raise CompileError("semantic error: relational ops currently support int only")
                if n.op in ("&&", "||"):
                    if not self.is_scalar_type(lt) or not self.is_scalar_type(rt):
                        raise CompileError("semantic error: logical ops require scalar operands")
                    return "int"
                raise CompileError(f"semantic error: unsupported binary op {n.op!r}")
            case UnaryOp(op="&", expr=expr):
                match expr:
                    case Var(name=name):
                        vt = self.var_type(name)
                        if vt == "array":
                            return "ptr"
                        return vt + "*"
                    case Index():
                        et = self.analyze_expr(expr, vars_init)
                        return et + "*"
                    case _:
                        raise CompileError("semantic error: unary '&' requires lvalue")
            case UnaryOp(op="*", expr=expr):
                et = self.analyze_expr(expr, vars_init)
                if not self.is_ptr_type(et):
                    raise CompileError("semantic error: unary '*' requires pointer operand")
                return et[:-1]
            case UnaryOp(op=op, expr=expr):
                et = self.analyze_expr(expr, vars_init)
                if et != "int":
                    raise CompileError(
                        f"semantic error: unary op {op!r} currently supports int only"
                    )
                return "int"
            case Cast(typ=typ, expr=expr):
                self.analyze_expr(expr, vars_init)
                return typ
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
                vt = self.var_type(name)
                if vt != "int" and not self.is_ptr_type(vt):
                    raise CompileError("semantic error: increment/decrement supports int/pointer only")
                return vt
            case AssignExpr(target=target, op=op, expr=expr):
                et = self.analyze_expr(expr, vars_init)
                match target:
                    case Var(name=name):
                        if name not in vars_init and name not in self.global_vars:
                            raise CompileError(
                                f"semantic error: assignment to undeclared variable {name!r}"
                            )
                        if name in self.local_arrays or name in self.global_arrays:
                            raise CompileError("semantic error: cannot assign to array object")
                        vt = self.var_type(name)
                        if et != vt:
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=" and name in vars_init and not vars_init[name]:
                            raise CompileError(
                                f"semantic error: use of uninitialized variable {name!r}"
                            )
                        if op != "=" and vt != "int":
                            if not (vt.endswith("*") and op in ("+=", "-=")):
                                raise CompileError(
                                    "semantic error: compound assignment supports int and pointer +/- only"
                                )
                        if name in vars_init:
                            vars_init[name] = True
                    case Index():
                        if et != "int":
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=":
                            raise CompileError(
                                "semantic error: compound assignment supports scalar variable only"
                            )
                        self.analyze_expr(target, vars_init)
                    case Member():
                        tt = self.analyze_expr(target, vars_init)
                        if et != tt:
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=" and tt != "int":
                            raise CompileError(
                                "semantic error: compound assignment supports int only"
                            )
                    case UnaryOp(op="*", expr=ptr_expr):
                        pt = self.analyze_expr(ptr_expr, vars_init)
                        if not self.is_ptr_type(pt):
                            raise CompileError("semantic error: unary '*' requires pointer operand")
                        if et != pt[:-1]:
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=":
                            raise CompileError(
                                "semantic error: compound assignment supports scalar variable only"
                            )
                    case _:
                        raise CompileError("semantic error: invalid assignment target")
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
            case Cast(expr=expr):
                return self.analyze_const_expr(expr)
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

    def var_type(self, name: str) -> str:
        if name in self.local_types:
            return self.local_types[name]
        if name in self.global_types:
            return self.global_types[name]
        raise CompileError(f"semantic error: unknown variable {name!r}")

    def is_ptr_type(self, ty: str) -> bool:
        return ty.endswith("*")

    def is_struct_type(self, ty: str) -> bool:
        return ty.startswith("struct:")

    def is_scalar_type(self, ty: str) -> bool:
        return ty == "int" or self.is_ptr_type(ty)

    def is_nullptr_constant(self, n: Node) -> bool:
        return isinstance(n, IntLit) and n.value == 0

    def lookup_struct_field_type(self, struct_ty: str, field: str) -> str:
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            raise CompileError(f"semantic error: unknown struct type {struct_ty!r}")
        for f in s.fields:
            if f.name == field:
                return f.typ
        raise CompileError(f"semantic error: unknown field {field!r} in {struct_ty!r}")


class IRBuilder:
    def __init__(self, prog: Program):
        self.prog = prog
        self.tmp_idx = 0
        self.label_idx = 0
        self.lines: List[str] = []
        self.slots: Dict[str, str] = {}
        self.local_arrays: Dict[str, int] = {}
        self.local_types: Dict[str, str] = {}
        self.loop_stack: List[tuple[str, str]] = []
        self.func_sigs: Dict[str, FunctionSig] = {}
        self.struct_defs: Dict[str, StructDef] = {s.name: s for s in prog.struct_defs}
        self.global_vars: Dict[str, Optional[Node]] = {}
        self.global_arrays: Dict[str, int] = {}
        self.global_types: Dict[str, str] = {}
        for d in prog.decls:
            self.func_sigs[d.name] = FunctionSig(d.ret_type, [p.typ for p in d.params])
        for g in prog.globals:
            self.global_vars[g.name] = g.init
            if g.size is not None:
                self.global_arrays[g.name] = g.size
                self.global_types[g.name] = "array"
            elif g.ptr_level > 0:
                self.global_types[g.name] = g.base_type + ("*" * g.ptr_level)
            else:
                self.global_types[g.name] = g.base_type
        for fn in prog.funcs:
            self.func_sigs[fn.name] = FunctionSig(fn.ret_type, [p.typ for p in fn.params])

    def llvm_ty(self, ty: str) -> str:
        if ty == "void":
            return "void"
        if ty.startswith("struct:"):
            tag = ty.split(":", 1)[1]
            return f"%struct.{tag}"
        if ty.endswith("*") or ty == "ptr":
            return "ptr"
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
            case Var(name=name):
                return self.resolve_var_type(name)
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
                ret = self.func_sigs[name].ret_type
                if ret == "void":
                    return "void"
                return ret
            case _:
                return "int"

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
                vty = self.resolve_var_type(name)
                if vty == "array":
                    raise CompileError("codegen error: array object cannot be used as scalar value")
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
                if op in {"+", "-", "*", "/", "%", "&", "|", "^"}:
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
                raise CompileError(f"codegen error: unsupported unary operator {op!r}")
            case Cast(typ=typ, expr=expr):
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: cast from void is not allowed")
                src = self.expr_type(expr)
                dst = typ
                if src == dst:
                    return v
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
            case AssignExpr(target=target, op=op, expr=expr):
                rv = self.codegen_expr(expr)
                if rv is None:
                    raise CompileError("codegen error: cannot assign void to int")
                slot = self.codegen_lvalue_ptr(target)
                target_ty = self.expr_type(target)
                if op == "=":
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
                op_map = {"+=": "add", "-=": "sub", "*=": "mul", "/=": "sdiv", "%=": "srem"}
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
                sig = self.func_sigs[name]
                args_text: List[str] = []
                for i, arg in enumerate(args):
                    v = self.codegen_expr(arg)
                    if v is None:
                        raise CompileError("codegen error: void argument is not allowed")
                    args_text.append(f"{self.llvm_ty(sig.param_types[i])} {v}")
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
            case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, init=init):
                slot = f"%{name}"
                self.slots[name] = slot
                if size is not None:
                    self.local_arrays[name] = size
                    self.local_types[name] = "array"
                    self.emit(f"  {slot} = alloca [{size} x i32]")
                    return False
                self.local_types[name] = base_type + ("*" * ptr_level)
                self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[name])}")
                if init is not None:
                    v = self.codegen_expr(init)
                    if v is None:
                        raise CompileError("codegen error: cannot initialize variable with void")
                    self.emit(f"  store {self.llvm_ty(self.local_types[name])} {v}, ptr {slot}")
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
                    cty = self.expr_type(cond)
                    if cty.endswith("*"):
                        self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                    else:
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
                cty = self.expr_type(cond)
                if cty.endswith("*"):
                    self.emit(f"  {cond_b} = icmp ne ptr {cond_v}, null")
                else:
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
                self.emit(
                    f"  store {self.llvm_ty(self.resolve_var_type(name))} {v}, ptr {self.resolve_var_ptr(name)}"
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
                    self.emit(f"  ret {self.llvm_ty(fn_ret_type)} {v}")
                return True
            case _:
                raise CompileError(f"codegen error: unsupported stmt {type(n).__name__}")

    def emit_declarations(self) -> None:
        for tag, s in self.struct_defs.items():
            field_tys = ", ".join(self.llvm_ty(f.typ) for f in s.fields)
            self.emit(f"%struct.{tag} = type {{{field_tys}}}")
        if self.struct_defs:
            self.emit("")

        for name, init in self.global_vars.items():
            if name in self.global_arrays:
                n_elem = self.global_arrays[name]
                self.emit(f"@{name} = global [{n_elem} x i32] zeroinitializer")
                continue
            gty = self.resolve_var_type(name)
            if gty.endswith("*"):
                self.emit(f"@{name} = global ptr null")
                continue
            if self.is_struct_type(gty):
                self.emit(f"@{name} = global {self.llvm_ty(gty)} zeroinitializer")
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
            params = ", ".join(self.llvm_ty(p.typ) for p in d.params)
            self.emit(f"declare {ret_ty} @{d.name}({params})")

    def codegen_function(self, fn: Function) -> None:
        self.tmp_idx = 0
        self.label_idx = 0
        self.slots = {}
        self.local_arrays = {}
        self.local_types = {}
        self.loop_stack = []

        ret_ty = self.llvm_ty(fn.ret_type)
        params_sig: List[str] = []
        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            params_sig.append(f"{self.llvm_ty(p.typ)} %{pname}.arg")
        self.emit(f"define {ret_ty} @{fn.name}({', '.join(params_sig)}) {{")
        self.emit("entry:")

        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            slot = f"%{pname}"
            self.slots[pname] = slot
            self.local_types[pname] = p.typ
            self.emit(f"  {slot} = alloca {self.llvm_ty(self.local_types[pname])}")
            self.emit(
                f"  store {self.llvm_ty(self.local_types[pname])} %{pname}.arg, ptr {slot}"
            )

        saw_return = False
        for st in fn.body:
            saw_return = self.codegen_stmt(st, fn.ret_type)
            if saw_return:
                break

        if not saw_return:
            if fn.ret_type == "void":
                self.emit("  ret void")
            else:
                default_ret = "null" if fn.ret_type.endswith("*") else "0"
                self.emit(f"  ret {self.llvm_ty(fn.ret_type)} {default_ret}")

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
