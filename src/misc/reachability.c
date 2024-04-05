/* -*- mode: C -*-  */
/* vim:set ts=4 sw=4 sts=4 et: */
/*
   IGraph library.
   Copyright (C) 2024  The igraph development team <igraph@igraph.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "igraph_adjlist.h"
#include "igraph_bitset_list.h"
#include "igraph_community.h"
#include "igraph_components.h"
#include "igraph_interface.h"
#include "igraph_reachability.h"

igraph_error_t igraph_reachability_directed(
    const igraph_t *graph,
    igraph_vector_int_t *membership,
    igraph_vector_int_t *csize,
    igraph_integer_t *no_of_components,
    igraph_bitset_list_t *reach)
{
    igraph_integer_t no_of_nodes;
    igraph_integer_t i, j, n;
    igraph_adjlist_t adjlist, dag;
    igraph_vector_int_t *dag_neighbours, *neighbours;
    igraph_bitset_t *from_bitset, *to_bitset;

    no_of_nodes = igraph_vcount(graph);

    IGRAPH_CHECK(igraph_connected_components(graph, membership, csize, no_of_components, IGRAPH_STRONG));

    IGRAPH_CHECK(igraph_adjlist_init(graph, &adjlist, IGRAPH_OUT, IGRAPH_LOOPS_ONCE, IGRAPH_MULTIPLE));
    IGRAPH_FINALLY(igraph_adjlist_destroy, &adjlist);

    IGRAPH_CHECK(igraph_bitset_list_resize(reach, *no_of_components));

    for (i = 0; i < *no_of_components; ++i)
    {
        IGRAPH_CHECK(igraph_bitset_resize(igraph_bitset_list_get_ptr(reach, i), no_of_nodes));
    }
    for (i = 0; i < no_of_nodes; ++i)
    {
        IGRAPH_BIT_SET(*igraph_bitset_list_get_ptr(reach, VECTOR(*membership)[i]), i);
    }

    IGRAPH_CHECK(igraph_adjlist_init_empty(&dag, *no_of_components));
    IGRAPH_FINALLY(igraph_adjlist_destroy, &dag);

    for (i = 0; i < no_of_nodes; ++i)
    {
        neighbours = igraph_adjlist_get(&adjlist, i);
        dag_neighbours = igraph_adjlist_get(&dag, VECTOR(*membership)[i]);
        n = igraph_vector_int_size(neighbours);
        for (j = 0; j < n; j++)
        {
            if (VECTOR(*membership)[i] != VECTOR(*membership)[VECTOR(*neighbours)[j]])
            {
                igraph_vector_int_push_back(dag_neighbours, VECTOR(*membership)[VECTOR(*neighbours)[j]]);
            }
        }
    }

    for (i = *no_of_components - 1; i >= 0; --i)
    {
        dag_neighbours = igraph_adjlist_get(&dag, i);
        from_bitset = igraph_bitset_list_get_ptr(reach, i);
        n = igraph_vector_int_size(dag_neighbours);
        for (j = 0; j < n; j++)
        {
            to_bitset = igraph_bitset_list_get_ptr(reach, VECTOR(*dag_neighbours)[j]);
            igraph_bitset_or(from_bitset, from_bitset, to_bitset);
        }
    }

    igraph_adjlist_destroy(&adjlist);
    igraph_adjlist_destroy(&dag);
    IGRAPH_FINALLY_CLEAN(2);

    return IGRAPH_SUCCESS;
}

igraph_error_t igraph_count_reachable_directed(
    const igraph_t *graph,
    igraph_vector_int_t *counts)
{
    igraph_vector_int_t membership, csize;
    igraph_integer_t no_of_components, no_of_nodes = igraph_vcount(graph);
    igraph_bitset_list_t reach;

    IGRAPH_CHECK(igraph_vector_int_init(&membership, 0));
    IGRAPH_FINALLY(igraph_vector_int_destroy, &membership);

    IGRAPH_CHECK(igraph_vector_int_init(&csize, 0));
    IGRAPH_FINALLY(igraph_vector_int_destroy, &csize);

    IGRAPH_CHECK(igraph_bitset_list_init(&reach, 0));
    IGRAPH_FINALLY(igraph_bitset_list_destroy, &reach);

    IGRAPH_CHECK(igraph_reachability_directed(graph, &membership, &csize, &no_of_components, &reach));

    IGRAPH_CHECK(igraph_vector_int_resize(counts, igraph_vcount(graph)));
    for (igraph_integer_t i = 0; i < no_of_nodes; i++)
    {
        VECTOR(*counts)[i] = igraph_bitset_popcount(igraph_bitset_list_get_ptr(&reach, VECTOR(membership)[i]));
    }

    igraph_bitset_list_destroy(&reach);
    igraph_vector_int_destroy(&csize);
    igraph_vector_int_destroy(&membership);
    IGRAPH_FINALLY_CLEAN(3);

    return IGRAPH_SUCCESS;
}