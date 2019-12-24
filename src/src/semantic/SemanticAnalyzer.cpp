#include "semantic/SemanticAnalyzer.hpp"
#include "AST/program.hpp"
#include "AST/declaration.hpp"
#include "AST/variable.hpp"
#include "AST/constant_value.hpp"
#include "AST/function.hpp"
#include "AST/compound_statement.hpp"
#include "AST/assignment.hpp"
#include "AST/print.hpp"
#include "AST/read.hpp"
#include "AST/variable_reference.hpp"
#include "AST/binary_operator.hpp"
#include "AST/unary_operator.hpp"
#include "AST/if.hpp"
#include "AST/while.hpp"
#include "AST/for.hpp"
#include "AST/return.hpp"
#include "AST/function_call.hpp"
#include "AST/symbol_entry.hpp"
#include "AST/symbol_table.hpp"

#include <iostream>
#include <stack>
#include <iomanip>
#include <cstdio>

extern char* arr_token[512];
extern string file_name;
extern int error_find;
extern vector<SymbolTableNode*> symbol_table_list;

SymbolTableNode* sem_table = nullptr;
SymbolTableNode* func_table = nullptr;
vector<SymbolTableNode*> for_lp_table;
vector<string> for_check_vec;
int for_check_p = 0, use_opd = 0;
int arr_ref_check = 0;
VariableInfo* first_type = nullptr;
VariableInfo* second_type = nullptr;
VariableInfo* tmp_type = nullptr;
vector<VariableInfo*> type_list;
string func_name = "", forloop_var = "";


VariableInfo* search_table(VariableInfo* target) {

    VariableInfo* find_type = nullptr;
    for(uint i = 0; i < sem_table->entries->size(); ++i) {
        if((*(sem_table->entries))[i]->sym_name == target->var_name && (*(sem_table->entries))[i]->decl_check == 1) {
            return (*(sem_table->entries))[i]->sym_type;
        }
    }
    if(func_table != nullptr) {
        for(uint i = 0; i < func_table->entries->size(); ++i) {
            if((*(func_table->entries))[i]->sym_name == target->var_name && (*(func_table->entries))[i]->decl_check == 1) {
                return (*(func_table->entries))[i]->sym_type;
            }
        }
    }
    for(uint i = 0; i < symbol_table_list[0]->entries->size(); ++i) {
        if((*symbol_table_list[0]->entries)[i]->sym_name == target->var_name && (*symbol_table_list[0]->entries)[i]->decl_check == 1) {
            return (*symbol_table_list[0]->entries)[i]->sym_type;
        }
    }
    return find_type;
}

string type_return(VariableInfo* info) {
    string s = "";
    switch(info->type) {
        case TYPE_INTEGER: 
            s += "integer"; break;
        case TYPE_REAL:    
            s += "real"; break;
        case TYPE_STRING:  
            s += "string"; break;
        case TYPE_BOOLEAN: 
            s += "boolean"; break;
        case TYPE_VOID: 
            s += "void"; break;
        default:           
            s += "something wrong"; break;
    }
    if(info->type_set == SET_ACCUMLATED) {
        VariableInfo* find_type = search_table(info);
        if(find_type->array_range.size() != info->array_range.size()) {
            s += " ";
            for(uint i = info->array_range.size(); i < find_type->array_range.size(); ++i){
                s += "[";
                s += to_string(find_type->array_range[i].end - find_type->array_range[i].start);
                s += "]";
            }
        }
    }
    return s;
}

void clear_tmp() {
    first_type = nullptr;
    second_type = nullptr;
    tmp_type = nullptr;
}

void space_arrow(int len) {
    error_find = 1;
    for(int i = 1; i < len + 4; ++i) {
        std::cerr << " ";
    }
    std::cerr << "^\n";
}

void SemanticAnalyzer::visit(ProgramNode *m) {

    // error detect
    if(m->program_name != file_name) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": program name must be the same as filename\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
    }

    sem_table = m->symbol_table_node;
    if (m->declaration_node_list != nullptr) {
        for(uint i = 0; i< m->declaration_node_list->size(); ++i) {
            (*(m->declaration_node_list))[i]->accept(*this);
        }
    }
    if (m->function_node_list != nullptr) {
        for(uint i=0; i< m->function_node_list->size(); i++){
            sem_table = m->symbol_table_node;
            (*(m->function_node_list))[i]->accept(*this);
        }
    }
    if (m->compound_statement_node != nullptr) {
        m->compound_statement_node->accept(*this);
    }

    // error detect
    if(m->program_name != m->end_name) {
        std::cerr << "<Error> Found in line " << m->end_line_number << ", column " << m->end_col_number << ": identifier at the end of program must be the same as identifier at the beginning of program\n";
        std::cerr << "    " << arr_token[m->end_line_number] << '\n';
        space_arrow(m->end_col_number);
    }
}

void SemanticAnalyzer::visit(DeclarationNode *m) {
    if (m->variables_node_list != nullptr) {
        for(uint i = 0; i < m->variables_node_list->size(); ++i) {
            (*(m->variables_node_list))[i]->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(VariableNode *m) {
    string name_len_check = m->variable_name; //check name length, bigger than 32 will ignore
    if(name_len_check.length() > 32) {
        name_len_check = name_len_check.assign(m->variable_name, 0, 32);
    }

    for(uint i = 0; i < sem_table->entries->size(); ++i) {
        if((*(sem_table->entries))[i]->sym_type->type_set == SET_ACCUMLATED) {
            for(uint j = 0; j < (*(sem_table->entries))[i]->sym_type->array_range.size(); ++j) {
                uint arr_start = (*(sem_table->entries))[i]->sym_type->array_range[j].start;
                uint arr_end = (*(sem_table->entries))[i]->sym_type->array_range[j].end;
                if(arr_start > arr_end) {
                    std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": '" << (*(sem_table->entries))[i]->sym_name << "' declared as an array with a lower bound greater or equal to upper bound\n";
                    std::cerr << "    " << arr_token[m->line_number] << '\n';
                    space_arrow(m->col_number);
                }
            }
        }
    }

    // for loop redeclare detect
    int check_redecl = 0;
    if(for_check_p == 1) {
        for(uint i = 0; i < for_check_vec.size(); ++i) {
            string tmp = for_check_vec[i];
            if(name_len_check == tmp) {
                check_redecl = 1;
                std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": symbol '" << name_len_check << "' is redeclared\n";
                std::cerr << "    " << arr_token[m->line_number] << '\n';
                space_arrow(m->col_number);
                for_check_p = 0;
                break;
            }
        }
        if(check_redecl == 0) {
            forloop_var = name_len_check;
            // (*sem_table->entries)[0]->decl_check = 1;
        }
    }

    // error detect
    for(uint i = 0; i < sem_table->entries->size(); ++i) {
        string tmp = (*sem_table->entries)[i]->sym_name;
        if(name_len_check == tmp && (*sem_table->entries)[i]->decl_check == 0) {
            (*sem_table->entries)[i]->decl_check = 1;
            break;
        } else if(name_len_check == tmp && (*sem_table->entries)[i]->decl_check == 1) { //redeclare
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": symbol '" << name_len_check << "' is redeclared\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
        }
    }
}

void SemanticAnalyzer::visit(ConstantValueNode *m) {
    if(arr_ref_check != 1) {
        m->constant_value->var_line = m->line_number;
        m->constant_value->var_col = m->col_number;
        type_list.push_back(m->constant_value);
        return ;
    }
    if(arr_ref_check == 1) {
        tmp_type = m->constant_value;
        tmp_type->var_line = m->line_number;
        tmp_type->var_col = m->col_number;
    }
}

void SemanticAnalyzer::visit(FunctionNode *m) {

    // error detect
    for(uint i = 0; i < sem_table->entries->size(); ++i) {
        string tmp = (*sem_table->entries)[i]->sym_name;
        if(m->function_name == tmp && (*sem_table->entries)[i]->decl_check == 0) {
            (*sem_table->entries)[i]->decl_check = 1;
            break;
        } else if(m->function_name == tmp && (*sem_table->entries)[i]->decl_check == 1) { //redeclare
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": symbol '" << m->function_name << "' is redeclared\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
        }
    }

    sem_table = m->symbol_table_node;
    func_table = m->symbol_table_node;
    if (m->parameters != nullptr) {
        for(uint i=0; i< m->parameters->size(); i++){
            (*(m->parameters))[i]->node->accept(*this);
        }
    }
    func_name = m->function_name;
    if (m->body != nullptr) {
        m->body->accept(*this);
    }

    func_table = nullptr;
    // error detect
    if(m->function_name != m->end_name) {
        std::cerr << "<Error> Found in line " << m->end_line_number << ", column " << m->end_col_number << ": identifier at the end of function must be the same as identifier at the beginning of function\n";
        std::cerr << "    " << arr_token[m->end_line_number] << '\n';
        space_arrow(m->end_col_number);
    }
    func_name = "";
}

void SemanticAnalyzer::visit(CompoundStatementNode *m) {

    sem_table = m->symbol_table_node;
    if (m->declaration_node_list != nullptr) {
        for(uint i = 0; i < m->declaration_node_list->size(); ++i) {
            (*(m->declaration_node_list))[i]->accept(*this);
        }
    }
    if (m->statement_node_list != nullptr) {
        for(uint i = 0; i < m->statement_node_list->size(); ++i) {
            sem_table = m->symbol_table_node;
            (*(m->statement_node_list))[i]->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(AssignmentNode *m) {

    if (m->variable_reference_node != nullptr) {
        m->variable_reference_node->accept(*this);
    }
    if(type_list.size() == 0) return;
    first_type = type_list.back();
    type_list.pop_back();
    if(first_type->type_set == SET_CONSTANT_LITERAL) {
        std::cerr << "<Error> Found in line " << first_type->var_line << ", column " << first_type->var_col << ": cannot assign to variable '" << first_type->var_name << "' which is a constant\n";
        std::cerr << "    " << arr_token[first_type->var_line] << '\n';
        space_arrow(first_type->var_col);
        clear_tmp();
        return;
    }
    if(first_type->type_set == SET_ACCUMLATED && first_type->array_range.size() != first_type->var_dim) {
        std::cerr << "<Error> Found in line " << first_type->var_line << ", column " << first_type->var_col << ": array assignment is not allowed\n";
        std::cerr << "    " << arr_token[first_type->var_line] << '\n';
        space_arrow(first_type->var_col);
        clear_tmp();
        return;
    }
    if(for_lp_table.size() != 0) {
        for(uint i = 0; i < for_check_vec.size(); ++i) {
            if(first_type->var_name == for_check_vec[i]) {
                std::cerr << "<Error> Found in line " << first_type->var_line << ", column " << first_type->var_col << ": the value of loop variable cannot be modified inside the loop\n";
                std::cerr << "    " << arr_token[first_type->var_line] << '\n';
                space_arrow(first_type->var_col);
                clear_tmp();
                return;
            }
        }
    }

    if (m->expression_node != nullptr) {
        m->expression_node->accept(*this);
    }
    if(type_list.size() == 0) return;
    second_type = type_list.back();
    type_list.pop_back();

    if(second_type->type_set == SET_ACCUMLATED && second_type->array_range.size() != second_type->var_dim) {
        std::cerr << "<Error> Found in line " << second_type->var_line << ", column " << second_type->var_col << ": array assignment is not allowed\n";
        std::cerr << "    " << arr_token[second_type->var_line] << '\n';
        space_arrow(second_type->var_col);
        clear_tmp();
        return;
    }  

    // error detect
    int type_con = 0;
    if(first_type != nullptr && second_type != nullptr) {
        if(first_type->type == TYPE_INTEGER || first_type->type == TYPE_REAL) {
            if(second_type->type == TYPE_INTEGER || second_type->type == TYPE_REAL) {
                type_con = 1;
            }
        }
        if(first_type->type != second_type->type && type_con == 0) {
            string first_ans = type_return(first_type), second_ans = type_return(second_type);
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": assigning to '";
            std::cerr << first_ans << "' from incompatible type '" << second_ans << "'\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            clear_tmp();
            return ; // undeclare not need to check arguements
        }
    }

    clear_tmp();
}

void SemanticAnalyzer::visit(PrintNode *m) {
    // cout << "    in print test \n";
    if (m->expression_node != nullptr) {
        m->expression_node->accept(*this);
    }
    if(type_list.size() != 0) {
        VariableInfo* right_type = nullptr;
        right_type = type_list.back();
        type_list.pop_back();
        string print_type_check = type_return(right_type);
        if(print_type_check != "integer" && print_type_check != "real" && print_type_check != "string" && print_type_check != "boolean") {
            std::cerr << "<Error> Found in line " << right_type->var_line << ", column " << right_type->var_col << ": variable reference of print statement must be scalar type\n";
            std::cerr << "    " << arr_token[right_type->var_line] << '\n';
            space_arrow(right_type->var_col);
            clear_tmp();
            right_type = nullptr;
            return;
        }
    }
}

void SemanticAnalyzer::visit(ReadNode *m) {
    if (m->variable_reference_node != nullptr) {
        m->variable_reference_node->accept(*this);
    }

    if(type_list.size() != 0) {
        VariableInfo* right_type = nullptr;
        right_type = type_list.back();
        type_list.pop_back();

        for(uint i = 0; i < for_check_vec.size(); ++i) {
            if(right_type->var_name == for_check_vec[i]) {
                std::cerr << "<Error> Found in line " << right_type->var_line << ", column " << right_type->var_col << ": the value of loop variable cannot be modified inside the loop\n";
                std::cerr << "    " << arr_token[right_type->var_line] << '\n';
                space_arrow(right_type->var_col);
                clear_tmp();
                right_type = nullptr;
                return;
            }
        }

        if(right_type->type_set == SET_CONSTANT_LITERAL) {
            std::cerr << "<Error> Found in line " << right_type->var_line << ", column " << right_type->var_col << ": variable reference of read statement cannot be a constant variable reference\n";
            std::cerr << "    " << arr_token[right_type->var_line] << '\n';
            space_arrow(right_type->var_col);
            clear_tmp();
            right_type = nullptr;
            return;
        }

        if(right_type->type_set == SET_ACCUMLATED && right_type->array_range.size() != right_type->var_dim) {
            std::cerr << "<Error> Found in line " << right_type->var_line << ", column " << right_type->var_col << ": variable reference of read statement must be scalar type\n";
            std::cerr << "    " << arr_token[right_type->var_line] << '\n';
            space_arrow(right_type->var_col);
            clear_tmp();
            right_type = nullptr;
            return;
        }
    }
}

void SemanticAnalyzer::visit(VariableReferenceNode *m) {

    // error detect
    VariableInfo* tmp_arr = new VariableInfo();
    int error_p = 0, dimension_cnt = 0;

    // start search global
    for(uint i = 0; i < symbol_table_list[0]->entries->size(); ++i) {
        string tmp = (*symbol_table_list[0]->entries)[i]->sym_name;
        if((*symbol_table_list[0]->entries)[i]->sym_kind == KIND_VAR || (*symbol_table_list[0]->entries)[i]->sym_kind == KIND_CONST) {
            if(m->variable_name == tmp && (*symbol_table_list[0]->entries)[i]->decl_check == 1) { // have declare
                tmp_arr->type_set = (*symbol_table_list[0]->entries)[i]->sym_type->type_set;
                tmp_arr->type = (*symbol_table_list[0]->entries)[i]->sym_type->type;
                tmp_arr->var_name = m->variable_name;
                tmp_arr->var_line = m->line_number;
                tmp_arr->var_col = m->col_number;
                error_p = 1;
                dimension_cnt = (*symbol_table_list[0]->entries)[i]->sym_type->array_range.size();
                tmp_arr->var_dim = dimension_cnt;
                break;
            }
        }
    }

    // second search func
    if(func_table != nullptr) {
        for(uint i = 0; i < func_table->entries->size(); ++i) {
            string tmp = (*func_table->entries)[i]->sym_name;
            if((*func_table->entries)[i]->sym_kind == KIND_PARAM) {
                if(m->variable_name == tmp && (*func_table->entries)[i]->decl_check == 1) { // have declare
                    tmp_arr->type_set = (*func_table->entries)[i]->sym_type->type_set;
                    tmp_arr->type = (*func_table->entries)[i]->sym_type->type;
                    tmp_arr->var_name = m->variable_name;
                    tmp_arr->var_line = m->line_number;
                    tmp_arr->var_col = m->col_number;
                    error_p = 1;
                    dimension_cnt = (*func_table->entries)[i]->sym_type->array_range.size();
                    tmp_arr->var_dim = dimension_cnt;
                    break;
                }
            }
        }
    }

    // third search for loop var
    if(for_lp_table.size() != 0) {
        for(uint i = 0; i < for_lp_table.size(); ++i) {
            string tmp = (*for_lp_table[0]->entries)[0]->sym_name;
            if(m->variable_name == tmp) { // have declare
                tmp_arr->type_set = (*for_lp_table[0]->entries)[0]->sym_type->type_set;
                tmp_arr->type = (*for_lp_table[0]->entries)[0]->sym_type->type;
                tmp_arr->var_name = m->variable_name;
                tmp_arr->var_line = m->line_number;
                tmp_arr->var_col = m->col_number;
                error_p = 1;
                dimension_cnt = (*for_lp_table[0]->entries)[0]->sym_type->array_range.size();
                tmp_arr->var_dim = dimension_cnt;
                break;
            }
        }
    }

    // finally search local
    for(uint i = 0; i < sem_table->entries->size(); ++i) {
        string tmp = (*sem_table->entries)[i]->sym_name;
        if((*sem_table->entries)[i]->sym_kind == KIND_VAR || (*sem_table->entries)[i]->sym_kind == KIND_CONST || (*sem_table->entries)[i]->sym_kind == KIND_PARAM || (*sem_table->entries)[i]->sym_kind == KIND_LP_VAR) {
            if(m->variable_name == tmp && (*sem_table->entries)[i]->decl_check == 1) { // have declare
                tmp_arr->type_set = (*sem_table->entries)[i]->sym_type->type_set;
                tmp_arr->type = (*sem_table->entries)[i]->sym_type->type;
                tmp_arr->var_name = m->variable_name;
                tmp_arr->var_line = m->line_number;
                tmp_arr->var_col = m->col_number;
                error_p = 1;
                dimension_cnt = (*sem_table->entries)[i]->sym_type->array_range.size();
                tmp_arr->var_dim = dimension_cnt;
                break;
            }
        }
    }
    if(error_p == 0) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": use of undeclared identifier '" << m->variable_name << "'\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        clear_tmp();
        return ; // undeclare not need to check arguements
    }
    if (m->expression_node_list != nullptr) {
        arr_ref_check = 1;
        for(uint i = 0; i < m->expression_node_list->size(); ++i) {
            (*(m->expression_node_list))[i]->accept(*this);
            if(tmp_type->type != TYPE_INTEGER) {
                std::cerr << "<Error> Found in line " << tmp_type->var_line << ", column " << tmp_type->var_col << ": index of array reference must be an integer\n";
                std::cerr << "    " << arr_token[tmp_type->var_line] << '\n';
                space_arrow(tmp_type->var_col);
                clear_tmp();
                return;
            }
            tmp_arr->array_range.push_back({1, 1});
            dimension_cnt--;
        }
        arr_ref_check = 0;
    }
    if(dimension_cnt < 0) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": there is an over array subscript\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        clear_tmp();
        return;
    }
    type_list.push_back(tmp_arr);
}

void SemanticAnalyzer::visit(BinaryOperatorNode *m) {
    string opt = "";
    switch(m->op) {
        case OP_PLUS: use_opd = 5; opt = "+"; break;
        case OP_MINUS: use_opd = 1; opt = "-"; break;
        case OP_MULTIPLY: use_opd = 1; opt = "*"; break;
        case OP_DIVIDE: use_opd = 1; opt = "/"; break;
        case OP_MOD: use_opd = 2; opt = "mod"; break;
        case OP_AND: use_opd = 3; opt = "and"; break;
        case OP_OR: use_opd = 3; opt = "or"; break;
        case OP_LESS: use_opd = 4; opt = "<"; break;
        case OP_LESS_OR_EQUAL: use_opd = 4; opt = "<="; break;
        case OP_EQUAL: use_opd = 4; opt = "="; break;
        case OP_GREATER: use_opd = 4; opt = ">"; break;
        case OP_GREATER_OR_EQUAL: use_opd = 4; opt = ">="; break;
        case OP_NOT_EQUAL: use_opd = 4; opt = "<>"; break;
        default: break;
    }

    int own_opt = use_opd, opr_cnt = type_list.size();
    // cout << type_list.size() << "      no\n";
    if (m->left_operand != nullptr) {
        m->left_operand->accept(*this);
        // opr_cnt++;
    }
    use_opd = own_opt;
    // cout << type_list.size() << "      left\n";
    if (m->right_operand != nullptr) {
        m->right_operand->accept(*this);
        // opr_cnt++;
    }
    // cout << type_list.size() << "      finish\n";
    use_opd = own_opt;
    if((type_list.size() - opr_cnt) < 2) {
        for(int i = 0; i < type_list.size() - opr_cnt; ++i) {
            type_list.pop_back();
        }
        use_opd = 0;
        return;
    }
    VariableInfo* left_op = nullptr;
    VariableInfo* right_op = nullptr;

    right_op = type_list.back();
    type_list.pop_back();
    left_op = type_list.back();
    type_list.pop_back();
    int str_plus = 0;
    string left_out = type_return(left_op), right_out = type_return(right_op);
    if(use_opd == 1 || use_opd == 5) {
        if( (left_out != "integer" && left_out != "real") || (right_out != "integer" && right_out != "real")) {
            if(use_opd == 5 && (left_out == "string" && right_out == "string")) {
                str_plus = 1;
            }
            if(str_plus == 0) {
                std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operands to binary operation '" << opt << "' ('";
                std::cerr << left_out << "' and '" << right_out << "')\n";
                std::cerr << "    " << arr_token[m->line_number] << '\n';
                space_arrow(m->col_number);
                use_opd = 0;
                str_plus = 0;
                return;
            }
        }
        if(left_op->type == TYPE_REAL || right_op->type == TYPE_REAL) {
            VariableInfo* tmp = new VariableInfo();
            tmp->type = TYPE_REAL;
            tmp->var_name = opt;
            tmp->var_line = m->line_number;
            tmp->var_col = m ->col_number;
            type_list.push_back(tmp);
        } else if(left_op->type == TYPE_STRING && right_op->type == TYPE_STRING && use_opd == 5) {
            VariableInfo* tmp = new VariableInfo();
            tmp->type = TYPE_STRING;
            tmp->var_name = opt;
            tmp->var_line = m->line_number;
            tmp->var_col = m ->col_number;
            type_list.push_back(tmp);
        } else {
            VariableInfo* tmp = new VariableInfo();
            tmp->type = TYPE_INTEGER;
            tmp->var_name = opt;
            tmp->var_line = m->line_number;
            tmp->var_col = m ->col_number;
            type_list.push_back(tmp);
        }
    } else if(use_opd == 2) {
        if(left_out != "integer" || right_out != "integer") {
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operands to binary operation '" << opt << "' ('";
            std::cerr << left_out << "' and '" << right_out << "')\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            use_opd = 0;
            str_plus = 0;
            return;
        }
        VariableInfo* tmp = new VariableInfo();
        tmp->type = TYPE_INTEGER;
        tmp->var_name = opt;
        tmp->var_line = m->line_number;
        tmp->var_col = m ->col_number;
        type_list.push_back(tmp);
    } else if(use_opd == 3) {
        if(left_out != "boolean" || right_out != "boolean") {
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operands to binary operation '" << opt << "' ('";
            std::cerr << left_out << "' and '" << right_out << "')\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            use_opd = 0;
            str_plus = 0;
            return;
        }
        VariableInfo* tmp = new VariableInfo();
        tmp->type = TYPE_BOOLEAN;
        tmp->var_name = opt;
        tmp->var_line = m->line_number;
        tmp->var_col = m ->col_number;
        type_list.push_back(tmp);
    } else if(use_opd == 4) {
        if((left_out != "integer" && left_out != "real") || (right_out != "integer" && right_out != "real")) {
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operands to binary operation '" << opt << "' ('";
            std::cerr << left_out << "' and '" << right_out << "')\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            use_opd = 0;
            str_plus = 0;
            return;
        }
        VariableInfo* tmp = new VariableInfo();
        tmp->type = TYPE_BOOLEAN;
        tmp->var_name = opt;
        tmp->var_line = m->line_number;
        tmp->var_col = m ->col_number;
        type_list.push_back(tmp);
    }
 
    use_opd = 0;
}

void SemanticAnalyzer::visit(UnaryOperatorNode *m) {

    // cout << "    in unary \n";
    string opt = "";
    switch(m->op) {
        case OP_MINUS: use_opd = 1; opt += "-"; break;
        case OP_NOT: use_opd = 3; opt += "not"; break;
        default: break;
    }

    int own_opt = use_opd, opr_cnt = type_list.size();
    if (m->operand != nullptr) {
        m->operand->accept(*this);
    }
    use_opd = own_opt;
    if((type_list.size() - opr_cnt) < 1) {
        use_opd = 0;
        return;
    }

    VariableInfo* left_op = nullptr;

    left_op = type_list.back();
    type_list.pop_back();

    string left_out = type_return(left_op);
    if(use_opd == 1) {
        if(left_out != "integer" && left_out != "real") {
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operand to unary operation '" << opt << "' ('";
            std::cerr << left_out << "')\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            use_opd = 0;
            return;
        }
        if(left_op->type == TYPE_REAL) {
            VariableInfo* tmp = new VariableInfo();
            tmp->type = TYPE_REAL;
            type_list.push_back(tmp);
        } else {
            VariableInfo* tmp = new VariableInfo();
            tmp->type = TYPE_INTEGER;
            type_list.push_back(tmp);
        }
    } else if(use_opd == 3) {
        if(left_out != "boolean") {
            std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": invalid operand to unary operation '" << opt << "' ('";
            std::cerr << left_out << "')\n";
            std::cerr << "    " << arr_token[m->line_number] << '\n';
            space_arrow(m->col_number);
            use_opd = 0;
            return;
        }
        VariableInfo* tmp = new VariableInfo();
        tmp->type = TYPE_BOOLEAN;
        type_list.push_back(tmp);
    }
    use_opd = 0;
    // cout << use_opd << "    asdf\n";
}

void SemanticAnalyzer::visit(IfNode *m) {
    
    if (m->condition != nullptr) {
        m->condition->accept(*this);
    }
    if(type_list.size() == 0) {
        return ;
    }
    // cout << type_list.size() << "      if\n";
    VariableInfo* if_condition_type = nullptr;
    if_condition_type = type_list.back();
    type_list.pop_back();

    if(if_condition_type->type != TYPE_BOOLEAN) {
        std::cerr << "<Error> Found in line " << if_condition_type->var_line << ", column " << if_condition_type->var_col << ": the expression of condition must be boolean type\n";
        std::cerr << "    " << arr_token[if_condition_type->var_line] << '\n';
        space_arrow(if_condition_type->var_col);
        clear_tmp();
        return;
    }


    if (m->body != nullptr) {
        for(uint i = 0; i < m->body->size(); ++i) {
            (*(m->body))[i]->accept(*this);
        }
    }

    if (m->body_of_else != nullptr) {
        for(uint i = 0; i < m->body_of_else->size(); ++i) {
            (*(m->body_of_else))[i]->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(WhileNode *m) {
    
    if (m->condition != nullptr) {
        m->condition->accept(*this);
    }

    if(type_list.size() == 0) {
        return ;
    }
    VariableInfo* while_condition_type = nullptr;
    while_condition_type = type_list.back();
    type_list.pop_back();

    if(while_condition_type->type != TYPE_BOOLEAN) {
        std::cerr << "<Error> Found in line " << while_condition_type->var_line << ", column " << while_condition_type->var_col << ": the expression of condition must be boolean type\n";
        std::cerr << "    " << arr_token[while_condition_type->var_line] << '\n';
        space_arrow(while_condition_type->var_col);
        clear_tmp();
        return;
    }

    if (m->body != nullptr) {
        for(uint i = 0; i < m->body->size(); ++i) {
            (*(m->body))[i]->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(ForNode *m) {

    // error detect
    sem_table = m->symbol_table_node;
    for_check_p = 1;
    if (m->loop_variable_declaration != nullptr) {
        m->loop_variable_declaration->accept(*this);
        if(for_check_p == 0) {
            return;
        }
    }
    if (m->initial_statement != nullptr) {
        m->initial_statement->accept(*this);
    }
    for_check_vec.push_back(forloop_var);
    forloop_var = "";
    for_check_p = 1;
    if (m->condition2 != nullptr) {
        m->condition2->accept(*this);
    }

    if (m->condition != nullptr) {
        m->condition->accept(*this);
    }

    VariableInfo* first_value = nullptr;
    VariableInfo* second_value = nullptr;

    second_value = type_list.back();
    type_list.pop_back();
    first_value = type_list.back();
    type_list.pop_back();

    if(first_value->int_literal > second_value->int_literal) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": the lower bound of iteration count must be smaller than or equal to the upper bound\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        for_check_vec.pop_back();
        return;
    }

    for_lp_table.push_back(m->symbol_table_node);
    if (m->body != nullptr) {
        for(uint i = 0; i < m->body->size(); ++i) {
            (*(m->body))[i]->accept(*this);
        }
    }
    for_lp_table.pop_back();
    for_check_vec.pop_back();
    for_check_p = 0;
}

void SemanticAnalyzer::visit(ReturnNode *m) {
    if (m->return_value != nullptr) {
        m->return_value->accept(*this);
    }
    VariableInfo* return_type = nullptr;
    return_type = type_list.back();
    type_list.pop_back();
    string ret_type = type_return(return_type), func_type = "";
    for(uint i = 0; i < symbol_table_list[0]->entries->size(); ++i) {
        string tmp = (*symbol_table_list[0]->entries)[i]->sym_name;
        if((*symbol_table_list[0]->entries)[i]->sym_kind == KIND_FUNC) {
            if(func_name == tmp && (*symbol_table_list[0]->entries)[i]->decl_check == 1) { // have declare
                func_type = type_return((*symbol_table_list[0]->entries)[i]->sym_type);
            }
        }
    }
    if(func_table == nullptr) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": program/procedure should not return a value\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        return ;
    }

    if(ret_type != func_type) {
        std::cerr << "<Error> Found in line " << return_type->var_line << ", column " << return_type->var_col << ": return '" << ret_type << "' from a function with return type '";
        std::cerr << func_type << "'\n";
        std::cerr << "    " << arr_token[return_type->var_line] << '\n';
        space_arrow(return_type->var_col);
        return ;
    }
}

void SemanticAnalyzer::visit(FunctionCallNode *m) {

    // cout << "    in func call test \n";
    // error detect
    int error_p = 0;
    vector<VariableInfo*> param_info;
    for(uint i = 0; i < symbol_table_list[0]->entries->size(); ++i) {
        string tmp = (*symbol_table_list[0]->entries)[i]->sym_name;
        if((*symbol_table_list[0]->entries)[i]->sym_kind == KIND_FUNC) {
            if(m->function_name == tmp && (*symbol_table_list[0]->entries)[i]->decl_check == 1) { // have declare
                second_type = (*symbol_table_list[0]->entries)[i]->sym_type;
                param_info = (*symbol_table_list[0]->entries)[i]->sym_attribute;
                error_p = 1;
                break;
            }
        }
    }
    if(error_p == 0) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": used of undeclared function '" << m->function_name << "'\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        return ; // undeclare not need to check arguements
    }

    // check parameters
    int param_cnt = 0;
    if (m->arguments != nullptr) {
        for(uint i = 0; i < m->arguments->size(); ++i) {
            (*(m->arguments))[i]->accept(*this);
            param_cnt++;
        }
    }

    if(param_cnt != param_info.size()) {
        std::cerr << "<Error> Found in line " << m->line_number << ", column " << m->col_number << ": too few/much arguments to function invocation\n";
        std::cerr << "    " << arr_token[m->line_number] << '\n';
        space_arrow(m->col_number);
        return ; // function call param need more
    }

    stack<VariableInfo*> func_param;
    for(uint i = 0; i < param_cnt; ++i) {
        VariableInfo* tmp = type_list.back();
        type_list.pop_back();
        func_param.push(tmp);
    }
    for(uint i = 0; i < param_cnt; ++i) {
        VariableInfo* tmp = func_param.top();
        func_param.pop();
        string call_param = type_return(tmp), acc_param = type_return(param_info[i]);
        if(call_param != acc_param) {
            std::cerr << "<Error> Found in line " << tmp->var_line << ", column " << tmp->var_col << ": incompatible types passing '" << call_param;
            std::cerr << "' to parameter of type '" << acc_param << "'\n";
            std::cerr << "    " << arr_token[tmp->var_line] << '\n';
            space_arrow(tmp->var_col);
            return ; // function call param type wrong
        }
    }
    // cout << use_opd << "     test \n";
    second_type->var_name = m->function_name;
    second_type->var_line = m->line_number;
    second_type->var_col = m->col_number;
    type_list.push_back(second_type);
    // if(use_opd != 0) {
    //     type_list.push_back(second_type);
    // }
}