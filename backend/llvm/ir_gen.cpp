#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include "ast/tree.h"
#include "ast/keyword.h"
#include "backend/llvm/ir_gen.h"

static int
keyword( const ast_node *node)
{
    assert(node);
    if (node->type == AST_NODE_KEYWORD)
            return ast_keyword( node);

    return 0;
}

static int64_t
number( const ast_node *node)
{
    assert(node);
    if (node->type == AST_NODE_NUMBER)
    {
        return ast_number( node);
    }

    return 0;
}

static uint64_t
unumber( const ast_node *node)
{
    return static_cast<uint64_t>( number( node));
}

static const char*
ident( const ast_node *node)
{
    assert( node && node->type == AST_NODE_IDENT);
    return ast_ident( node);
}

llvm::Module*
IRGenerator::compile( const ast_node* root)
{
    return nullptr;
}

llvm::Value*
IRGenerator::compile_stmt( const ast_node* root)
{
    assert( root);

    if ( root->left )
    {
        compile_stmt( root->left);
    }

    /*
    if ( !root->right || !keyword( root) )
            return syntax_error(root);

            */
    ast_node *stmt = root->right;
    switch ( ast_keyword( stmt) ) {
    case AST_ASSIGN:
            return compile_assign( stmt);
    case AST_DEFINE:
            return compile_define( stmt);
    case AST_IF:
            return compile_if( root->right);
    case AST_WHILE:
            return compile_while( root->right);
    case AST_CALL:
            return compile_call( root->right);
    case AST_OUT:
            /*
$$              error = compile_expr(stmt->right, symtabs, vm);
            if (error)
                    return syntax_error(root);
            error = compile_stdcall(stmt, symtabs, vm, SYM_PRINT);
            if (error)
                    return syntax_error(root);
                    */
            /* Don(t need return value */
            //register_pop(vm);
            //return success(root);
    case AST_RETURN:
            return compile_return( stmt);
    default:
            //return syntax_error(root);
            return nullptr;
    }
}

llvm::Value*
IRGenerator::compile_call( const ast_node* root)
{
    assert( root);

    llvm::Function *function = module_->getFunction( ident( root->left));
    if ( !function )
    {
        throw std::runtime_error{ "can't find function prototype"};
    }

    /* We need to check if function arguments number is correct */
    size_t n_params = 0;
    for ( ast_node* param = root->right;
          param != nullptr;
          param = param->left )
    {
        n_params++;
    }

    if ( n_params != function->arg_size() )
    {
        throw std::runtime_error{ "invalid arguments number"};
    }

    /* Calculate and push function parameters */
    std::vector<llvm::Value*> args;
    for ( ast_node* param = root->right;
          param != nullptr;
          param = param->left )
    {
        args.push_back( compile_expr( param));
    }

    return builder_->CreateCall( function, args, "calltmp");
}

llvm::Value*
IRGenerator::compile_define( const ast_node* root)
{
    assert( root);

    ast_node *func_node = root->left;
    if ( keyword( func_node) != AST_FUNC )
    {
        throw std::runtime_error{ "AST_FUNC node type is required"};
    }

    ast_node *func_name = func_node->left;
    llvm::Function *function = module_->getFunction( ident( func_name));
    assert( function && "It is an assert! All functions must be declared!");

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, "entry", function);
    builder_->SetInsertPoint( bb);

    Scope& scope = scopes_.emplace_back();

    for ( ast_node* param = func_node->right;
          param != nullptr;
          param = param->left )
    {
        for ( auto& arg : function->args() )
        {
            scope.emplace( arg.getName(), &arg);
        }
    }

    // Compile body
    compile_stmt( root->right);

    scopes_.pop_back();
    return nullptr;
}

llvm::Value*
IRGenerator::compile_assign( const ast_node* root)
{
    assert( root);

    if ( keyword( root) != AST_ASSIGN)
    {
        throw std::runtime_error{ "AST_ASSIGN node type is required"};
    }

    const std::string name = ident( root->left);

    if ( scopes_.is_global() )
    {
            throw std::runtime_error{ "can't reassign global variables"};
    }

    // Declare global variable
    ast_node *shift_node = root->left->right;
    size_t shift = shift_node ? (unumber( shift_node) + 1u)
                              : (1u);

    llvm::ArrayType *type = llvm::ArrayType::get( llvm::Type::getInt64Ty( context_), shift);
    if ( !scopes_.find( name) )
    {
        if ( scopes_.is_global() )
        {
            module_->getOrInsertGlobal( name, type);
            llvm::GlobalVariable *global = module_->getNamedGlobal( name);

            llvm::Function *function = module_->getFunction( "__ass_init_globals");
            assert( function);

            llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, name, function);
            builder_->SetInsertPoint( bb);

            // Compile initialization code
            llvm::Value* init_value = compile_expr( root->right);
            llvm::Value *global_ptr = builder_->CreateConstGEP2_64( type, global, 0, shift);
            builder_->CreateStore( init_value, global_ptr);
            return nullptr;

        } else
        {
        }
    }
    }

    if ( scopes_.is_global() )
    {
        {
            throw std::runtime_error{ "can't reassign global variables"};
        }

        // Declare global variable
        ast_node *shift_size = root->left->right;
        module_->getOrInsertGlobal( name,
                                    llvm::ArrayType::get( llvm::Type::getInt64Ty( context_),
                                    sizeof( int64_t) * (unumber( shift_size) + 1)));

        llvm::GlobalVariable *global = module_->getNamedGlobal( name);

        llvm::Function *function = module_->getFunction( "__ass_init_globals");
        assert( function);

        llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, name, function);
        builder_->SetInsertPoint( bb);

        // Compile initialization code
        llvm::Value* init_value = compile_expr( root->right);
        builder_->CreateStore( init_value, global);
        return nullptr;
    }

    llvm::Value* rhs = compile_expr( root->right);
    Scope& scope = scopes_.back();
    scope[name] = rhs;
    return nullptr;
}

static ast_node *compile_assign(ast_node *root, stack *symtabs, ac_virtual_memory *vm)
{
        assert(vm);
        assert(root);
        assert(symtabs);
        ast_node *error = nullptr;
$$
        require(root, AST_ASSIGN);
$$
        require_ident(root->left);
        ac_symbol sym = {
                .type   = AC_SYM_VAR,
                .vis    = AC_VIS_LOCAL,
                .ident  = ast_ident(root->left),
                .node   = root,
                .addend = 0,
                .offset = rip(vm),
                .info   = 8,
        };

        error = compile_expr(root->right, symtabs, vm);
        if (error)
                return error;
$$
        ac_symbol *exist = find_symbol(symtabs, ast_ident(root->left));
        if (exist) {
                /* Can't reassign variables at global scope */
                if (is_global_scope(symtabs))
                        return syntax_error(root);

                return compile_store(root->left, symtabs, vm, exist);
        }
$$
        ast_node *size = root->left->right;
        if (size) {
                require_number(size);
                sym.info = 8 * (ast_number(size) + 1);
        }
$$
        elf64_section *sec = nullptr;
        if (is_global_scope(symtabs)) {
                sym.vis    = AC_VIS_GLOBAL;
                sec = vm->secs + SEC_BSS;
                sym.addend = (imm32)sec->size;
        } else {
                sym.vis    = AC_VIS_LOCAL;
                sec = vm->secs + SEC_NULL;
                sym.addend = - (imm32)((imm32)sec->size + sym.info);
        }

        sec->size += (size_t)sym.info;
$$
        array_push((array *)top_stack(symtabs), &sym, sizeof(ac_symbol));
$       (dump_symtab(symtabs);)
$$
        return compile_store(root->left, symtabs, vm, &sym);
}

    llvm::Value ret_value = compile_stmt();
    builder_->CreateRet( ret_value);
llvm::Value*
IRGenerator::compile_expr( const ast_node* root)
{
    assert( root);

    if ( root->type == AST_NODE_NUMBER )
    {
        return llvm::ConstantInt::get( context_, llvm::APInt( 64, unumber( root), true));
    }

    if ( root->type == AST_NODE_IDENT)
    {
        llvm::Value* variable = scopes_.get( ident( root));
        return builder_->CreateLoad( llvm::Type::getInt64Ty( context_), variable, ident( root));
    }

    if ( ast_keyword( root) == AST_CALL )
    {
        return compile_call( root);
    }

    llvm::Value* lhs = compile_expr( root->left);
    llvm::Value* rhs = compile_expr( root->right);

    switch ( ast_keyword( root) ) {
    case AST_ADD:
        return builder_->CreateAdd( lhs, rhs, "addtmp");
    case AST_SUB:
        return builder_->CreateSub( lhs, rhs, "subtmp");
    case AST_MUL:
        return builder_->CreateMul( lhs, rhs, "multmp");
    case AST_DIV:
        return builder_->CreateSDiv( lhs, rhs, "sdivtmp");
    case AST_NOT:
    case AST_AND:
    case AST_OR:
        return builder_->CreateAdd( lhs, rhs, "addtmp");
    case AST_EQUAL:
    case AST_NEQUAL:
    case AST_GREAT:
    case AST_LOW:
    case AST_GEQUAL:
    case AST_LEQUAL:
        return builder_->CreateAdd( lhs, rhs, "addtmp");
    default:
        return nullptr;
    }
}

/**
 * About
 *
 *
class IRGenerator
{
public:
    IRGenerator()
        : context_{}
        , builder_{ std::make_unique<llvm::IRBuilder<>>( context_)}
        , module_{}
    {}

public:
    llvm::Module* compile( const ast_node* root);

private:
    llvm::Value* compile_stdcall( const ast_node* node);
    llvm::Value* compile_define ( const ast_node* node);
    llvm::Value* compile_return ( const ast_node* node);
    llvm::Value* compile_assign ( const ast_node* node);
    llvm::Value* compile_expr   ( const ast_node* node);
    llvm::Value* compile_while  ( const ast_node* node);
    llvm::Value* compile_call   ( const ast_node* node);
    llvm::Value* compile_stmt   ( const ast_node* node);
    llvm::Value* compile_if     ( const ast_node* node);

private:
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::Module> module_;
};
 */

/*
llvm::Value*
IRGenerator::find_symbol  ( const char* name)
{
    for ( auto&& symtab : symtabs_ )
    {
        auto symbol = symtab.find( name);
        if ( symbol != symtab.end() )
        {
            return symbol->second;
        }
    }

    throw std::out_of_range{ "find variable symbol failed"};
}
*/


