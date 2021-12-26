//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predictor.h"
#include <math.h>

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

int global_history;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//




// Tournament


uint32_t* local_history_table;
uint32_t* local_prediction_table;
uint32_t* global_prediction_table;
uint32_t* choice_table;


// Custom predictor additional Data Structures
uint32_t* predictor_selection_table;
uint32_t* TAKEN_Table;
uint32_t* NOTTAKEN_Table;


uint32_t get_index(uint32_t value, uint32_t mask_num){
    uint32_t mask = (1<<mask_num) - 1;
    return mask & value;
}

uint8_t make_predictor_selection(uint32_t choice_table_entry){
    if (choice_table_entry > 1)
      return 1;
    else
      return 0;
}

uint8_t get_decision(uint32_t prediction){
    if (prediction >= WT ){
      return TAKEN;
    }
    else{
      return NOTTAKEN;
    }
}


void init_tournament_predictor(){

    global_history = 0;

    local_history_table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<pcIndexBits);

    local_prediction_table = (uint32_t*) malloc(sizeof(uint32_t)* 1<<lhistoryBits);
    memset(local_prediction_table, WN, sizeof(local_prediction_table));

    global_prediction_table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<ghistoryBits);
    memset(global_prediction_table, WN, sizeof(global_prediction_table));

    choice_table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<ghistoryBits);
    memset(choice_table, 2, sizeof(choice_table));

}

void init_custom_predictor(){

    global_history = 0;

    predictor_selection_table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<pcIndexBits);

    TAKEN_Table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<ghistoryBits);
    memset(TAKEN_Table, WT, sizeof(TAKEN_Table));

    NOTTAKEN_Table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<ghistoryBits);
    memset(NOTTAKEN_Table, WN, sizeof(NOTTAKEN_Table));

}


uint8_t predict_custom_predictor(uint32_t pc){


    uint32_t predictor_selection_table_index = get_index(pc, pcIndexBits);
    uint8_t selected_table = get_decision(predictor_selection_table[predictor_selection_table_index]);
    uint32_t table_index = get_index(pc^global_history, ghistoryBits);

    if(selected_table == TAKEN){
        return get_decision(TAKEN_Table[table_index]);
    }
    else{
        return get_decision(NOTTAKEN_Table[table_index]);
    }
}





uint8_t tournament_local_prediction(uint32_t pc){

    uint32_t pc_index = get_index(pc, pcIndexBits);
    uint32_t local_history_entry = local_history_table[pc_index];
    uint32_t local_prediction_index = get_index(local_history_entry, lhistoryBits);
    uint32_t local_prediction = local_prediction_table[local_prediction_index];

    return get_decision(local_prediction);

}

uint8_t tournament_global_prediction(){

    uint32_t global_prediction_index = get_index(global_history, ghistoryBits);
    uint32_t global_prediction = global_prediction_table[global_prediction_index];

    return get_decision(global_prediction);
    
}

uint8_t predict_tournament_predictor(uint32_t pc){

    uint32_t choice_table_index = get_index(global_history, ghistoryBits);

    uint32_t choice_table_entry = choice_table[choice_table_index];

    uint8_t predictor = make_predictor_selection(choice_table_entry);

    if(predictor == 0){
      // Local
      return tournament_local_prediction(pc);
    }

    else{
      // Global
      return tournament_global_prediction(pc);
    }

}


uint32_t get_new_prediction(uint32_t old_prediction, uint8_t outcome){

  switch(old_prediction){
    case SN:
      if(outcome == TAKEN)
        return WN;
    case WN:
      if(outcome == TAKEN)
        return WT;
      else
        return SN;
    case WT:
      if(outcome == TAKEN)
        return ST;
      else
        return WN;
    case ST:
      if(outcome == NOTTAKEN)
        return WT;

    default:
      break;
  }

}

void update_local_history_prediction_table(uint32_t pc, uint8_t outcome){

  uint32_t pc_index = get_index(pc, pcIndexBits);
  uint32_t local_history_entry = local_history_table[pc_index];
  uint32_t local_prediction_index = get_index(local_history_entry, lhistoryBits);
  uint32_t local_prediction = local_prediction_table[local_prediction_index];

  local_prediction_table[local_prediction_index] = get_new_prediction(local_prediction, outcome);
  local_history_table[local_history_entry] = (local_history_table[local_history_entry]<<1) | outcome;

}

void update_global_history_prediction_table(uint8_t outcome){

  uint32_t global_prediction_index = get_index(global_history, ghistoryBits);
  uint32_t global_prediction = global_prediction_table[global_prediction_index];
  
  global_prediction_table[global_prediction_index] = get_new_prediction(global_prediction, outcome);
  global_history = (global_history<<1) | outcome;

}

void train_tournament_predictor(uint32_t pc, uint8_t outcome){

  uint8_t global_pred = tournament_global_prediction();
  uint8_t local_pred = tournament_local_prediction(pc);
  
  if(global_pred != local_pred){
      uint32_t choice_table_index = get_index(global_history, ghistoryBits);
      uint32_t choice_table_entry = choice_table[choice_table_index];

      if(global_pred == outcome && choice_table[choice_table_index] < 3){
          choice_table[choice_table_index] += 1;
      }
      else if(local_pred == outcome && choice_table[choice_table_index] > 0){
          choice_table[choice_table_index] -= 1;
      }

  }

  update_local_history_prediction_table(pc, outcome);
  update_global_history_prediction_table(outcome);

}


void train_custom_predictor(uint32_t pc , uint8_t outcome){

  uint32_t predictor_selection_table_index = get_index(pc, pcIndexBits);
  uint32_t old_selection = predictor_selection_table[predictor_selection_table_index];
  uint32_t selected_table = get_decision(predictor_selection_table[predictor_selection_table_index]);
  uint32_t table_index = get_index(pc^global_history, ghistoryBits);
  

  predictor_selection_table[predictor_selection_table_index] = get_new_prediction(old_selection, outcome);

  if(selected_table == TAKEN){
      TAKEN_Table[table_index] = get_new_prediction(TAKEN_Table[table_index], outcome);
  }
  else{
      NOTTAKEN_Table[table_index] = get_new_prediction(NOTTAKEN_Table[table_index], outcome);
  }

  global_history = (global_history<<1) | outcome;
}



//GSHARE BEGIN ----- Harpreet's Implementation

uint32_t* Branch_History_Table;

void init_gshare_predictor(){

    Branch_History_Table = (uint32_t*) malloc(sizeof(uint32_t) * 1<<ghistoryBits);
    memset(Branch_History_Table, WN, sizeof(Branch_History_Table));

}

uint8_t predict_gshare_predictor(uint32_t pc){

    uint32_t bht_index = (pc^global_history) % (1<<ghistoryBits);
    return get_decision(Branch_History_Table[bht_index]);
}

void train_gshare_predictor(uint32_t pc, uint8_t outcome){

     uint32_t bht_index = (pc^global_history) % (1<<ghistoryBits);
     uint32_t old_pred = Branch_History_Table[bht_index];
     Branch_History_Table[bht_index] = get_new_prediction(old_pred, outcome);
     global_history = (global_history<<1) | outcome;
}

//GSHARE END



//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

void init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //

  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      init_gshare_predictor();
      return;
    case TOURNAMENT:
      init_tournament_predictor();
      return;
    case CUSTOM:
    init_custom_predictor();
      return;
    default:
      break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

uint8_t make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return predict_gshare_predictor(pc);
      break;
    case TOURNAMENT:
      return predict_tournament_predictor(pc);
    case CUSTOM:
      return predict_custom_predictor(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //

  switch (bpType) {
    case STATIC:
      return;
    case GSHARE:
      train_gshare_predictor(pc, outcome);
      break;
    case TOURNAMENT:
      train_tournament_predictor(pc, outcome);
      break;
    case CUSTOM:
      train_custom_predictor(pc, outcome);
      break;
    default:
      break;
  }



}
