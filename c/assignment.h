#pragma once
#ifndef _assignment_h_
#define _assignment_h_

#ifdef __cplusplus
extern "C" {
#endif

int assignment(size_t n, double *M, size_t *result, double *cost);

#ifdef __cplusplus
}
#endif

#define ASSIGNMENT_PROBLEM_TOO_LARGE -1
#define ASSIGNMENT_NOT_ENOUGH_MEMORY -2

#endif
