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

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;

namespace hw3 {

// helper function that helps to filter out the different terms between 2 lists
// of DocIDElementHeader and append ranks as necessary.
static list<DocIDElementHeader> FilterResults(
  const list<DocIDElementHeader>& first,
  const list<DocIDElementHeader>& second);

QueryProcessor::QueryProcessor(const list<string>& index_list, bool validate) {
  // Stash away a copy of the index list.
  index_list_ = index_list;
  array_len_ = index_list_.size();
  Verify333(array_len_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader* [array_len_];
  itr_array_ = new IndexTableReader* [array_len_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = index_list_.begin();
  for (int i = 0; i < array_len_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < array_len_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t doc_id;  // The document ID within the index file.
  int     rank;    // The rank of the result so far.
} IdxQueryResult;

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string>& query) const {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> final_result;

  for (int i = 0; i < array_len_; i++) {
    list<DocIDElementHeader> result_first;
    list<DocIDElementHeader> result_second;

    // get the instances of dtr and itr
    DocTableReader* dtr = dtr_array_[i];
    IndexTableReader* itr = itr_array_[i];

    // search if there is a match in this index file with the first
    // query word
    DocIDTableReader* ditr = itr->LookupWord(query[0]);
    if (ditr == NULL) {  // no match found, go to next
      continue;
    }

    result_first = ditr->GetDocIDList();
    delete ditr;

    // if match found, process the following query words
    for (int j = 1; j < (int) query.size(); j++) {
      ditr = itr->LookupWord(query[j]);
      if (ditr == NULL) {  // no match found, conclude the search
        result_first.clear();
        break;
      }

      result_second = ditr->GetDocIDList();
      delete ditr;

      // filter out the different results in two searches
      result_first = FilterResults(result_first, result_second);
      if (result_first.size() == 0) {
        break;
      }
    }

    // translate the results into QueryResult
    if (result_first.size() != 0) {
      string file_name;
      list<DocIDElementHeader>::iterator it;

      for (it = result_first.begin(); it != result_first.end(); it++) {
        // get the file name string
        Verify333(dtr->LookupDocID(it->doc_id, &file_name));

        // processing the QueryResult
        QueryProcessor::QueryResult qr;
        qr.document_name = file_name;
        qr.rank = it->num_positions;
        final_result.push_back(qr);
      }
    }
  }

  // Sort the final results.
  sort(final_result.begin(), final_result.end());
  return final_result;
}

static list<DocIDElementHeader> FilterResults(
  const list<DocIDElementHeader>& first,
  const list<DocIDElementHeader>& second) {
  
  list<DocIDElementHeader> result;
  list<DocIDElementHeader>::const_iterator it_first;
  list<DocIDElementHeader>::const_iterator it_second;

  for (it_first = first.begin(); it_first != first.end(); it_first++) {
    for (it_second = second.begin(); it_second != second.end(); it_second++) {
      if (it_first->doc_id == it_second->doc_id) {
        DocIDElementHeader header;
        header.doc_id = it_first->doc_id;
        header.num_positions = it_first->num_positions + it_second->num_positions;
        result.push_back(header);

        break;
      }
    }
  }

  return result;
}

}  // namespace hw3
