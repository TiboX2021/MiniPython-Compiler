#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "int_helpers.h"
#include "print_helpers.h"
#include "type_helpers.h"
#include "alloc_helpers.h"
#include "string_list_helpers.h"

// WARNING: static inline functions are abstracted away and cannot be called by other modules, like our asm code !
// Only the functions declared in this file can be called by our asm code.

// ************************************************** PRINTING ****************************************************** //

/** Print a dynamic value with a newline */
void println_dynamic(void *value)
{
    print_dynamic(value);
    putchar('\n');
}

// ************************************************ COMPUTATION ***************************************************** //

/** Compute the truthyness of a value */
static inline int is_truthy(void *value)
{
    switch (type_value(value))
    {
    case BOOL:
    case INT64:
    case STRING:
    case LIST:
        return (*((long long *)(value + 1 + 8))) != 0;
        break;
    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type for boolean valuation: '%s'\n", value_label(value));
        exit(1);
        break;
    }
}

/** Compute the equality of two values. If the types are not compatible, the program will exit with an error.
 */
static inline int is_equal(void *value1, void *value2)
{
    // compatible : all types

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        return *((long long *)(value1 + 1 + 8)) == *((long long *)(value2 + 1 + 8));
    case STRING_STRING:
    {
        return strcmp((char *)(value1 + 1 + 8 + 8), (char *)(value2 + 1 + 8 + 8)) == 0;
    }
    case LIST_LIST:
    {
        long long size1 = *((long long *)(value1 + 1 + 8));
        long long size2 = *((long long *)(value2 + 1 + 8));
        int result = 1;

        if (size1 != size2)
        {
            result = 0;
        }
        else
        {
            for (long long i = 0; i < size1; i++)
            {
                void *elem1 = *((void **)(value1 + 1 + 8 + 8 + i * 8));
                void *elem2 = *((void **)(value2 + 1 + 8 + 8 + i * 8));

                if (!is_equal(elem1, elem2))
                {
                    result = 0;
                    break;
                }
            }
        }
        return result;
    }

    case NONE_NONE:
        return 1;

    default:
        return 0;
    }
}

/** Compute the "<" operation for two values. If the types are not compatible, the program will exit with an error.
 */
static inline int is_lt(void *value1, void *value2)
{
    // compatible : int & bool
    // string & string
    // none is never compatible

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        return *((long long *)(value1 + 1 + 8)) < *((long long *)(value2 + 1 + 8));

    case STRING_STRING:
        return strcmp((char *)(value1 + 1 + 8 + 8), (char *)(value2 + 1 + 8 + 8)) < 0;

    case LIST_LIST:
    {
        long long size1 = *((long long *)(value1 + 1 + 8));
        long long size2 = *((long long *)(value2 + 1 + 8));
        long long min_size = size1 < size2 ? size1 : size2;
        int result = 0;

        for (long long i = 0; i < min_size; i++)
        {
            void *elem1 = *((void **)(value1 + 1 + 8 + 8 + i * 8));
            void *elem2 = *((void **)(value2 + 1 + 8 + 8 + i * 8));

            if (is_equal(elem1, elem2))
            {
                continue;
            }
            else if (is_lt(elem1, elem2))
            {
                result = 1;
                break;
            }
            else
            {
                result = 0;
                break;
            }
        }

        if (result == 0)
        {
            return size1 < size2;
        }

        return result;
    }

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for comparison (>, <, <=, =>): '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }
}

/** Set list[index] = value, with arguments all being dynamic */
void set_element(void *list, void *index, void *value)
{
    // 1. Get the list index
    void **index_ptr = list_index(list, index);

    // 3.
    // TODO: garbage collect the previous value

    // 4. Set the new value
    *index_ptr = value;

    // 5. Increment the reference count of the new value
    *((long long *)(value + 1)) += 1;
}

/** Get a list element and return it */
void *get_element(void *list, void *index)
{
    return *list_index(list, index);
}

/** Add two dynamic values. If the types are not compatible, the program will exit with an error.
 */
void *add_dynamic(void *value1, void *value2)
{
    // compatible : int & bool
    // string & string
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        result = allocate_int64();
        add_int_helper(value1, value2, result);
        break;

    case STRING_STRING:
    {
        result = add_string_helper(value1, value2);
        break;
    }
    case LIST_LIST:
    {
        result = add_list_helper(value1, value2);
        break;
    }

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for +: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }

    return result;
}

void *add_dynamic_temp_1(void *value1, void *value2)
{
    // compatible : int & bool
    // string & string
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        add_int_helper(value1, value2, value1);
        return value1;

    case STRING_STRING:
    {
        result = add_string_helper(value1, value2);
        free(value1);
        return result;
    }
    case LIST_LIST:
    {
        result = add_list_helper(value1, value2);
        free(value1);
        return result;
    }

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for +: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }
}

void *add_dynamic_temp_2(void *value1, void *value2)
{
    // compatible : int & bool
    // string & string
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        add_int_helper(value1, value2, value1);
        free(value2);
        return value1;

    case STRING_STRING:
    {
        result = add_string_helper(value1, value2);
        free(value1);
        free(value2);
        return result;
    }
    case LIST_LIST:
    {
        result = add_list_helper(value1, value2);
        free(value1);
        free(value2);
        return result;
    }

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for +: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }
}

/** Subtract two dynamic values. If the types are not compatible, the program will exit with an error.
 */
void *sub_dynamic(void *value1, void *value2)
{
    // compatible : int & bool
    // all other types are not compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        // TODO: detect if one of the input values is a temporary one, and reuse it
        // TODO: garbage collect the other input value if both are temporary
        result = allocate_int64();
        *((long long *)(result + 1 + 8)) = *((long long *)(value1 + 1 + 8)) - *((long long *)(value2 + 1 + 8));
        break;

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for -: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }

    return result;
}

/** Compare two dynamic values with the "<" operator. If the types are not compatible, the program will exit with an error.
 */
void *lt_dynamic(void *value1, void *value2)
{
    void *result = allocate_bool();
    *((long long *)(result + 1 + 8)) = is_lt(value1, value2);
    return result;
}

/** Compare two dynamic values with the ">" operator. If the types are not compatible, the program will exit with an error.
 */
void *gt_dynamic(void *value1, void *value2)
{
    return lt_dynamic(value2, value1);
}

/** Compare two dynamic values with the ">=" operator. If the types are not compatible, the program will exit with an error.
 */
void *ge_dynamic(void *value1, void *value2)
{
    void *result = lt_dynamic(value1, value2);
    *((long long *)(result + 1 + 8)) = !(*((long long *)(result + 1 + 8)));
    return result;
}

/** Compare two dynamic values with the "<=" operator. If the types are not compatible, the program will exit with an error.
 */
void *le_dynamic(void *value1, void *value2)
{
    return ge_dynamic(value2, value1);
}

/** Compute the negation of a value. If the type is incompatible, the program will exit with an error
 */
static inline void *neg_dynamic_helper(void *value, void *result)
{
    // Compatible: int and bool -> int
    // All others are not

    switch (type_value(value))
    {
    case INT64:
    case BOOL:
        *((long long *)(result + 1 + 8)) = -(*((long long *)(value + 1 + 8)));
        break;

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type for -: '%s'\n", value_label(value));
        exit(1);
        break;
    }
}

void *neg_dynamic(void *value)
{
    void *result;
    result = allocate_int64();
    neg_dynamic_helper(value, result);
    return result;
}

void *neg_dynamic_temp(void *value)
{
    neg_dynamic_helper(value, value);
    return value;
}

/** Multiply two dynamic values. If the types are not compatible, the program will exit with an error.
 */
void *mul_dynamic(void *value1, void *value2)
{
    // compatible : int & bool
    // string & int
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        // TODO: detect if one of the input values is a temporary one, and reuse it
        // TODO: garbage collect the other input value if both are temporary
        result = allocate_int64();
        *((long long *)(result + 1 + 8)) = *((long long *)(value1 + 1 + 8)) * *((long long *)(value2 + 1 + 8));
        break;

    case STRING_INT64:
    {
        // TODO: garbage collect the 2 operands. We allocate a new string.
        // Compute output size
        if (*((long long *)(value2 + 1 + 8)) > 0)
        {
            long long size = *((long long *)(value1 + 1 + 8)) * *((long long *)(value2 + 1 + 8));
            result = allocate_string(size);

            // Copy the first string
            strcpy((char *)(result + 1 + 8 + 8), (char *)(value1 + 1 + 8 + 8));

            for (long long i = 1; i < *((long long *)(value2 + 1 + 8)); i++)
            {
                strcat((char *)(result + 1 + 8 + 8), (char *)(value1 + 1 + 8 + 8));
            }
        }
        else
        {
            result = allocate_string(0);
        }

        break;
    }
    case INT64_STRING:
    {
        result = mul_dynamic(value2, value1);
        break;
    }
    case LIST_INT64:
    {
        // TODO: garbage collect the 2 operands. We allocate a new list.
        // Compute output size
        if (*((long long *)(value2 + 1 + 8)) > 0)
        {
            long long size = *((long long *)(value1 + 1 + 8)) * *((long long *)(value2 + 1 + 8));
            result = allocate_list(size);

            for (long long i = 0; i < *((long long *)(value2 + 1 + 8)); i++)
            {
                for (long long j = 0; j < *((long long *)(value1 + 1 + 8)); j++)
                {
                    *((void **)(result + 1 + 8 + 8 + i * *((long long *)(value1 + 1 + 8)) * 8 + j * 8)) = *((void **)(value1 + 1 + 8 + 8 + j * 8));
                    *((long long *)(*((void **)(result + 1 + 8 + 8 + i * *((long long *)(value1 + 1 + 8)) * 8 + j * 8)) + 1)) += 1;
                }
            }
        }
        else
        {
            result = allocate_list(0);
        }

        break;
    }
    case INT64_LIST:
    {
        result = mul_dynamic(value2, value1);
        break;
    }

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for x: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }

    return result;
}

/** Divide two dynamic values. If the types are not compatible, the program will exit with an error.
 */
void *div_dynamic(void *value1, void *value2)
{
    // compatible : int & bool
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        // TODO: detect if one of the input values is a temporary one, and reuse it
        // TODO: garbage collect the other input value if both are temporary
        if (*((long long *)(value2 + 1 + 8)) == 0)
        {
            printf("ZeroDivisionError: division by zero\n");
            exit(1);
        }
        result = allocate_int64();
        *((long long *)(result + 1 + 8)) = *((long long *)(value1 + 1 + 8)) / *((long long *)(value2 + 1 + 8));
        break;

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for //: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }

    return result;
}

/** Get the modulus of two dynamic values. If the types are not compatible, the program will exit with an error.
 */
void *mod_dynamic(void *value1, void *value2)
{
    // compatible : int & bool
    // none is never compatible

    void *result = NULL;

    switch (combined_type(value1, value2))
    {
    // Boolean and integer addition.
    case BOOL_BOOL:
    case BOOL_INT64:
    case INT64_BOOL:
    case INT64_INT64:
        // TODO: detect if one of the input values is a temporary one, and reuse it
        // TODO: garbage collect the other input value if both are temporary
        if (*((long long *)(value2 + 1 + 8)) == 0)
        {
            printf("ZeroDivisionError: modulo by zero\n");
            exit(1);
        }
        result = allocate_int64();
        *((long long *)(result + 1 + 8)) = *((long long *)(value1 + 1 + 8)) % *((long long *)(value2 + 1 + 8));
        break;

    default:
        // Default: unsupported types
        printf("TypeError: unsupported operand type(s) for %%: '%s' and '%s'\n", value_label(value1), value_label(value2));
        exit(1);
        break;
    }

    return result;
}

/** Compute the truthyness of a value. If the type is incompatible, the program will exit with an error */
void *truthy_dynamic(void *value)
{
    // Compatible: all types can be coerced to a truthy or falsy value
    void *result = allocate_bool();
    *((long long *)(result + 1 + 8)) = is_truthy(value);
    return result;
}

/** Compute the not operation for a value. If the type is incompatible, the program will exit with an error */
static inline void *not_dynamic_helper(void *value, void *result)
{
    *((long long *)(result + 1 + 8)) = !is_truthy(value);
    return result;
}

void *not_dynamic(void *value)
{
    void *result = allocate_bool();
    not_dynamic_helper(value, result);
    return result;
}

void *not_dynamic_temp(void *value)
{
    not_dynamic_helper(value, value);
    return value;
}

/** Compute the and operation for two values. If the types are incompatible, the program will exit with an error */
void *and_dynamic(void *value1, void *value2)
{
    void *result = allocate_bool();
    *((long long *)(result + 1 + 8)) = is_truthy(value1) && is_truthy(value2);
    return result;
}

/** Compute the or operation for two values. If the types are incompatible, the program will exit with an error */
void *or_dynamic(void *value1, void *value2)
{
    void *result = allocate_bool();
    *((long long *)(result + 1 + 8)) = is_truthy(value1) || is_truthy(value2);
    return result;
}

/** Compute the == operation for two values. If the types are incompatible, the program will exit with an error */
void *eq_dynamic(void *value1, void *value2)
{
    void *result = allocate_bool();
    *((long long *)(result + 1 + 8)) = is_equal(value1, value2);
    return result;
}

/** Compute the != operation for two values. If the types are incompatible, the program will exit with an error */
void *neq_dynamic(void *value1, void *value2)
{
    void *result = eq_dynamic(value1, value2);
    *((long long *)(result + 1 + 8)) = !(*((long long *)(result + 1 + 8)));
    return result;
}

/** Compute the length of a dynamic value. Only works for strings and lists */
void *len_dynamic(void *value)
{
    void *result = allocate_int64();

    switch (type_value(value))
    {
    case STRING:
    case LIST:
        *((long long *)(result + 1 + 8)) = *((long long *)(value + 1 + 8));
        break;

    default:
        // Default: unsupported types
        printf("TypeError: object of type '%s' has no len()\n", value_label(value));
        exit(1);
        break;
    }

    return result;
}

void *range_list(void *value)
{
    void *result = NULL;

    switch (type_value(value))
    {
    case INT64:
    case BOOL:
    {
        long long size = *((long long *)(value + 1 + 8));
        result = allocate_list(size);

        for (long long i = 0; i < size; i++)
        {
            void *elem = allocate_int64();
            *((long long *)(elem + 1 + 8)) = i;
            *((void **)(result + 1 + 8 + 8 + i * 8)) = elem;
        }
        break;
    }

    default:
        // Default: unsupported types
        printf("TypeError: range() argument must be int, not %s\n", value_label(value));
        exit(1);
        break;
    }

    return result;
}
