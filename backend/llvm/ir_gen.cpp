#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <iostream>
#include "logs.h"
#include "ast/tree.h"
#include "ast/keyword.h"
#include "backend/llvm/ir_gen.h"
#include "asslib_support.h"

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
    llvm::FunctionCallee init_globals = module_->getOrInsertFunction( kGlobalsInitIdent, type);

    declare_stdlib();
    declare_functions( root);

    // Insert global scope
    scopes_.emplace_back();
    compile_stmt( root);
    scopes_.pop_back();

    assert( scopes_.size() == 0 );

    {
        std::string entry = "main";
        llvm::Function *main = module_->getFunction( entry);
        assert( main);

        llvm::BasicBlock* entry_bb = &main->getEntryBlock();
        llvm::BasicBlock* prepare_bb = llvm::BasicBlock::Create( context_, "__prepare", main, entry_bb);

        builder_->SetInsertPoint( prepare_bb);
        builder_->CreateCall( init_globals);
        builder_->CreateBr( entry_bb);

        llvm::Function *function = module_->getFunction( kGlobalsInitIdent);
        builder_->SetInsertPoint( &function->back());
        builder_->CreateRetVoid();
    }

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

    if ( root->left )
    {
        compile_stmt( root->left);
    }

    ast_node *stmt = root->right;
    dump_tree( stmt);
    switch ( ast_keyword( stmt) ) {
    case AST_ASSIGN:
        return compile_assign( stmt);
    case AST_DEFINE:
        return compile_define( stmt);
    case AST_IF:
        return compile_if( stmt);
    case AST_WHILE:
        return compile_while( stmt);
    case AST_CALL:
        return compile_call( stmt);
    case AST_OUT:
    {
        llvm::Function *function = module_->getFunction( get_asslib_ident( AsslibID::ASSLIB_PRINT));
        assert( function );
        return compile_call( function, root->right);
    }
    case AST_RETURN:
        return compile_return( stmt);
    default:
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

    return compile_call( function, root->right);

#if 0
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
#endif
}

llvm::Value*
IRGenerator::compile_call( llvm::Function* function, const ast_node* params)
{
    /* We need to check if function arguments number is correct */
    size_t n_params = 0;
    for ( const ast_node* param = params;
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
    for ( const ast_node* param = params;
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
            // @var = global [1 x i64], zeroinitializer
            just_allocated_value = new llvm::GlobalVariable{ *module_, type, false,
                                                             llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                             llvm::Constant::getNullValue( type), name};

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

    llvm::Value* rhs = compile_expr( root->right);
    llvm::Value* variable_ptr = get_element_ptr( name, shift);
    
    assert( rhs && variable_ptr );
    builder_->CreateStore( rhs, variable_ptr);

    return nullptr;
}

llvm::Value*
IRGenerator::compile_expr( const ast_node* root)
{
    assert( root );

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

    if ( ast_keyword( root) == AST_IN )
    {
        dump_tree( (ast_node*)root);
        llvm::Function *function = module_->getFunction( get_asslib_ident( AsslibID::ASSLIB_SCAN));

        assert( function );
        return compile_call( function, nullptr);
    }

    assert( root->right);
    llvm::Value* rhs = compile_expr( root->right);

    switch ( ast_keyword( root) ) {
    case AST_NOT:
        return builder_->CreateOr( rhs);
    default:
        break;
    }

    assert( root->left);
    llvm::Value* lhs = compile_expr( root->left);

    switch ( ast_keyword( root) ) {
    case AST_ADD:
        return builder_->CreateAdd( lhs, rhs);
    case AST_SUB:
        return builder_->CreateSub( lhs, rhs);
    case AST_MUL:
        return builder_->CreateMul( lhs, rhs);
    case AST_DIV:
        return builder_->CreateSDiv( lhs, rhs);
    case AST_AND:
        return builder_->CreateAnd( lhs, rhs);
    case AST_OR:
        return builder_->CreateOr( lhs, rhs);
    case AST_EQUAL:
        return builder_->CreateICmpEQ( lhs, rhs);
    case AST_NEQUAL:
        return builder_->CreateICmpNE( lhs, rhs);
    case AST_GREAT:
        return builder_->CreateICmpSGT( lhs, rhs);
    case AST_LOW:
        return builder_->CreateICmpSLT( lhs, rhs);
    case AST_GEQUAL:
        return builder_->CreateICmpSGE( lhs, rhs);
    case AST_LEQUAL:
        return builder_->CreateICmpSLE( lhs, rhs);
    default:
        return nullptr;
    }
}

llvm::Value*
IRGenerator::compile_cond( const ast_node* root)
{
    llvm::Value* cond = compile_expr( root);

    // If we have value in condition part we have to convert it to i1 manually
    llvm::Type* cond_type = cond->getType();
    if ( cond_type->isIntegerTy() && cond_type->getIntegerBitWidth() != 1 )
    {
        cond = builder_->CreateICmpEQ( cond, llvm::ConstantInt::get( cond_type, 0));
    }

    return cond;
}

llvm::Value*
IRGenerator::compile_if( const ast_node* root)
{
    assert( root && ast_keyword( root) == AST_IF );

    llvm::Value* cond = compile_cond( root->left);

    ast_node* decision = root->right;
    assert( decision && ast_keyword( decision) == AST_DECISN );
    assert( decision->left );

    llvm::Function* function = builder_->GetInsertBlock()->getParent();

    llvm::BasicBlock *then_bb = llvm::BasicBlock::Create( context_, "__if.then", function);
    llvm::BasicBlock *else_bb = llvm::BasicBlock::Create( context_, "__if.else", function);
    llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create( context_, "__if.exit", function);

    builder_->CreateCondBr( cond, then_bb, else_bb);

    if ( decision->right )
    {
        scopes_.emplace_back();
        builder_->SetInsertPoint( else_bb);
        compile_stmt( decision->right);
        scopes_.pop_back();

        builder_->CreateBr( exit_bb);
    }

    scopes_.emplace_back();
    builder_->SetInsertPoint( then_bb);
    compile_stmt( decision->left);
    scopes_.pop_back();

    builder_->CreateBr( exit_bb );
    builder_->SetInsertPoint( exit_bb);

    return nullptr;
}

llvm::Value*
IRGenerator::compile_while( const ast_node* root)
{
    assert( root );

    llvm::Function* function = builder_->GetInsertBlock()->getParent();

    llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create( context_, "__while.cond", function);
    llvm::BasicBlock *body_bb = llvm::BasicBlock::Create( context_, "__while.body", function);
    llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create( context_, "__while.exit", function);

    assert( root->left && root->right );

    builder_->CreateBr( cond_bb);

    builder_->SetInsertPoint( cond_bb);
    llvm::Value* cond = compile_cond( root->left);

    builder_->CreateCondBr( cond, body_bb, exit_bb);

    builder_->SetInsertPoint( body_bb);
    compile_stmt( root->right);
    builder_->CreateBr( cond_bb);

    builder_->SetInsertPoint( exit_bb);
    return nullptr;
}

llvm::Value*
IRGenerator::compile_return( const ast_node* root)
{
    assert( root->right );
    llvm::Value* return_value = compile_expr( root->right);
    builder_->CreateRet( return_value);
    return nullptr;
}

void
IRGenerator::declare_stdlib()
{
#define ASS_STDLIB( AST_ID, ID, NAME, ARGS )                                                                 \
{                                                                                                            \
    std::vector<llvm::Type*> params{};                                                                       \
    for ( int i = 0; i < (ARGS); ++i )                                                                       \
    {                                                                                                        \
        params.push_back( llvm::Type::getInt64Ty( context_));                                                \
    }                                                                                                        \
                                                                                                             \
    llvm::FunctionType *__type = llvm::FunctionType::get( llvm::Type::getInt64Ty( context_), params, false);  \
    module_->getOrInsertFunction( (NAME), __type);                                                           \
}

#include "../../STDLIB"
#undef ASS_STDLIB
}
