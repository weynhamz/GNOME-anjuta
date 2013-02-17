/*
 * anjuta-completion-test.c
 *
 * Copyright (C) 2013 - Carl-Anton Ingmarsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libanjuta/anjuta-completion.h>

static void
test_completion_basic (void)
{
    AnjutaCompletion* completion;
    GList* l;

    completion = anjuta_completion_new (NULL);
    anjuta_completion_add_item (completion, "a_a");
    anjuta_completion_add_item (completion, "a_b");
    anjuta_completion_add_item (completion, "a_c");
    anjuta_completion_add_item (completion, "a_d");

    /* Match everything */
    l = anjuta_completion_complete (completion, "a", -1);
    g_assert_cmpint (g_list_length (l), ==, 4);
    g_list_free (l);

    /* Match everything again */
    l = anjuta_completion_complete (completion, "a", -1);
    g_assert_cmpint (g_list_length (l), ==, 4);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "b", -1);
    g_assert_cmpint (g_list_length (l), ==, 0);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "a_", -1);
    g_assert_cmpint (g_list_length (l), ==, 4);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "a_a", -1);
    g_assert_cmpint (g_list_length (l), ==, 1);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "a_d", -1);
    g_assert_cmpint (g_list_length (l), ==, 1);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "a_e", -1);
    g_assert_cmpint (g_list_length (l), ==, 0);
    g_list_free (l);


    /* Test limiting the number of completions */
    l = anjuta_completion_complete (completion, "a_", 1);
    g_assert_cmpint (g_list_length (l), ==, 1);
    g_list_free (l);

    l = anjuta_completion_complete (completion, "a_", 3);
    g_assert_cmpint (g_list_length (l), ==, 3);
    g_list_free (l);

    g_object_unref (completion);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/completion/basic", test_completion_basic);

    return g_test_run ();
}
