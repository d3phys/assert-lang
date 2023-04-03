#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <iostream>
#include "logs.h"
#include "ast/tree.h"
#include "ast/keyword.h"
#include "backend/llvm/ir_gen.h"

static const char* kGlobalsInitIdent = "__ass_globals_init";

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
    // Generate globals initializer
    llvm::FunctionType *type = llvm::FunctionType::get( llvm::Type::getVoidTy( context_), false);
    //llvm::Function::Create( type, llvm::Function::InternalLinkage, kGlobalsInitIdent, *module_);
    llvm::FunctionCallee init_globals = module_->getOrInsertFunction( kGlobalsInitIdent, type);

    declare_functions( root);

    {
        std::string entry = "main";
        llvm::Function *function = module_->getFunction( entry);
        assert( function);

        llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, "__call_globals_init", function);
        builder_->SetInsertPoint( bb);
        builder_->CreateCall( init_globals);
    }

    Scope& global_scope = scopes_.emplace_back();
    compile_stmt( root);
    scopes_.pop_back();

    {
        llvm::Function *function = module_->getFunction( kGlobalsInitIdent);
        llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, "__final", function);
        builder_->SetInsertPoint( bb);
        builder_->CreateRetVoid();
    }

    std::cout << "#[LLVM IR]:\n";
    std::string s;
    llvm::raw_string_ostream os(s);
    module_->print(os, nullptr);
    os.flush();
    std::cout << s;
    return nullptr;
}

void
IRGenerator::declare_functions( const ast_node* root)
{
    assert( root );

    if ( root->left )
    {
        declare_functions( root->left);
    }

    if ( keyword( root->right) != AST_DEFINE )
    {
        return;
    }

    ast_node *define_node = root->right;
    assert( keyword( define_node) == AST_DEFINE );

    ast_node *func_node = define_node->left;
    assert( keyword( func_node) == AST_FUNC );

    ast_node *name_node = func_node->left;
    assert( name_node );

    std::string name = ident( name_node);

    //
    // AST standard disables double definition.
    //
    for ( llvm::Function& func : module_->getFunctionList() )
    {
        if ( func.getName() == name )
        {
            throw std::runtime_error{ "double function definition"};
        }
    }

    std::vector<llvm::Type*> arg_types{};
    for ( ast_node* param = func_node->right;
          param != nullptr;
          param = param->left )
    {
        arg_types.push_back( llvm::Type::getInt64Ty( context_));
    }

    // According to the AST standard each function must return integer.
    llvm::FunctionType *type = llvm::FunctionType::get( llvm::Type::getInt64Ty( context_),
                                                        std::move( arg_types), false);
    module_->getOrInsertFunction( name, type);
    //llvm::Function::Create( type, llvm::Function::ExternalLinkage, name, *module_);
}

llvm::Value*
IRGenerator::compile_stmt( const ast_node* root)
{
    assert( root);
$$
    if ( root->left )
    {
        compile_stmt( root->left);
    }

    /*
    if ( !root->right || !keyword( root) )
            return syntax_error(root);

            */
$$
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
    {
        assert( stmt->right);
        llvm::Value* return_value = compile_expr( stmt->right);
        builder_->CreateRet( return_value);
        return nullptr;
    }
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
        args.push_back( compile_expr( param->right));
    }

    return builder_->CreateCall( function, args);
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
    llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, "__entry", function);
    builder_->SetInsertPoint( bb);

    Scope& scope = scopes_.emplace_back();

    for ( ast_node* param = func_node->right;
          param != nullptr;
          param = param->left )
    {
        llvm::ArrayType *type = llvm::ArrayType::get( llvm::Type::getInt64Ty( context_), 1);
        for ( llvm::Argument& arg : function->args() )
        {
            // Make function arguments mutable
            llvm::AllocaInst* local = builder_->CreateAlloca( type);
            builder_->CreateStore( &arg, local);
            scope.emplace( ident( param->right), Allocation{ local, type});
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
$$
    if ( keyword( root) != AST_ASSIGN)
    {
        throw std::runtime_error{ "AST_ASSIGN node type is required"};
    }

    const std::string name = ident( root->left);

    ast_node *shift_node = root->left->right;
    size_t shift = shift_node ? unumber( shift_node)
                              : 0;

    //
    // Specific AST standard requirements about variables size:
    // See https://github.com/futherus/language/blob/master/tree_standard.md
    //
    size_t alloca_size = shift + 1;
    llvm::ArrayType *type = llvm::ArrayType::get( llvm::Type::getInt64Ty( context_), alloca_size);

    if ( !scopes_.find( name).value )
    {
        llvm::Value* just_allocated_value{};
        if ( scopes_.is_global() )
        {
            // @var = external global [1 x i64]
            module_->getOrInsertGlobal( name, type);
            just_allocated_value = module_->getNamedGlobal( name);

            //
            // All globals initialization code is placed inside "__ass_init_globals" function.
            // That is why we need to change builder insert point.
            //
            llvm::Function *function = module_->getFunction( kGlobalsInitIdent);
            assert( function);

            llvm::BasicBlock *bb = llvm::BasicBlock::Create( context_, name, function);
            builder_->SetInsertPoint( bb);
        } else
        {
            // %var = alloca [(shift + 1) x i64], align 8
            just_allocated_value = builder_->CreateAlloca( type);
        }

        assert( just_allocated_value);

        // Add just allocated variable to the current scope
        Scope& scope = scopes_.back();
        scope[name] = Allocation{ just_allocated_value, type};

    } else
    {
        // See AST standard...
        if ( scopes_.is_global() )
        {
            throw std::runtime_error{ "can't reassign global variables at global scope!"};
        }
    }

$$
    llvm::Value* rhs = compile_expr( root->right);
    llvm::Value* variable_ptr = get_element_ptr( name, shift);
    builder_->CreateStore( rhs, variable_ptr);

    return nullptr;
}

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
        llvm::Value* index = root->right ? compile_expr( root->right)
                                         : llvm::ConstantInt::get( llvm::Type::getInt64Ty( context_), 0);

        llvm::Value* variable = get_element_ptr( ident( root), index);
        return builder_->CreateLoad( llvm::Type::getInt64Ty( context_), variable);
    }

    if ( ast_keyword( root) == AST_CALL )
    {
        return compile_call( root);
    }

    dump_tree((ast_node*)root);
    assert( root->left);
    assert( root->right);
    llvm::Value* lhs = compile_expr( root->left);
    llvm::Value* rhs = compile_expr( root->right);

    switch ( ast_keyword( root) ) {
    case AST_ADD:
        return builder_->CreateAdd( lhs, rhs);
    case AST_SUB:
        return builder_->CreateSub( lhs, rhs);
    case AST_MUL:
        return builder_->CreateMul( lhs, rhs);
    case AST_DIV:
        return builder_->CreateSDiv( lhs, rhs);
    case AST_NOT:
    case AST_AND:
    case AST_OR:
        return builder_->CreateAdd( lhs, rhs);
    case AST_EQUAL:
    case AST_NEQUAL:
    case AST_GREAT:
    case AST_LOW:
    case AST_GEQUAL:
    case AST_LEQUAL:
        return builder_->CreateAdd( lhs, rhs);
    default:
        return nullptr;
    }
}

llvm::Value*
IRGenerator::compile_if( const ast_node* root)
{ assert( 0 );}

llvm::Value*
IRGenerator::compile_while( const ast_node* root)
{ assert( 0 );}

llvm::Value*
IRGenerator::compile_return( const ast_node* root)
{ assert( 0 );}



