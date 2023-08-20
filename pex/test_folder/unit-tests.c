// TODO

#include <stdarg.h>
#include <setjmp.h> 
#include <stddef.h>
#include "cmocka.h"
#include "../pe_common.h"
#include "../pe_exchange.h"
#include "../pe_trader.h"

// ! Test get_command first
static void test_index_of(void **state)
{
    int arr[] = {1,2,3,4,5};
    int res = index_of(arr, 5, 1);
    assert_int_equal(res, 0);
}

static void test_array_and(void **state)
{
    int arr[7] = {1, 0, 0, 0, 0, 0, 0};
    int res = array_and(arr, 7);
    assert_int_equal(res, 1);
    int arr1[7] = {0};
    int res1 = array_and(arr1, 7);
    assert_int_equal(res1, 0);
}

static void test_find_item(void **state)
{
    char *arr[5] = {"Hello", "World" , "ABC", "Stick", "Bugg"};
    int res = find_item(arr, 5,"Hello");
    assert_int_equal(res, 0);
    int invalid_res = find_item(arr, 5,"Rick");
    assert_int_equal(invalid_res, -1);
}

static void test_check_msg(void **state)
{
    char a[] = "Hello;";
    char to_find = ';';
    int res = check_msg(a, to_find);
    assert_int_equal(res, 1);
}

static void test_check_if(void **state)
{
    char a[] = "120";
    int res = check_if_integer(a);
    assert_int_equal(res, 1);
    char b[] = "abcd";
    int res1 = check_if_integer(b);
    assert_int_equal(res1, 0);
}

static void test_is_empty(void ** state)
{
    char a[] = "";
    int res = is_empty(a);
    assert_int_equal(res, 1);

    char b[] = "a";
    int res2 = is_empty(b);
    assert_int_equal(res2, 0);
}

static void test_valid_command(void ** state)
{
    char a[] = "a";
    int res = valid_command(a);
    assert_int_equal(res, 0);

    char b[] = "BUY";
    int res2 = valid_command(b);
    assert_int_equal(res2, 1);
}

static void test_valid_product_name_or_valid_string(void ** state)
{
    char a[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int res = valid_product_name(a); 
    assert_int_equal(res, 0);
    char b[] = "ABCDEFGHIJK";
    int res2 = valid_product_name(b);
    assert_int_equal(res2, 1);
    char c[] = "ABC";
    int res3 = valid_product_name(c);
    assert_int_equal(res3, 1);
}

static void test_valid_int(void ** state)
{
    int a = -1; 
    int res = valid_int(a);
    assert_int_equal(res, -1);

    int b = 999998; 
    int res2 = valid_int(b);
    assert_int_equal(res2, 1);
}

static void test_new_desc(void ** state)
{
    DESC * new_d = new_desc(100, 0, 1);
    assert_int_equal(new_d->trader_id, 100);
    assert_int_equal(new_d->order_id, 0);
    assert_int_equal(new_d->quantity, -1);
    assert_int_equal(new_d->time_priority, 1);
    assert_non_null(new_d);
    assert_null(new_d->next);
}

static void test_new_node(void ** state)
{
    NODE * n = new_node(100, 10, 1, NULL);
    assert_int_equal(n->price, 100);
    assert_int_equal(n->num, 10);
    assert_non_null(n);
    assert_int_equal(n->level, 1);
    assert_null(n->next);
    assert_null(n->adjacent);
}

static void test_new_item(void ** state)
{
    ITEM * i = new_item(0, 0);
    assert_non_null(i);
    assert_int_equal(i->buy_size, 0);
    assert_int_equal(i->sell_size, 0);
    assert_non_null(i->buys);
    assert_non_null(i->sells);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_index_of),
        cmocka_unit_test(test_array_and),
        cmocka_unit_test(test_find_item),
        cmocka_unit_test(test_check_msg),
        cmocka_unit_test(test_is_empty),
        cmocka_unit_test(test_valid_command),
        cmocka_unit_test(test_valid_product_name_or_valid_string),
        cmocka_unit_test(test_new_desc),
        cmocka_unit_test(test_new_node),
        cmocka_unit_test(test_new_item),


    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}