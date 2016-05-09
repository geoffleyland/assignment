#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "assignment.h"

static const double EPSILON = 1e-6;


/*--------------------------------------------------------------------------*/

/* All the memory we need */
typedef struct scratch
{
  size_t n;                 /* dimension of problem */
  size_t *result;           /* where to store the result (provided by caller) */
  double *W;                /* copy of cost matrix, which we'll modify */
  int *C;                   /* Cover matrix */
  int *internal_flow;       /* flows from rows to columns */
  int *ss_flow;             /* flows from source to rows (0 - n-1) and columns to sink (n - 2n-1) */
  int *labelled;            /* whether rows and columns have been labelled (numbering as above) */
  size_t *pred;             /* predecessor for each node */
} scratch;


static void create_scratch(size_t n, double *M, size_t *result, scratch *S)
{
  S->n = n;
  S->result = result;

  S->W = malloc(n * n * sizeof(double));
  memcpy(S->W, M, n * n * sizeof(double));

  S->C = malloc(n * n * sizeof(int));
  S->internal_flow = calloc(n * n, sizeof(int));
  S->ss_flow = calloc(2 * n, sizeof(int));
  S->labelled = malloc(2 * n * sizeof(int));
  S->pred = malloc(2 * n * sizeof(size_t));
}


static void free_scratch(scratch *S)
{
  free(S->pred);
  free(S->labelled);
  free(S->ss_flow);
  free(S->internal_flow);
  free(S->C);
  free(S->W);
}


/*--------------------------------------------------------------------------*/

/* Deal with floating point crudely - make things near zero, zero */
static double make_zero(double v)
{
  return fabs(v) < EPSILON ? 0.0 : v;
}


/*--------------------------------------------------------------------------*/

/* Make sure every row has a zero by subracting the minimal value in each
   row from the whole row */
static void zero_rows(scratch *S)
{
  size_t i;
  for (i = 0; i < S->n; ++i)
  {
    double min = DBL_MAX;
    size_t j;
    for (j = 0; j < S->n; ++j)
    {
      double w = S->W[i * S->n + j];
      min = w < min ? w : min;
    }
    if (min > 0.0)
      for (j = 0; j < S->n; ++j)
        S->W[i * S->n + j] = make_zero(S->W[i * S->n + j] - min);
  }
}


/* Make sure every column has a zero by subracting the minimal value in each
   column from the whole column */
static void zero_columns(scratch *S)
{
  size_t i;
  for (i = 0; i < S->n; ++i)
  {
    double min = DBL_MAX;
    size_t j;
    for (j = 0; j < S->n; ++j)
    {
      double w = S->W[j * S->n + i];
      min = w < min ? w : min;
    }
    if (min > 0.0)
      for (j = 0; j < S->n; ++j)
        S->W[j * S->n + i] = make_zero(S->W[j * S->n + i] - min);
  }
}


/* Create an initial assignment by using the first zero in each row, provided
   that that column hasn't already been used.
   It might be worth being a bit smarter here, I haven't really looked into it */
static void initialise_flow(scratch *S)
{
  size_t i, j;
  for (i = 0; i < S->n; ++i)
    for (j = 0; j < S->n; ++j)
      if (S->W[i * S->n + j] == 0 &&
          S->ss_flow[S->n + j] == 0)
      {
        S->internal_flow[i * S->n + j] = 1;
        S->ss_flow[i] = 1;
        S->ss_flow[S->n + j] = 1;
        break;
      }
}


/*--------------------------------------------------------------------------*/
/* Most of the work in the Hungarian Algorithm is finding a minimal cover
   of all the zeroes in the modified cost matrix.
   This is analogous to finding a maximim flow from rows to columns of the
   matrix where only row, column pairs with zero cost have an arc with a
   capacity of one between them.
   We find a maximum flow by repeatedly searching the network for flow
   augmenting paths and adding them.
   Since we're repeatedly searching, I choose to do a depth-first search for
   a flow augmenting path - we just want to find one that works.
   (Towards the end of the search, the paths get surprisingly complicated,
   so I wonder if there isn't a point where it's wise to switch from
   depth-first to breadth-first search)
*/


static size_t scan_left(size_t i, scratch *S);

static size_t scan_right(size_t j, scratch *S)
{
  size_t i, tail;
  if (S->ss_flow[S->n + j] == 0) /* we've found a path to the sink */
    return S->n + j;
  /* current node is on the right: search its column */
  for (i = 0; i < S->n; ++i)
  {
    if (S->W[i * S->n + j] == 0.0 && S->internal_flow[i * S->n + j] == 1 && !S->labelled[i])
    {
      S->labelled[i] = 1;
      S->pred[i] = S->n + j;
      if ((tail = scan_left(i, S)) != SIZE_MAX)
        return tail;
    }
  }
  return SIZE_MAX;
}


static size_t scan_left(size_t i, scratch *S)
{
  size_t j, tail;
  /* current node is on the left: search its row */
  for (j = 0; j < S->n; ++j)
  {
    if (S->W[i * S->n + j] == 0.0 && S->internal_flow[i * S->n + j] == 0 && !S->labelled[S->n + j])
    {
      S->labelled[S->n + j] = 1;
      S->pred[S->n + j] = i;
      if ((tail = scan_right(j, S)) != SIZE_MAX)
        return tail;
    }
  }
  return SIZE_MAX;
}


static int recover_path(size_t tail, scratch *S)
{
  if (tail != SIZE_MAX)
  {
    S->ss_flow[tail] = 1;
    while (S->pred[tail] != SIZE_MAX)
    {
      size_t p = S->pred[tail];
      if (tail >= S->n)
        S->internal_flow[p * S->n + tail - S->n] = 1;
      else
        S->internal_flow[tail * S->n + p - S->n] = 0;
      tail = p;
    }
    S->ss_flow[tail] = 1;
    return -1;
  }
  else
  {
    /* We didn't find a path - we're done.  Return cover and arcs with flow */
    size_t i, j, k, result_index = 0;
    int assigned = 0;
    for (i = 0; i < S->n; ++i)
    {
      for (j = 0; j < S->n; ++ j)
      {
        if (S->internal_flow[i * S->n + j] == 1)
        {
          S->result[result_index++] = i;
          S->result[result_index++] = j;

          if (S->labelled[i] == 0)
            for (k = 0; k < S->n; ++k) S->C[i * S->n + k] += 1;
          if (S->labelled[j + S->n] == 1)
            for (k = 0; k < S->n; ++k) S->C[k * S->n + j] += 1;
          assigned++;
        }
      }
    }
    return assigned;
  }
}


static int cover(scratch *S)
{
  size_t i, j, tail;
  int assigned;

  while (1)
  {
    tail = SIZE_MAX;
    memset(S->labelled, 0, 2 * S->n * sizeof(int));
    memset(S->pred, 0xFF, 2 * S->n * sizeof(size_t));

    /* label all the left-side nodes that have no flow and are connected to the right */
    for (i = 0; i < S->n; ++i)
    {
      if (S->ss_flow[i] == 0)
      {
        for (j = 0; j < S->n; ++j)
          if (S->W[i * S->n + j] == 0.0)
          {
            S->labelled[i] = 1;
            if ((tail = scan_left(i, S)) != SIZE_MAX)
              goto path_found;
            break;
          }
      }
    }

path_found:
    assigned = recover_path(tail, S);
    if (assigned != -1)
      return assigned;
  }
}


/*--------------------------------------------------------------------------*/

static void augment(scratch *S)
{
  size_t i, j;
  double min = DBL_MAX;
  for (i = 0; i < S->n; ++i)
    for (j = 0; j < S->n; ++j)
      if (S->C[i * S->n + j] == 0 && S->W[i * S->n + j] < min)
        min = S->W[i * S->n + j];

  for (i = 0; i < S->n; ++i)
    for (j = 0; j < S->n; ++j)
    {
      int c = S->C[i * S->n + j];
      if (c == 0)
        S->W[i * S->n + j] = make_zero(S->W[i * S->n + j] - min);
      else if (c == 2)
        S->W[i * S->n + j] += min;
    }
}


/*--------------------------------------------------------------------------*/

double assignment(size_t n, double *M, size_t *result)
{
  double cost = 0.0;
  size_t i;

  scratch S;
  create_scratch(n, M, result, &S);

  zero_rows(&S);
  zero_columns(&S);
  initialise_flow(&S);

  while (1)
  {
    memset(S.C, 0, n * n * sizeof(int));
    if (cover(&S) == n)
    {
      for (i = 0; i < n; ++i)
        cost += M[n * result[i*2] + result[i*2+1]];
      break;
    }
    augment(&S);
  }

  free_scratch(&S);

  return cost;
}


/*--------------------------------------------------------------------------*/
