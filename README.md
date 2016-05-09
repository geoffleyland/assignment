# Assignment - A C implementation of the Hungarian algorithm for the Assignment problem

[![Build Status](https://travis-ci.org/geoffleyland/assignment?branch=master)](https://travis-ci.org/geoffleyland/assignment)

## 1. What?

This is a very vanilla implementation of the [hungarian algorithm](https://en.wikipedia.org/wiki/Hungarian_algorithm) in C.


## 2. Why?

I needed something to solve an assignment problem and I wanted to remember how it all worked.
Turns out that the full algorithm is not that well documented -
descriptions tend to give a detailed description of augmenting the cost matrix
and then say "and then find a minimal cover" without commenting on how to do
that.  And that's the hard part.


## 3. Alternatives

+ Just search.  There's plenty.