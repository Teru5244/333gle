/*
 * Copyright ©2022 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc
static void Usage(void);
static char* InputString(FILE* f, size_t size); // for dynamically allocating when reading user inputs


//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char** argv) {
  if (argc != 2) {
    Usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.

  int res; // for checking crawler
  DocTable* dt;
  MemIndex* mi;
  char* input; // user input
  char* token;
  char* save_ptr; // for strtok_r
  int qlen; // query length
  int qsize; // query size, initially 1, increase if needed
  char** query;
  LinkedList* ll_res; // for search
  LLIterator* ll_it; // for printing SearchResults
  SearchResult* sr;

  printf("Indexing '%s'\n", argv[1]);

  // indexing
  res = CrawlFileTree(argv[1], &dt, &mi);
  if (!res) {
    fprintf(stderr, "Indexing failed.\n");
    Usage();
    return EXIT_FAILURE;
  }

  while (1) {
    printf("enter query:\n");
    // read inputs
    input = InputString(stdin, 16);

    if (input == NULL) { // out of memory for malloc
      fprintf(stderr, "Out of Memory.");
      return EXIT_FAILURE;
    }

    if (*input == '\0') { // ctrl D pressed, shut down
      free(input);
      DocTable_Free(dt);
      MemIndex_Free(mi);
      printf("shutting down...\n");
      return EXIT_SUCCESS;
    }

    // initialize the values
    qlen = 0;
    qsize = 1;
    query = (char**) malloc(sizeof(char*) * qsize);

    // get tokens for query array
    while (1) {
      token = strtok_r(input, " ", &save_ptr);
      if (token == NULL) {
        break;
      }

      query[qlen] = token;
      qlen++;

      // dynamically allocating query by 1 each time
      if (qlen == qsize) {
        query = realloc(query, sizeof(char*) * (qsize += 1));
      }

      input = NULL;
    }

    // do search
    ll_res = MemIndex_Search(mi, query, qlen);
    if (ll_res != NULL) {
      // initialize an iterator for printing the results
      ll_it = LLIterator_Allocate(ll_res);
      Verify333(ll_it != NULL);

      while (LLIterator_IsValid(ll_it)) {
        LLIterator_Get(ll_it, (LLPayload_t) &sr);
        printf("  %s (%u)\n", DocTable_GetDocName(dt, sr->doc_id), sr->rank);
        LLIterator_Next(ll_it);
      }
      LinkedList_Free(ll_res, &free);
      LLIterator_Free(ll_it);
    }
    free(query);
  }

  return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static char* InputString(FILE* f, size_t size) {
  char *str;
  int c;
  size_t len = 0;
  str = malloc(sizeof(*str)*size);
  if (!str) {
    return NULL;
  }
  // read by character and reallocate if needed
  while(EOF != (c = fgetc(f)) && c != '\n'){
    str[len++] = c;
    if(len == size){
      str = realloc(str, sizeof(*str)*(size += 16));
      if (!str) {
        return NULL;
      }
    }
  }
  str[len++] = '\0';

  return realloc(str, sizeof(*str)*len); // shrink the bytes allocated to fit the length of the string
}
