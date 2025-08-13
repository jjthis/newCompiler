#include <bits/stdc++.h>
using namespace std;

enum class TokenType {
    End, Number, Ident,
    Plus, Minus, Star, Slash,
    LParen, RParen,
    Assign, Semicolon,
    KwPrint
};

struct Token {
    TokenType type;
    string text;
    long long number;
    Token(TokenType t=TokenType::End): type(t), number(0) {}
};

struct Lexer {
    string s; size_t i=0;
    Lexer(const string &src): s(src) {}
    void skipws(){ while(i<s.size() && isspace((unsigned char)s[i])) ++i; }
    bool starts_ident() { return isalpha((unsigned char)s[i]) || s[i]=='_'; }
    Token next(){
        skipws();
        if(i>=s.size()) return Token(TokenType::End);
        char c = s[i];
        if(isdigit((unsigned char)c)){
            long long v=0;
            size_t st=i;
            while(i<s.size() && isdigit((unsigned char)s[i])){
                v = v*10 + (s[i]-'0');
                ++i;
            }
            Token t(TokenType::Number); t.number = v; t.text = s.substr(st, i-st);
            return t;
        }
        if(starts_ident()){
            size_t st = i;
            while(i<s.size() && (isalnum((unsigned char)s[i])||s[i]=='_')) ++i;
            string w = s.substr(st, i-st);
            if(w=="print") { Token t(TokenType::KwPrint); t.text=w; return t; }
            Token t(TokenType::Ident); t.text = w; return t;
        }
        ++i;
        switch(c){
            case '+': return Token(TokenType::Plus);
            case '-': return Token(TokenType::Minus);
            case '*': return Token(TokenType::Star);
            case '/': return Token(TokenType::Slash);
            case '(' : return Token(TokenType::LParen);
            case ')' : return Token(TokenType::RParen);
            case '=' : return Token(TokenType::Assign);
            case ';' : return Token(TokenType::Semicolon);
            default:
                cerr << "Unexpected char in lexer: '"<<c<<"'\n";
                return Token(TokenType::End);
        }
    }
};

/* ---- AST ---- */
struct CodeGenContext;
struct Node {
    virtual ~Node() = default;
    virtual void collect_vars(set<string>&) const {}
    virtual void gen(CodeGenContext& ctx) const = 0;
};

using NodePtr = unique_ptr<Node>;

struct Expr : Node {};

struct NumberNode : Expr {
    long long v;
    NumberNode(long long vv): v(vv) {}
    void gen(CodeGenContext& ctx) const override;
};

struct VarNode : Expr {
    string name;
    VarNode(string n): name(move(n)) {}
    void collect_vars(set<string>& vs) const override { vs.insert(name); }
    void gen(CodeGenContext& ctx) const override;
};

struct BinaryNode : Expr {
    char op;
    NodePtr L, R;
    BinaryNode(char o, NodePtr l, NodePtr r): op(o), L(move(l)), R(move(r)) {}
    void collect_vars(set<string>& vs) const override { L->collect_vars(vs); R->collect_vars(vs); }
    void gen(CodeGenContext& ctx) const override;
};

struct Stmt : Node {};

struct AssignNode : Stmt {
    string name;
    NodePtr expr;
    AssignNode(string n, NodePtr e): name(move(n)), expr(move(e)) {}
    void collect_vars(set<string>& vs) const override { vs.insert(name); expr->collect_vars(vs); }
    void gen(CodeGenContext& ctx) const override;
};

struct PrintNode : Stmt {
    NodePtr expr;
    PrintNode(NodePtr e): expr(move(e)) {}
    void collect_vars(set<string>& vs) const override { expr->collect_vars(vs); }
    void gen(CodeGenContext& ctx) const override;
};

/* ---- Parser (recursive descent) ---- */
struct Parser {
    vector<Token> toks; size_t p=0;
    Parser(vector<Token>&& t): toks(move(t)) {}
    Token& cur(){ return toks[p]; }
    void eat(){ if(p<toks.size()) ++p; }
    bool accept(TokenType t){ if(cur().type==t){ eat(); return true;} return false; }
    bool expect(TokenType t, const string &msg=""){
        if(cur().type==t){ eat(); return true; }
        cerr << "Parse error, expected " << msg << "\n"; return false;
    }

    // expression grammar:
    // expr := add
    // add := mul ( ('+'|'-') mul )*
    // mul := unary ( ('*'|'/') unary )*
    // unary := ('-' unary) | primary
    // primary := NUMBER | IDENT | '(' expr ')'

    NodePtr parse_primary(){
        if(cur().type==TokenType::Number){
            auto n= make_unique<NumberNode>(cur().number);
            eat(); return n;
        }
        if(cur().type==TokenType::Ident){
            string name = cur().text; eat(); return make_unique<VarNode>(name);
        }
        if(cur().type==TokenType::LParen){
            eat();
            auto e = parse_expr();
            expect(TokenType::RParen, "')'");
            return e;
        }
        cerr<<"Unexpected token in primary\n"; return nullptr;
    }
    NodePtr parse_unary(){
        if(accept(TokenType::Minus)){
            auto node = parse_unary();
            // Implement as (0 - node)
            return make_unique<BinaryNode>('-', make_unique<NumberNode>(0), move(node));
        }
        return parse_primary();
    }
    NodePtr parse_mul(){
        auto left = parse_unary();
        while(cur().type==TokenType::Star || cur().type==TokenType::Slash){
            char op = (cur().type==TokenType::Star? '*':'/');
            eat();
            auto right = parse_unary();
            left = make_unique<BinaryNode>(op, move(left), move(right));
        }
        return left;
    }
    NodePtr parse_expr(){
        auto left = parse_mul();
        while(cur().type==TokenType::Plus || cur().type==TokenType::Minus){
            char op = (cur().type==TokenType::Plus? '+':'-');
            eat();
            auto right = parse_mul();
            left = make_unique<BinaryNode>(op, move(left), move(right));
        }
        return left;
    }

    // parse statements until End
    vector<NodePtr> parse_all(){
        vector<NodePtr> out;
        while(cur().type != TokenType::End){
            if(cur().type == TokenType::KwPrint){
                eat();
                auto e = parse_expr();
                expect(TokenType::Semicolon, "';'");
                out.push_back(make_unique<PrintNode>(move(e)));
            } else if(cur().type == TokenType::Ident){
                string name = cur().text; eat();
                if(cur().type == TokenType::Assign){
                    eat();
                    auto e = parse_expr();
                    expect(TokenType::Semicolon, "';'");
                    out.push_back(make_unique<AssignNode>(name, move(e)));
                } else {
                    cerr<<"Unexpected token after identifier\n";
                    break;
                }
            } else {
                // expression statement treated as print
                auto e = parse_expr();
                expect(TokenType::Semicolon, "';'");
                out.push_back(make_unique<PrintNode>(move(e)));
            }
        }
        return out;
    }
};

/* ---- Code generation context ---- */
struct CodeGenContext {
    ostream &out;
    set<string> vars; // variable names
    CodeGenContext(ostream &o): out(o) {}
    void header(){
        out <<
            "extern printf\n"
            "extern ExitProcess\n"
            "\n"
            "global main\n"
            "\n"
            "section .data\n"
            "    fmt dq 0 ; placeholder, overwritten below\n";
        // fmt will be written more cleanly later; keep placeholder
    }
    void footer(){
        // nothing here
    }
};

/* ---- Code generation for expressions/statements ----
   We'll use a stack-based evaluation: each expr.gen pushes result into stack (push rax),
   BinaryNode will pop rbx, pop rax, operate, push rax.
   For variables we use [rel varname] (dq 0) in .data.
   For print: pop rax, mov rcx, fmt, mov rdx, rax, call printf
*/

void NumberNode::gen(CodeGenContext& ctx) const {
    ctx.out << "    mov rax, " << v << "\n";
    ctx.out << "    push rax\n";
}

void VarNode::gen(CodeGenContext& ctx) const {
    ctx.out << "    lea rax, [rel " << name << "]\n";
    ctx.out << "    mov rax, [rax]\n";
    ctx.out << "    push rax\n";
}

void BinaryNode::gen(CodeGenContext& ctx) const {
    // evaluate left then right (so stack: ... left right)
    L->gen(ctx);
    R->gen(ctx);
    ctx.out << "    pop rbx\n"; // right
    ctx.out << "    pop rax\n"; // left
    if(op == '+'){
        ctx.out << "    add rax, rbx\n";
    } else if(op == '-'){
        ctx.out << "    sub rax, rbx\n";
    } else if(op == '*'){
        ctx.out << "    imul rax, rbx\n";
    } else if(op == '/'){
        // signed division: dividend in rax, divisor in rbx
        ctx.out << "    cqo\n";        // sign extend rax -> rdx:rax
        ctx.out << "    idiv rbx\n";  // quotient in rax, remainder in rdx
    }
    ctx.out << "    push rax\n";
}

void AssignNode::gen(CodeGenContext& ctx) const {
    // evaluate rhs, then pop and store to var
    expr->gen(ctx);
    ctx.out << "    pop rax\n";
    ctx.out << "    mov [rel " << name << "], rax\n";
}

void PrintNode::gen(CodeGenContext& ctx) const {
    expr->gen(ctx);
    ctx.out << "    pop rax\n";
    // call printf with (fmt, value). Windows x64: RCX=format, RDX=value
    ctx.out << "    lea rcx, [rel fmt_print]\n";
    ctx.out << "    mov rdx, rax\n";
    // align stack: ensure rsp 16-byte before call. We're already in prologue ensuring alignment and we keep pushes; but to be safe, allocate shadow again around call
    ctx.out << "    sub rsp, 40\n";
    ctx.out << "    call printf\n";
    ctx.out << "    add rsp, 40\n";
}

/* ---- Utility: collect variables from ASTs ---- */
void collect_vars_from_list(const vector<NodePtr>& stmts, set<string>& vs){
    for(auto &s: stmts) s->collect_vars(vs);
}

/* ---- top-level assembly generation ---- */
void generate_asm(const vector<NodePtr>& stmts, const string &outpath){
    ofstream f(outpath);
    if(!f) { cerr<<"Cannot open output file\n"; return; }
    CodeGenContext ctx(f);

    // gather var names
    set<string> vars;
    collect_vars_from_list(stmts, vars);

    // header + data
    f << "extern printf\nextern ExitProcess\n\nglobal main\n\nsection .data\n";
    // declare variables
    for(auto &v: vars){
        f << "    " << v << " dq 0\n";
    }
    // printf format
    f << "fmt_print db \"%lld\", 0Dh,0Ah, 0\n\n";

    // text
    f << "section .text\n";
    f << "main:\n";
    // prologue: sub rsp, 40 (shadow + align)
    f << "    sub rsp, 40\n";

    // generate each statement
    for(auto &s: stmts){
        s->gen(ctx);
    }

    // exit: call ExitProcess(0)
    f << "    xor ecx, ecx\n";
    f << "    call ExitProcess\n";

    f.close();
}

int main(){
    ifstream in("code.jt");
    if(!in) { cerr<<"Failed to open code.jt\n"; return 1; }
    string src, line;
    while(getline(in, line)){
        src += line;
        src += '\n';
    }

    Lexer lx(src);
    vector<Token> toks;
    while(true){
        Token t = lx.next();
        toks.push_back(t);
        if(t.type == TokenType::End) break;
    }

    Parser parser(move(toks));
    auto stmts = parser.parse_all();

    generate_asm(stmts, "out.asm");
    cout << "Generated out.asm\n";
    return 0;
}
