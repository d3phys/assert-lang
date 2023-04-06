<p align="center">
     <img src="resources//logo.png" alt="Logo" width="300"/>
</p>

## Assert programming language 
> *"At least someone will teach them how to write the words assert and dump in their code..."*

```
dump factorial(x) {
        if (x <= 1)
                assert(return 1);
                
        assert(return factorial(x - 1) * x);        
}

dump main() {
        assert(out(factorial(in())));
        assert(return 0);
}
```

Read about Assert in the [Assert Book](https://d3phys.github.io/assert-book/) ❤

#### Project layout
Here you can find an overview of the project structure.
```
/* Directories */
├── ast         Abstract Syntax Tree module
├── backend     Compiler backend code
│   ├── legacy  Legacy hand-written backend
│   └── llvm    LLVM IR backend
├── frontend    Compiler frontend code
├── include     All include files
├── lib         Libraries and Components
│   └── logs    Log and dump subsystem
├── resources   Images and stuff
└── trans       AST to Source code decompiler

/* Files */
├── AST            AST nodes format
├── asslib.s       Assert Standard Library code for legacy backend
├── asslib-llvm.c  Assert Standard Library code for LLVM backend
├── grammar        Language EBNF grammar
├── KEYWORDS       Lexer script
├── STDLIB         Standard functions table
```
