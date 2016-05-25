#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "assignment.h"


int main(int argc, const char **argv)
{
  double *M, t, cost, answer;
  size_t *result;
  size_t i, j, n;
  long fpos;
  long type;
  int quiet = 0, ok = 0, argi = 1, status;
  const char *test_file, *answer_file;
  char answer_name[16];
  FILE *tf, *af;

  if (argc > 1)
  {
    if (strcmp(argv[argi], "-q") == 0)
    {
      quiet = 1;
      ++argi;
    }
    if (argc >= argi + 2)
    {
      test_file = argv[argi];
      answer_file = argv[argi+1];
      ok = 1;
    }
  }

  if (!ok)
  {
    fprintf(stderr, "Usage: %s [-q] <test file> <answer file>\n", argv[0]);
    exit(1);
  }

  tf = fopen(test_file, "r");
  if (!tf)
  {
    fprintf(stderr, "Error: Couldn't open test case file '%s'\n", test_file);
    exit(1);
  }

  if (fscanf(tf, "%zu", &n) == 0)
  {
    fprintf(stderr, "Error: Couldn't read problem size in file '%s'\n", test_file);
    exit(1);
  }

  if (n > 32767)
  {
    /* This is partly to try to keep coverity happy */
    fprintf(stderr, "Error: problem too large (greater than 32767) in '%s'\n", test_file);
    exit(1);
  }

  M = (double*)calloc(n * n, sizeof(double));
  result = (size_t*)malloc(2 * n * sizeof(size_t));

  if (!quiet) fprintf(stdout, "Reading %zu x %zu\n", n, n);

  fpos = ftell(tf);
  type = fscanf(tf, "%zu %zu %lf%*[ ]%*[\n] %zu", &i, &j, &t, &i);
  fseek(tf, fpos, SEEK_SET);

  if (type == 3)
  {
    for (i = 0; i < n; ++i)
      for (j = 0; j < n; ++j)
        if (fscanf(tf, "%lf", M + i * n + j) == EOF)
        {
          fprintf(stderr, "Error: Couldn't parse test case file '%s'\n", test_file);
          exit(1);
        }
  }
  else
  {
    while (fscanf(tf, "%zu %zu %lf", &i, &j, &t) != EOF)
      M[(i-1) * n + (j-1)] = t;

    for (i = 0; i < n; ++i)
      for (j = 0; j < n; ++j)
        if (M[i * n + j] == 0.0)
          M[i * n + j] = DBL_MAX;
  }

  fclose(tf);

  af = fopen(answer_file, "r");
  if (!af)
  {
    fprintf(stderr, "Error: Couldn't open answer file '%s'\n", answer_file);
    exit(1);
  }

  ok = 0;
  if (fscanf(af, "%*[^\n]") != 0)
  {
    fprintf(stderr, "Error: Couldn't read answer file '%s'\n", answer_file);
    exit(1);
  }

  while (fscanf(af, "%11s %lf", answer_name, &answer) != EOF)
  {
    if (strncmp(answer_name, test_file + strlen(test_file) - strlen(answer_name) - 4, strlen(answer_name)) == 0)
    {
      ok = 1;
      break;
    }
  }

  fclose(af);

  if (!ok)
  {
    fprintf(stderr, "Error: Couldn't find answer for '%s' in answer file\n", test_file);
    exit(1);
  }

  if (!quiet) fprintf(stdout, "Solving\n");
  if ((status = assignment(n, M, result, &cost)) != 0)
  {
    if (status == ASSIGNMENT_PROBLEM_TOO_LARGE)
      fprintf(stderr, "Error: problem too large solving '%s'\n", test_file);
    else if (status == ASSIGNMENT_NOT_ENOUGH_MEMORY)
      fprintf(stderr, "Error: not enough memory solving '%s'\n", test_file);
    else
      fprintf(stderr, "Error: unknown error solving '%s'\n", test_file);
    exit(1);
  }

  if (!quiet)
  {
    fprintf(stdout, "Minimum Cost: %f\nAssignment:\n", cost);
    for (i = 0; i < n; ++i)
      fprintf(stdout, "%zu -> %zu\n", result[i*2], result[i*2+1]);
  }

  fprintf(stdout, "%s\n", fabs(cost - answer) < 1e-8 ? "PASS" : "FAIL");

  free(M);
  free(result);

  return fabs(cost - answer) > 1e-8;
}
