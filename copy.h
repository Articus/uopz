/*
  +----------------------------------------------------------------------+
  | uopz                                                                 |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2015                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <krakjoe@php.net>                                |
  +----------------------------------------------------------------------+
 */
#ifndef HAVE_UOPZ_COPY_H
#define HAVE_UOPZ_COPY_H

ZEND_EXTERN_MODULE_GLOBALS(uopz);

/* {{{ */
static HashTable* uopz_copy_statics(HashTable *old) {
	HashTable *statics = NULL;
	
	if (old) {
		ALLOC_HASHTABLE(statics);
		zend_hash_init(statics,
			zend_hash_num_elements(old),
			NULL, ZVAL_PTR_DTOR, 0);
		zend_hash_copy(
			statics, 
			old, (copy_ctor_func_t) zval_add_ref);
	}
	
	return statics;
} /* }}} */

/* {{{ */
static zend_string** uopz_copy_variables(zend_string **old, int end) {
	zend_string **variables = safe_emalloc(end, sizeof(zend_string*), 0);
	int it = 0;
	
	while (it < end) {
		variables[it] = zend_string_copy(old[it]);
		it++;
	}
	
	return variables;
} /* }}} */

/* {{{ */
static zend_try_catch_element* uopz_copy_try(zend_try_catch_element *old, int end) {	
	zend_try_catch_element *try_catch = safe_emalloc(end, sizeof(zend_try_catch_element), 0);
	
	memcpy(
		try_catch, 
		old,
		sizeof(zend_try_catch_element) * end);
	
	return try_catch;
} /* }}} */

#if PHP_VERSION_ID < 70100
/* {{{ */
static zend_brk_cont_element* uopz_copy_brk(zend_brk_cont_element *old, int end) {
	zend_brk_cont_element *brk_cont = safe_emalloc(end, sizeof(zend_brk_cont_element), 0);
	
	memcpy(
		brk_cont,
		old, 
		sizeof(zend_brk_cont_element) * end);
	
	return brk_cont;
} /* }}} */
#else
static zend_live_range* uopz_copy_live(zend_live_range *old, int end) {
	zend_live_range *range = safe_emalloc(end, sizeof(zend_live_range), 0);

	memcpy(
		range,
		old,
		sizeof(zend_live_range) * end);

	return range;
}
#endif

/* {{{ */
static zval* uopz_copy_literals(zval *old, int end) {
	zval *literals = (zval*) safe_emalloc(end, sizeof(zval), 0);
	int it = 0;

	memcpy(literals, old, sizeof(zval) * end);

	while (it < end) {
		zval_copy_ctor(&literals[it]);	
		it++;
	}
	
	return literals;
} /* }}} */

/* {{{ */
static zend_op* uopz_copy_opcodes(zend_op_array *op_array, zval *literals) {
	zval *literal;
	uint32_t it = 0;
	zend_op *copy = safe_emalloc(
		op_array->last, sizeof(zend_op), 0);

	memcpy(copy, op_array->opcodes, sizeof(zend_op) * op_array->last);

#if ZEND_USE_ABS_CONST_ADDR || ZEND_USE_ABS_JMP_ADDR
	zend_op *opline = copy;
	zend_op *end    = copy + op_array->last;

	for (; opline < end; opline++) {
#if ZEND_USE_ABS_CONST_ADDR
		if (opline->op1_type == IS_CONST)
			opline->op1.zv = (zval*)((char*)opline->op1.zv + ((char*)op_array->literals - (char*)literals));
		if (opline->op2_type == IS_CONST) 
			opline->op2.zv = (zval*)((char*)opline->op2.zv + ((char*)op_array->literals - (char*)literals));
#endif
#if ZEND_USE_ABS_JMP_ADDR
		if ((op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO) != 0) {
			switch (opline->opcode) {
				case ZEND_JMP:
				case ZEND_FAST_CALL:
				case ZEND_DECLARE_ANON_CLASS:
				case ZEND_DECLARE_ANON_INHERITED_CLASS:
					 opline->op1.jmp_addr = &copy[opline->op1.jmp_addr - op_array->opcodes];
				break;

				case ZEND_JMPZNZ:
				case ZEND_JMPZ:
				case ZEND_JMPNZ:
				case ZEND_JMPZ_EX:
				case ZEND_JMPNZ_EX:
				case ZEND_JMP_SET:
				case ZEND_COALESCE:
				case ZEND_NEW:
				case ZEND_FE_RESET_R:
				case ZEND_FE_RESET_RW:
				case ZEND_ASSERT_CHECK:
					opline->op2.jmp_addr = &copy[opline->op2.jmp_addr - op_array->opcodes];
				break;
			}
		}
#endif
	}
#endif

	return copy;
} /* }}} */

/* {{{ */
static zend_arg_info* uopz_copy_arginfo(zend_op_array *op_array, zend_arg_info *old, uint32_t end) {
	zend_arg_info *info;
	uint32_t it = 0;

	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		old--;
		end++;
	}

	if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
		end++;
	}

	info = safe_emalloc
		(end, sizeof(zend_arg_info), 0);
	memcpy(info, old, sizeof(zend_arg_info) * end);	

	while (it < end) {
		if (info[it].name)
			info[it].name = zend_string_copy(old[it].name);
		if (info[it].class_name)
			info[it].class_name = zend_string_copy(old[it].class_name);
		it++;
	}
	
	if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		info++;
	}
	
	return info;
} /* }}} */

static inline zend_function* uopz_copy_user_function(zend_function *function) {
	zend_function  *copy;	
	zend_op_array  *op_array;
	zend_string   **variables;
	zval           *literals;
	zend_arg_info  *arg_info;

	copy = (zend_function*) 
		zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	memcpy(copy, function, sizeof(zend_op_array));
	
	op_array = &copy->op_array;
	variables = op_array->vars;
	literals = op_array->literals;
	arg_info = op_array->arg_info;

	op_array->function_name = zend_string_dup(op_array->function_name, 1);
	op_array->refcount = emalloc(sizeof(uint32_t));
	(*op_array->refcount) = 1;

	if (op_array->fn_flags & ZEND_ACC_CLOSURE) {
		if (zend_hash_index_add_ptr(
			&UOPZ(closures), (zend_long) copy, copy)) {
			(*op_array->refcount)++;
			GC_REFCOUNT((zend_object*) op_array->prototype)++;
		}
	}

	if (op_array->doc_comment) {
		op_array->doc_comment = zend_string_copy(op_array->doc_comment);
	}

	if (op_array->literals) op_array->literals = uopz_copy_literals (literals, op_array->last_literal);

	op_array->opcodes = uopz_copy_opcodes(op_array, literals);

	if (op_array->arg_info) 	op_array->arg_info = uopz_copy_arginfo(op_array, arg_info, op_array->num_args);
#if PHP_VERSION_ID < 70100
	if (op_array->brk_cont_array) 	op_array->brk_cont_array = uopz_copy_brk(op_array->brk_cont_array, op_array->last_brk_cont);
#else
	if (op_array->live_range)		op_array->live_range = uopz_copy_live(op_array->live_range, op_array->last_live_range);
#endif
	if (op_array->try_catch_array)  op_array->try_catch_array = uopz_copy_try(op_array->try_catch_array, op_array->last_try_catch);
	if (op_array->vars) 		op_array->vars = uopz_copy_variables(variables, op_array->last_var);
	if (op_array->static_variables) op_array->static_variables = uopz_copy_statics(op_array->static_variables);

	return copy;
}

static inline zend_function* uopz_copy_internal_function(zend_function *function) {
	zend_internal_function *copy = 
		(zend_internal_function*) pemalloc(sizeof(zend_internal_function), 1);
	memcpy(copy, function, sizeof(zend_internal_function));
	zend_string_addref(copy->function_name);
	return (zend_function*) copy;
}

/* {{{ */
static zend_function* uopz_copy_function(zend_function *function) {
	zend_function *copy;
	if (function->type == ZEND_USER_FUNCTION) {
		copy = uopz_copy_user_function(function);
	} else {
		copy = uopz_copy_internal_function(function);
	}
	return copy;
} /* }}} */

/* {{{ */
static void uopz_copy_function_ctor(zval *bucket) {
	Z_PTR_P(bucket) = uopz_copy_function(Z_PTR_P(bucket));
} /* }}} */
#endif

