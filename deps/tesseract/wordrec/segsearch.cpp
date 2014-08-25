///////////////////////////////////////////////////////////////////////
// File:        segsearch.h
// Description: Segmentation search functions.
// Author:      Daria Antonova
// Created:     Mon Jun 23 11:26:43 PDT 2008
//
// (C) Copyright 2009, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////

#include "wordrec.h"

#include "associate.h"
#include "baseline.h"
#include "language_model.h"
#include "matrix.h"
#include "oldheap.h"
#include "params.h"
#include "ratngs.h"
#include "states.h"

ELISTIZE(SEG_SEARCH_PENDING);

namespace tesseract {

void Wordrec::SegSearch(CHUNKS_RECORD *chunks_record,
                        WERD_CHOICE *best_choice,
                        BLOB_CHOICE_LIST_VECTOR *best_char_choices,
                        WERD_CHOICE *raw_choice,
                        STATE *output_best_state,
                        BlamerBundle *blamer_bundle) {
  int row, col = 0;
  if (segsearch_debug_level > 0) {
    tprintf("Starting SegSearch on ratings matrix:\n");
    chunks_record->ratings->print(getDict().getUnicharset());
  }
  // Start with a fresh best_choice since rating adjustments
  // used by the chopper and the new segmentation search are not compatible.
  best_choice->set_rating(WERD_CHOICE::kBadRating);
  // TODO(antonova): Due to the fact that we currently do not re-start the
  // segmentation search from the best choice the chopper found, sometimes
  // the the segmentation search does not find the best path (that chopper
  // did discover) and does not have a chance to adapt to it. As soon as we
  // transition to using new-style language model penalties in the chopper
  // this issue will be resolved. But for how we are forced clear the
  // accumulator choices.
  //
  // Clear best choice accumulator (that is used for adaption), so that
  // choices adjusted by chopper do not interfere with the results from the
  // segmentation search.
  getDict().ClearBestChoiceAccum();

  MATRIX *ratings = chunks_record->ratings;
  // Priority queue containing pain points generated by the language model
  // The priority is set by the language model components, adjustments like
  // seam cost and width priority are factored into the priority.
  HEAP *pain_points = MakeHeap(segsearch_max_pain_points);

  // best_path_by_column records the lowest cost path found so far for each
  // column of the chunks_record->ratings matrix over all the rows.
  BestPathByColumn *best_path_by_column =
    new BestPathByColumn[ratings->dimension()];
  for (col = 0; col < ratings->dimension(); ++col) {
    best_path_by_column[col].avg_cost = WERD_CHOICE::kBadRating;
    best_path_by_column[col].best_vse = NULL;
  }

  // Compute scaling factor that will help us recover blob outline length
  // from classifier rating and certainty for the blob.
  float rating_cert_scale = -1.0 * getDict().certainty_scale / rating_scale;

  language_model_->InitForWord(prev_word_best_choice_,
                               assume_fixed_pitch_char_segment,
                               best_choice->certainty(),
                               segsearch_max_char_wh_ratio, rating_cert_scale,
                               pain_points, chunks_record, blamer_bundle,
                               wordrec_debug_blamer);

  MATRIX_COORD *pain_point;
  float pain_point_priority;
  BestChoiceBundle best_choice_bundle(
      output_best_state, best_choice, raw_choice, best_char_choices);

  // pending[i] stores a list of the parent/child pair of BLOB_CHOICE_LISTs,
  // where i is the column of the child. Initially all the classified entries
  // in the ratings matrix from column 0 (with parent NULL) are inserted into
  // pending[0]. As the language model state is updated, new child/parent
  // pairs are inserted into the lists. Next, the entries in pending[1] are
  // considered, and so on. It is important that during the update the
  // children are considered in the non-decreasing order of their column, since
  // this guarantees that all the parents would be up to date before an update
  // of a child is done.
  SEG_SEARCH_PENDING_LIST *pending =
    new SEG_SEARCH_PENDING_LIST[ratings->dimension()];

  // Search for the ratings matrix for the initial best path.
  for (row = 0; row < ratings->dimension(); ++row) {
    if (ratings->get(0, row) != NOT_CLASSIFIED) {
      pending[0].add_sorted(
          SEG_SEARCH_PENDING::compare, true,
          new SEG_SEARCH_PENDING(row, NULL, LanguageModel::kAllChangedFlag));
    }
  }
  UpdateSegSearchNodes(0, &pending, &best_path_by_column, chunks_record,
                       pain_points, &best_choice_bundle, blamer_bundle);

  // Keep trying to find a better path by fixing the "pain points".
  int num_futile_classifications = 0;
  STRING blamer_debug;
  while (!SegSearchDone(num_futile_classifications) ||
         (blamer_bundle != NULL &&
          blamer_bundle->segsearch_is_looking_for_blame)) {
    // Get the next valid "pain point".
    int pop;
    while (true) {
      pop = HeapPop(pain_points, &pain_point_priority, &pain_point);
      if (pop == -1) break;
      if (pain_point->Valid(*ratings) &&
        ratings->get(pain_point->col, pain_point->row) == NOT_CLASSIFIED) {
        break;
      } else {
        delete pain_point;
      }
    }
    if (pop == -1) {
      if (segsearch_debug_level > 0) tprintf("Pain points queue is empty\n");
      break;
    }
    ProcessSegSearchPainPoint(pain_point_priority, *pain_point,
                              best_choice_bundle.best_choice, &pending,
                              chunks_record, pain_points, blamer_bundle);

    UpdateSegSearchNodes(pain_point->col, &pending, &best_path_by_column,
                         chunks_record, pain_points, &best_choice_bundle,
                         blamer_bundle);
    if (!best_choice_bundle.updated) ++num_futile_classifications;

    if (segsearch_debug_level > 0) {
      tprintf("num_futile_classifications %d\n", num_futile_classifications);
    }

    best_choice_bundle.updated = false;  // reset updated
    delete pain_point;  // done using this pain point

    // See if it's time to terminate SegSearch or time for starting a guided
    // search for the true path to find the blame for the incorrect best_choice.
    if (SegSearchDone(num_futile_classifications) && blamer_bundle != NULL &&
        blamer_bundle->incorrect_result_reason == IRR_CORRECT &&
        !blamer_bundle->segsearch_is_looking_for_blame &&
        blamer_bundle->truth_has_char_boxes &&
        !ChoiceIsCorrect(getDict().getUnicharset(),
                         best_choice, blamer_bundle->truth_text)) {
      InitBlamerForSegSearch(best_choice_bundle.best_choice, chunks_record,
                             pain_points, blamer_bundle, &blamer_debug);
    }
  }  // end while loop exploring alternative paths
  FinishBlamerForSegSearch(best_choice_bundle.best_choice,
                           blamer_bundle, &blamer_debug);

  if (segsearch_debug_level > 0) {
    tprintf("Done with SegSearch (AcceptableChoiceFound: %d)\n",
            language_model_->AcceptableChoiceFound());
  }

  // Clean up.
  FreeHeapData(pain_points, MATRIX_COORD::Delete);
  delete[] best_path_by_column;
  delete[] pending;
  for (row = 0; row < ratings->dimension(); ++row) {
    for (col = 0; col <= row; ++col) {
      BLOB_CHOICE_LIST *rating = ratings->get(col, row);
      if (rating != NOT_CLASSIFIED) language_model_->DeleteState(rating);
    }
  }
}

void Wordrec::UpdateSegSearchNodes(
    int starting_col,
    SEG_SEARCH_PENDING_LIST *pending[],
    BestPathByColumn *best_path_by_column[],
    CHUNKS_RECORD *chunks_record,
    HEAP *pain_points,
    BestChoiceBundle *best_choice_bundle,
    BlamerBundle *blamer_bundle) {
  MATRIX *ratings = chunks_record->ratings;
  for (int col = starting_col; col < ratings->dimension(); ++col) {
    if (segsearch_debug_level > 0) {
      tprintf("\n\nUpdateSegSearchNodes: evaluate children in col=%d\n", col);
    }
    // Iterate over the pending list for this column.
    SEG_SEARCH_PENDING_LIST *pending_list = &((*pending)[col]);
    SEG_SEARCH_PENDING_IT pending_it(pending_list);
    GenericVector<int> non_empty_rows;
    while (!pending_it.empty()) {
      // Update language model state of this child+parent pair.
      SEG_SEARCH_PENDING *p = pending_it.extract();
      if (non_empty_rows.length() == 0 ||
          non_empty_rows[non_empty_rows.length()-1] != p->child_row) {
        non_empty_rows.push_back(p->child_row);
      }
      BLOB_CHOICE_LIST *current_node = ratings->get(col, p->child_row);
      LanguageModelFlagsType new_changed =
        language_model_->UpdateState(p->changed, col, p->child_row,
                                     current_node, p->parent, pain_points,
                                     best_path_by_column, chunks_record,
                                     best_choice_bundle, blamer_bundle);
      if (new_changed) {
        // Since the language model state of this entry changed, add all the
        // pairs with it as a parent and each of its children to pending, so
        // that the children are updated as well.
        int child_col = p->child_row + 1;
        for (int child_row = child_col;
             child_row < ratings->dimension(); ++child_row) {
          if (ratings->get(child_col, child_row) != NOT_CLASSIFIED) {
            SEG_SEARCH_PENDING *new_pending =
              new SEG_SEARCH_PENDING(child_row, current_node, 0);
            SEG_SEARCH_PENDING *actual_new_pending =
              reinterpret_cast<SEG_SEARCH_PENDING *>(
                  (*pending)[child_col].add_sorted_and_find(
                  SEG_SEARCH_PENDING::compare, true, new_pending));
            if (new_pending != actual_new_pending) delete new_pending;
            actual_new_pending->changed |= new_changed;
            if (segsearch_debug_level > 0) {
                  tprintf("Added child(col=%d row=%d) parent(col=%d row=%d)"
                          " changed=0x%x to pending\n", child_col,
                          actual_new_pending->child_row,
                          col, p->child_row, actual_new_pending->changed);
            }
          }
        }
      }  // end if new_changed
      delete p;  // clean up
      pending_it.forward();
    } // end while !pending_it.empty()
    language_model_->GeneratePainPointsFromColumn(
      col, non_empty_rows, best_choice_bundle->best_choice->certainty(),
      pain_points, best_path_by_column, chunks_record);
  }  // end for col

  if (best_choice_bundle->updated) {
    language_model_->GeneratePainPointsFromBestChoice(
        pain_points, chunks_record, best_choice_bundle);
  }

  language_model_->CleanUp();
}

void Wordrec::ProcessSegSearchPainPoint(float pain_point_priority,
                                        const MATRIX_COORD &pain_point,
                                        const WERD_CHOICE *best_choice,
                                        SEG_SEARCH_PENDING_LIST *pending[],
                                        CHUNKS_RECORD *chunks_record,
                                        HEAP *pain_points,
                                        BlamerBundle *blamer_bundle) {
  if (segsearch_debug_level > 0) {
    tprintf("Classifying pain point priority=%.4f, col=%d, row=%d\n",
            pain_point_priority, pain_point.col, pain_point.row);
  }
  MATRIX *ratings = chunks_record->ratings;
  BLOB_CHOICE_LIST *classified = classify_piece(
      chunks_record->chunks, chunks_record->word_res->denorm,
      chunks_record->splits,
      pain_point.col, pain_point.row, blamer_bundle);
  ratings->put(pain_point.col, pain_point.row, classified);

  if (segsearch_debug_level > 0) {
    print_ratings_list("Updated ratings matrix with a new entry:",
                       ratings->get(pain_point.col, pain_point.row),
                       getDict().getUnicharset());
    ratings->print(getDict().getUnicharset());
  }

  // Insert initial "pain points" to join the newly classified blob
  // with its left and right neighbors.
  if (!classified->empty()) {
    float worst_piece_cert;
    bool fragmented;
    if (pain_point.col > 0) {
      language_model_->GetWorstPieceCertainty(
          pain_point.col-1, pain_point.row, chunks_record->ratings,
          &worst_piece_cert, &fragmented);
      language_model_->GeneratePainPoint(
          pain_point.col-1, pain_point.row, false,
          LanguageModel::kInitialPainPointPriorityAdjustment,
          worst_piece_cert, fragmented, best_choice->certainty(),
          segsearch_max_char_wh_ratio, NULL, NULL,
          chunks_record, pain_points);
    }
    if (pain_point.row+1 < ratings->dimension()) {
      language_model_->GetWorstPieceCertainty(
          pain_point.col, pain_point.row+1, chunks_record->ratings,
          &worst_piece_cert, &fragmented);
      language_model_->GeneratePainPoint(
          pain_point.col, pain_point.row+1, true,
          LanguageModel::kInitialPainPointPriorityAdjustment,
          worst_piece_cert, fragmented, best_choice->certainty(),
          segsearch_max_char_wh_ratio, NULL, NULL,
          chunks_record, pain_points);
    }
  }

  // Record a pending entry with the pain_point and each of its parents.
  int parent_row = pain_point.col - 1;
  if (parent_row < 0) {  // this node has no parents
    (*pending)[pain_point.col].add_sorted(
        SEG_SEARCH_PENDING::compare, true,
        new SEG_SEARCH_PENDING(pain_point.row, NULL,
                               LanguageModel::kAllChangedFlag));
  } else {
    for (int parent_col = 0; parent_col < pain_point.col; ++parent_col) {
      if (ratings->get(parent_col, parent_row) != NOT_CLASSIFIED) {
        (*pending)[pain_point.col].add_sorted(
            SEG_SEARCH_PENDING::compare, true,
            new SEG_SEARCH_PENDING(pain_point.row,
                                   ratings->get(parent_col, parent_row),
                                   LanguageModel::kAllChangedFlag));
      }
    }
  }
}

void Wordrec::InitBlamerForSegSearch(const WERD_CHOICE *best_choice,
                                     CHUNKS_RECORD *chunks_record,
                                     HEAP *pain_points,
                                     BlamerBundle *blamer_bundle,
                                     STRING *blamer_debug) {
  blamer_bundle->segsearch_is_looking_for_blame = true;
  if (wordrec_debug_blamer) {
    tprintf("segsearch starting to look for blame\n");
  }
  // Clear pain points heap.
  int pop;
  float pain_point_priority;
  MATRIX_COORD *pain_point;
  while ((pop = HeapPop(pain_points, &pain_point_priority,
                         &pain_point)) != -1) {
    delete pain_point;
  }
  // Fill pain points for any unclassifed blob corresponding to the
  // correct segmentation state.
  *blamer_debug += "Correct segmentation:\n";
  for (int idx = 0;
        idx < blamer_bundle->correct_segmentation_cols.length(); ++idx) {
    blamer_debug->add_str_int(
        "col=", blamer_bundle->correct_segmentation_cols[idx]);
    blamer_debug->add_str_int(
        " row=", blamer_bundle->correct_segmentation_rows[idx]);
    *blamer_debug += "\n";
    if (chunks_record->ratings->get(
        blamer_bundle->correct_segmentation_cols[idx],
        blamer_bundle->correct_segmentation_rows[idx]) == NOT_CLASSIFIED) {
      if (!language_model_->GeneratePainPoint(
          blamer_bundle->correct_segmentation_cols[idx],
          blamer_bundle->correct_segmentation_rows[idx],
          false, -1.0, -1.0, false, -1.0, segsearch_max_char_wh_ratio,
          NULL, NULL, chunks_record, pain_points)) {
        blamer_bundle->segsearch_is_looking_for_blame = false;
        *blamer_debug += "\nFailed to insert pain point\n";
        blamer_bundle->SetBlame(IRR_SEGSEARCH_HEUR, *blamer_debug, best_choice,
                                wordrec_debug_blamer);
        break;
      }
    }
  }  // end for blamer_bundle->correct_segmentation_cols/rows
}

void Wordrec::FinishBlamerForSegSearch(const WERD_CHOICE *best_choice,
                                       BlamerBundle *blamer_bundle,
                                       STRING *blamer_debug) {
  // If we are still looking for blame (i.e. best_choice is incorrect, but a
  // path representing the correct segmentation could be constructed), we can
  // blame segmentation search pain point prioritization if the rating of the
  // path corresponding to the correct segmentation is better than that of
  // best_choice (i.e. language model would have done the correct thing, but
  // because of poor pain point prioritization the correct segmentation was
  // never explored). Otherwise we blame the tradeoff between the language model
  // and the classifier, since even after exploring the path corresponding to
  // the correct segmentation incorrect best_choice would have been chosen.
  // One special case when we blame the classifier instead is when best choice
  // is incorrect, but it is a dictionary word and it classifier's top choice.
  if (blamer_bundle != NULL && blamer_bundle->segsearch_is_looking_for_blame) {
    blamer_bundle->segsearch_is_looking_for_blame = false;
    if (blamer_bundle->best_choice_is_dict_and_top_choice) {
      *blamer_debug = "Best choice is: incorrect, top choice, dictionary word";
      *blamer_debug += " with permuter ";
      *blamer_debug += best_choice->permuter_name();
      blamer_bundle->SetBlame(IRR_CLASSIFIER, *blamer_debug, best_choice,
                              wordrec_debug_blamer);
    } else if (blamer_bundle->best_correctly_segmented_rating <
        best_choice->rating()) {
      *blamer_debug += "Correct segmentation state was not explored";
      blamer_bundle->SetBlame(IRR_SEGSEARCH_PP, *blamer_debug, best_choice,
                              wordrec_debug_blamer);
    } else {
      if (blamer_bundle->best_correctly_segmented_rating >=
          WERD_CHOICE::kBadRating) {
        *blamer_debug += "Correct segmentation paths were pruned by LM\n";
      } else {
        char debug_buffer[256];
        *blamer_debug += "Best correct segmentation rating ";
        sprintf(debug_buffer, "%g",
                blamer_bundle->best_correctly_segmented_rating);
        *blamer_debug += debug_buffer;
        *blamer_debug += " vs. best choice rating ";
        sprintf(debug_buffer, "%g", best_choice->rating());
        *blamer_debug += debug_buffer;
      }
      blamer_bundle->SetBlame(IRR_CLASS_LM_TRADEOFF, *blamer_debug, best_choice,
                              wordrec_debug_blamer);
    }
  }
}

}  // namespace tesseract
