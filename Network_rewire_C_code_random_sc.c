/*------------------------------------------------------------------------------*/
/* C code used to re-wire New Zealand dairy cattle movement 2010-2011.
This code can be distributed under MIT License.
----------------------------------------------------------------------------------
MIT License

Copyright (c) 2016 Arata Hidano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
-------------------------------------------------------------------------------------
Rewire steps.
/* 1 Get the outward stubs orderd by date
/* 2 Pick up a single distance for this outward stub. This distance is already estimated based on hurdle regression models.
/* 3 Pick up inward stubs that best match to the outward stubs, by distance and by date. First criteria is distance.
/* 4 Go through all eligible inward stubs and overwrite if better one appears.
/* 5 Get the best inward stubs and connect them.
/* 6 Delete this inward from the list.
/* 7 Store the created new movements.
/* 8 Go through all outward stubs until all find their matches. */

/*------------------------------------------------------------------------------*/


/* ########################################################################## */
/* C LIBRARIES TO INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

/* STRUCTURE DECLARATIONS----------------------------------------------------- */  
  struct stub_node {
      int farm_id;   
      int batch_type;
      struct stub_node *next_node;
   };   
/* ########################################################################## */
/* FUNCTION DEFINITIONS */
  
void read_farm_data(); 
void read_movement_data() ;
void fill_prob_array(); 
void add_node_to_day();
void visualize_list();
void count_batch_by_distance() ;
void distance_interval_data();

int write_freq_dis();
int comp_day();
int comp_random();
double calc_dis() ;

unsigned int rand_interval();

int write_rewired_data();
int remove_node_from_day();
int write_freq_data();


   
/* ########################################################################## */
/* MAIN PROGRAM */
int main(void){
       srand((unsigned)time(NULL));
/* 1. SPECIFY VARIABLES AND THE DATA STORAGE FOR THE OUTCOME------------------------*/
    
    /* 1. USER DEFINED VARIABLES ------------------------------------------------ */
      char FarmDataFile[] = "/C_run/Data/farm_short_discat.csv";// Read in farm information
      long int num_farms = 10233; 
      long int num_farm_var = 5;
      
      char MoveDataFile[] = "/C_run/Data/final_movement_data_2010_analysis_v6_1.csv"; // Read in batch information
      long int num_moves = 23443; 
      
      char DistanceIntervalFile[] = "/C_run/Data/DistanceIntervalFile.csv"; // Read in predicted distance information
      long int num_covs = 23443; // set the number of rows
      int num_covs_var = 1000; // set the number of columns
      int num_simu = 1000;
      
      int distant_cat, distance_cat_this_move;
      
      long int num_move_vars = 6;
      long int num_rewired_move_vars = 9; 
      
      /*Set the output files*/
      char RewiredDataFile[] = "/C_run/out/RewiredDataFile_baseline_v1.csv";
      char FreqDisFile[] = "/C_run/out/FreqDisFile_baseline_v1_all.csv";
      char FreqDisFile_calf[] = "/C_run/out/FreqDisFile_baseline_v1_calf.csv";
      char FreqDisFile_heifer[] = "/C_run/out/FreqDisFile_baseline_v1_heifer.csv";
      char FreqDisFile_adult[] = "/C_run/out/FreqDisFile_baseline_v1_adult.csv";
      char DCAfreqDataFile[] = "/C_run/out/DCAfreqDataFile_baseline_v1.csv";
      
      int src_testarea, des_testarea,test_area_comb;// src_testarea, des_testarea specify the DCA status for source and destination farm respectively.
      // 0 is MCA(Area4), 1 is STA(Area3), 2 is STB(Area2), 3 is STD(Area1b), and 4 is STT(Area1a). test_area_comb defines the unique 26 combinations of src and des DCA status
      
      long int i = 0;
      long int j = 0;
      long int h = 0;
      long int batch = 0 ; //counter for each batch in movement data
      long int count_iter = 0; // counter for iterations
      double src_x, src_y, des_x, des_y ;
      int max_dis = 0; // initialise the maximum distance, which will be overwritten soon by calculating the real data
      int dca_combination = 26; //25 combinations 5*5 and column[25] for batch that includes at least one unknown testarea
      
      int src_farm_id, des_farm_id,dis_src_des ;
      int num_days = 365 ;
      int current_day ;
      int error_range_day_movement = 7 ;//Erro Range of days that will be allowed for inward stubs.
      int search_day;
     
      
/*2. READ DATA AND PREPARE THE OUTCOME STORAGE-------------------------------------------------*/   
/* PREPARATION OF DATA AND OUTPUT FILE.
2.1 PREPARE THE FRAME OF PAIR-WISE DISTANCE MATRIX FOR ALL FARMS.
2.2 READ FARM DATA.
2.3 READ MOVEMENT DATA.
2.4 FILL IN DISTANCE MATRIX.
2.5 CREATE ARRAY OF DISTANCE THAT STORES DISTANCE FREQUENCY AND FILL BY 0.
    2.5.1 CREATE DATAFRAME FOR FREQUENCY BETWEEN EACH DISEASE CONTROL AREA
2.6 GENERATE THE DISTANCE FREQUENCY FOR THE OBSERVED DATA AND FILL THE FIRST COLUMN OF DISTANCE ARRAY.
    2.6.1 FILL OUT THE FREQUENCY OF BETWEEN AND WITHIN DCA MOVEMENT.
    2.6.2 CREATE AND READ IN THE PREDICTED DISTANCE FILES.
2.7 OPTIONAL: VISUALISE THE INWARD STUBS THAT ARE AVAILABLE On A GIVEN DAY.
2.8 OPTIONAL: CREATE DATAFRAME THAT STORES GENERATED REWIRED MOVEMENT. */


/*2.1Distance matrix*/
      int **dis_matrix = (int**)malloc(sizeof(int*)*num_farms);
      for(i = 0; i < num_farms; i++)
        {
            /*Then, for each row, specifies the required memory size for column*/
          dis_matrix[i] = (int*)malloc( sizeof(int) * num_farms);  
        }

      
/* 2.2  Read in Farm Data */
    /* First create array FarmData, which specifies the row memory size*/
      double **FarmData = (double**)malloc( sizeof(double *) * num_farms);
      for(i = 0; i < num_farms; i++)
        {
            /*Then, for each row, specifies the required memory size for column*/
          FarmData[i] = (double*)malloc( sizeof(double) * num_farm_var);  
        }
      read_farm_data(FarmDataFile, FarmData, num_farms); 
      
/*2.3 Read movement data*/
          double **MoveData = (double**)malloc( sizeof(double *) * num_moves);
          for(i = 0; i < num_moves; i++)
                {
                MoveData[i] = (double*)malloc( sizeof(double) * num_move_vars);  
                }
                read_movement_data(MoveDataFile, MoveData, num_moves); 
 
 /*2.4 FILL OUT THE DISTANCE MATRIX*/
 /* Note: The calculating speed can be improved because it's calculating the pair-wise distance twice*/
     for (i =0 ; i < num_farms; i++)
     {
         src_x = FarmData[i][1];
         src_y = FarmData[i][2];
         
         for (j = 0; j < num_farms; j++)
         {
             des_x = FarmData[j][1] ;
             des_y = FarmData[j][2] ;
             dis_matrix[i][j] = calc_dis(src_x, src_y, des_x, des_y) ;
             if (dis_matrix[i][j] > max_dis)
             {
             max_dis = dis_matrix[i][j];
             }
         }
     }
     printf("%d\n", max_dis) ; // max_dis is maximum possible distance between two farms in NZ
    // system("pause") ;
    
/*2.5 COUNT OUT THE DISTANCE AND SAVE THE COUNT IN DISTANCE ARRAY*/
      /*Distance array for all movements*/
                int **dis_array = (int**)malloc(sizeof(int*)*max_dis) ; //prepare the memory for the length of distance for all movements
                /*Distance array for calf*/
                int **dis_array_calf = (int**)malloc(sizeof(int*)*max_dis) ; //prepare the memory for the length of distance for calf movements
                /*Distance array for heifer*/
                int **dis_array_heifer = (int**)malloc(sizeof(int*)*max_dis) ; //prepare the memory for the length of distance for heifer movements
                /*Distance array for adult*/
                int **dis_array_adult = (int**)malloc(sizeof(int*)*max_dis) ; //prepare the memory for the length of distance for adult movements
                
                for (i =0; i < max_dis; i++) 
                {
                dis_array[i] = (int*)malloc(sizeof(int)*(num_simu+1)) ; 
                dis_array_calf[i] = (int*)malloc(sizeof(int)*(num_simu+1)) ;
                dis_array_heifer[i] = (int*)malloc(sizeof(int)*(num_simu+1)) ;
                dis_array_adult[i] = (int*)malloc(sizeof(int)*(num_simu+1)) ;
                 }
                 /*Fill distance array with 0*/
                 for (i = 0 ; i < max_dis; i++)
                 {
                 for (j=0; j < num_simu+1 ; j++)
                  {
                  dis_array[i][j] = 0 ;
                  dis_array_calf[i][j] = 0 ;
                  dis_array_heifer[i][j] = 0 ;
                  dis_array_adult[i][j] = 0 ;
                  }
                  } 
      
               
/*2.5.1 CREATE DATAFRAME FOR FREQUENCY BETWEEN EACH DISEASE CONTROL AREA.*/
          int **FreqTestArea = (int**)malloc(sizeof(int*) * (num_simu+1)) ;
          for (i = 0; i < num_simu+1; i++)
          {
              FreqTestArea[i] = (int*)malloc(sizeof(int) * dca_combination) ;
              }
          /*Fill testarea frequency array with 0*/
                 for (i = 0 ; i < num_simu+1; i++)
                 {
                 for (j=0; j <dca_combination  ; j++)
                  {
                  FreqTestArea[i][j] = 0 ;
                  }
                  } 
                
/*2.6 . Extract the distance from the distance matrix for observed movements*/
/*2.6.1 FILL OUT THE FREQUENCY OF BETWEEN AND WITHIN DCA MOVEMENT*/
                  for (i=0; i < num_moves; i++)
                   {
         src_farm_id = (int)MoveData[i][0] ; //get source farm id.
         des_farm_id = (int)MoveData[i][1] ; // get destination farm id
         dis_src_des = dis_matrix[src_farm_id][des_farm_id] ;
         
         dis_array[dis_src_des][0] = dis_array[dis_src_des][0] + 1 ; // counter +1
         
         //and fill the testarea combination
         src_testarea = (int)FarmData[src_farm_id][3] ;
		 des_testarea = (int)FarmData[des_farm_id][3] ;
       
       if (src_testarea != 99 && des_testarea !=99)
          {
          test_area_comb = (src_testarea)*5 + des_testarea;
          FreqTestArea[0][test_area_comb] = FreqTestArea[0][test_area_comb] + 1 ; //INCREASE THE COUNT OF GIVEN COMBINATION OF TEST AREA BY 1
          }
           else
           {
           FreqTestArea[0][dca_combination-1] = FreqTestArea[0][dca_combination-1] + 1 ; //If testarea of either src or des farm is unknown, then store at column dca_combination-1 (i.e. column 25).
           }
         }
                 printf("Making FreqTestArea done"); 
/*2.6.2 CREATE AND READ IN THE PREDICTED DISTANCE FILES*/
        int **CovPredData = (int**)malloc(sizeof(int*) * (num_covs)) ;
          for (i = 0; i < num_covs; i++)
          {
              CovPredData[i] = (int*)malloc(sizeof(int) * num_covs_var) ;
              }
              printf("Making cov dataframe done"); 
              distance_interval_data(DistanceIntervalFile, CovPredData, num_covs, num_simu) ;

         printf("First line of CovPredData is %d, %d", CovPredData[0][0],CovPredData[1][0]);  
               
         
/*2.7 OPTIONAL: VISUALISE THE INWARD STUBS THAT ARE AVAILABLE On A GIVEN DAY*/
          
         

          /* VISUALIZE ARRAY */
                // for(i =0 ; i < num_days; i++)
                // {
                // visualize_list( instubs_day, i)   ; 
                // }
                // printf("\n\n");
/*2.9 OPTIONAL: CREATE REWIRED MOVEMENTS*/
      //   int **RewiredMoveData = (int**)malloc( sizeof(int *) * num_moves);
       //   for(i = 0; i < num_moves; i++)
        //        {
         //       RewiredMoveData[i] = (int*)malloc( sizeof(int) * num_rewired_move_vars);  
          //      }


/*===============================================================================*/
     
/*3. REWIRE ALGORITHM===========================================*/

/* REWIRE STEPS.
3.1. START LOOP A - ITERATIONS OF REWIRING OF WHOLE STUBS.
3.2. START LOOP B - LOOP FOR EACH STUBS IN A GIVEN ITERATION.
3.3. IDENTIFY THE BEST INSTUBS FOR EACH OUTWARD STUB. 
 3.3.1. ON THE EXACT MOVEMENT DAY OF OUTWARD STUB.
 3.3.2. DAYS WITHIN RANGE.
3.4. STORE THE IDENTIFIED INSTUB INFORMATION.
3.5. DELETE THE IDENTIFIED INSTUBS FROM THE LINKED ARRAY LIST.
3.6. CREATE OUTPUT DATA.
     3.6.1 CALCULATE THE DISTANCE FREQUENCY.
     3.6.2 CALCULATE THE FREQUENCY OF BATCH BETWEEN EACH DISEASE CONTROL AREA.*/

     /* INITALISATION OF VARIABLES*/
     struct stub_node *find_node; // find_node is a pointer to a struct. Declare this before simulation iteration loop.
     struct stub_node *best_node;

     int randInt, selected_dis,dis_to_inward,dis_diff,batch_this_move,day_this_move,src_batch_type,cov_this_move,day_to_delete2 ;
     int randInt_prob;
     int shortest_dis;
      unsigned int range_min = 0;
      unsigned int prob_max = 1000;
      int min_diff = max_dis ; //maximum distance between two farms in NZ
       int array_ordered_day[error_range_day_movement];//in order for loop to be used for searching best farm +/- range days, make array of numbers that tells the order of searching
        int move_id_this_move ;
         int day_to_delete = day_this_move;
         
/* 3.1 Start Loop A - 1000 iterations*/
for (count_iter = 0 ; count_iter < num_simu; count_iter++) 

{
  	/* Attach a random number to each batch for random sorting*/
        for (i = 0; i < num_moves; i++)
        {
        	randInt = (int)rand_interval(range_min,(unsigned int)(num_moves));
		
        MoveData[i][num_move_vars] = randInt;
        }
  	/* Reorder the movement data*/
  	qsort(MoveData, num_moves, sizeof(MoveData[0]), comp_random);
  	
 /* MAKE ARRAY OF POINTERS TO STUB NODE STRUCTS*/
          struct stub_node *outstubs_day[num_days]; // a pointer to a struct
          for(i = 0; i < num_days; i++)
                {
                outstubs_day[i] = NULL;
                }

          /* POPULATE THE ARRAY THAT WILL BE lINKED BY POINTERS*/ 
          struct stub_node *new_node; // each farm struct is also a pointer to a struct   
          for (i=0; i < num_moves; i++)
          { 
                /* CREATE A NEW STRUCT FOR THE OUT STUB */
                new_node = (struct stub_node*)malloc(sizeof( struct stub_node )); 
                new_node -> farm_id = (int)MoveData[i][0] ; /*source farm*/
                new_node -> batch_type = (int)MoveData[i][3] ;
                new_node -> next_node = NULL;   
         
                current_day = (int)MoveData[i][2] ;
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_node_to_day(outstubs_day, current_day, new_node ) ;
                 
           } 
           printf("adding node done");


/* 3.2 START OF LOOP B - one loop is one outward stub*/
    for (batch = 0; batch < num_moves ; batch++) //batch is counter to count and look each batch from the top

    { 
        
	best_node = NULL; // Initialise the indicator if best node is found or not

        min_diff = 9999; // Initialise the minimum distance found between the outward and candidate inward to 9999, which is longer than possible between farm distance in NZ
        shortest_dis = 9999;
        int next_min_diff =9999;
 
        des_farm_id = (int)MoveData[batch][1]; 
        batch_this_move = (int)MoveData[batch][3]; //batch type of this batch
        day_this_move = (int)MoveData[batch][2] ; //day of movement
      move_id_this_move = (int)MoveData[batch][4] ; //id that links this batch to the predicted distance 
      des_testarea = (int)FarmData[des_farm_id][3] ; //testarea of destination farm
      selected_dis = CovPredData[move_id_this_move][count_iter] ; // get the predicted distance for this batch
      
     
/* 3.3 SEARCH FOR INWARD STUBS AS FOLLOWS.
      1. First check if there are any instubs on this day. If yes, check batch type, then their distance, get the best farm.
      2. If the identified distance difference is not 0 (i.e. there is possibility that other inward on other candidate days can be better), then move to the other days, only if this date has inward.*/
      find_node = outstubs_day[day_this_move]; //jump to the starting farm on a given day in outstub
      
     /*3.3.1. On the observed movement day*/    
  
      if (find_node != NULL) //if this date has inward farms
      {
       while(find_node != NULL)
       {
        if (find_node -> farm_id != des_farm_id)
        { //if the source farm and pointed farm are different; otherwise go to next farm
        src_batch_type = find_node -> batch_type; //Get the batch type of this inward.
        
        if (src_batch_type == batch_this_move) //Batch type should be the same.*****LOOP STARTS
        { //Loop same batch
           src_farm_id = find_node -> farm_id ; //Get the farm which is pointed now. 
           dis_to_inward = dis_matrix[src_farm_id][des_farm_id]; //Get the distance between source and destination.
         /*Calclulate the difference in distance between the chosen random value and dis_to_inward*/
           dis_diff = abs(selected_dis - dis_to_inward) ;
           /*If dis_diff is smaller than the current best distance, overwrite*/
           if (dis_diff < min_diff) 
              { 
              min_diff = dis_diff;
              best_node = find_node ; //store the pointer to this farm
              day_to_delete = day_this_move;
                 if (min_diff == 0)
                 {
                 break; 
                 } //Once the distance diffference reaches 0, stop searching anymore       
              }
       
             
          }//Loop same batch end.
          }//IF source!= pointed farm ends.
          find_node = find_node -> next_node ; //go to next node
        }//while loop ends
       }//If statement ends; Step 1 to search the best farm on the best day done.
       

      /*3.3.2 Days within the specified range of the observed movement day*/
       /*ONLY IF min_diff !=0*/
       
       if (min_diff != 0) // if min_diff is already 0 then no point of searching anymore
       {
          
              for (i = 0 ; i < error_range_day_movement; i++)
                {
               array_ordered_day[i] = day_this_move - (i+1);
               
                } //making array done.
        for (i = 0; i < error_range_day_movement; i++)
            {//search each day below
            search_day = array_ordered_day[i];
            if (search_day >=0 && search_day < num_days)
            {
               find_node = outstubs_day[search_day]; //jump to the starting farm on a given day in instub
               
                         if (find_node != NULL) //if this date has inward farms
                         {
                                  
                          while(find_node != NULL)
                          {
                          if (find_node -> farm_id != des_farm_id)
                          {
                          src_batch_type = find_node -> batch_type; //Get the batch type.
        
                                         if (src_batch_type == batch_this_move) //Batch type should be the same.*****LOOP STARTS
                                         { //Loop same batch
                                         src_farm_id = find_node -> farm_id ; //Get the farm which is pointed now. 
                                         dis_to_inward = dis_matrix[src_farm_id][des_farm_id];
                                         dis_diff = abs(selected_dis - dis_to_inward) ;
      
                                         if (dis_diff < min_diff) 
                                         { 
                                          min_diff = dis_diff;
                                           best_node = find_node ; //store the pointer to this farm
                                            day_to_delete = search_day; //overwrite the day from which the node is deleted.
                                                     if (min_diff == 0)
                                                     {
                                                      break; 
                                                      } //Once the distance diffference reaches 0, stop searching anymore       
                                         }

                                         }//Loop same batch end.
                             }//IF source farm != pointed farm ends.
                             find_node = find_node -> next_node ; //go to next node
                           
                           }//while loop ends
                          }//If (find_node!=NULL) ends
            }//If (search_day>=0 && search-day<= num_days) ends.

            }//search each day ends.
        }//Loop for Step2 to search other days ends.
         

 /* 3.4. STORE THE INSTUB DATA - MAKE SURE SAVE THESE DATA BEFORE DELETING THE NODE*/        
       if (best_node != NULL) // only if outward stubs found their partners
       {
       src_farm_id = best_node -> farm_id ;
       dis_src_des = dis_matrix[src_farm_id][des_farm_id];
       dis_array[dis_src_des][count_iter+1] = dis_array[dis_src_des][count_iter+1] + 1; //INCREASE THE DISTANCE COUNTER BY 1
       /* Age type specific counter for distance*/
       //calf
       if (batch_this_move==0)
       {
       dis_array_calf[dis_src_des][count_iter+1] = dis_array_calf[dis_src_des][count_iter+1] + 1; 
       }
       else if (batch_this_move==1)
       {
       //heifer
       dis_array_heifer[dis_src_des][count_iter+1] = dis_array_heifer[dis_src_des][count_iter+1] + 1;
       }
       else if (batch_this_move==2)
       {
       //adults
       dis_array_adult[dis_src_des][count_iter+1] = dis_array_adult[dis_src_des][count_iter+1] + 1;
       }
       
       src_testarea = (int)FarmData[src_farm_id][3] ;
       
       if (src_testarea != 99 && des_testarea !=99) // if neither source and destination DCA is known, calculate the follwoing to get a unique DCA combination indicator
          {
          test_area_comb = (src_testarea)*5 + des_testarea;
          FreqTestArea[count_iter+1][test_area_comb] = FreqTestArea[count_iter+1][test_area_comb] + 1 ; //INCREASE THE COUNTE OF GIVEN COMBINATION OF TEST AREA
          }
           else
           {
           FreqTestArea[count_iter+1][dca_combination-1] = FreqTestArea[count_iter+1][dca_combination-1] + 1 ; //If testarea of either src or des farm is unknown, then store at column dca_combination-1.
           }
           
       }
           
/* 3.5. DELETE IDENTIFIED STUBS FROM THE INSTUB LISTS*/
    find_node = outstubs_day[day_to_delete]; 
 
    if(find_node != NULL)
    {
       while(find_node != NULL)
          {
              if(find_node == best_node)
                   {
                   remove_node_from_day(outstubs_day, day_to_delete, find_node);
                   break;
                   }
              else
                  {
                 find_node = find_node -> next_node;  
                  }  
          }  
   }
   
   } //########################### LOOP B ENDS HERE.
   

       
      
      for (i = 0 ; i < num_days; i++)
      {
          visualize_list( outstubs_day, i);
          }
     printf("Iteration %d done", count_iter) ;
    
     
} 
  // write output files
       write_freq_dis(FreqDisFile,dis_array,max_dis, num_simu);
       write_freq_dis(FreqDisFile_calf,dis_array_calf,max_dis, num_simu);
       write_freq_dis(FreqDisFile_heifer,dis_array_heifer,max_dis, num_simu);
       write_freq_dis(FreqDisFile_adult,dis_array_adult,max_dis, num_simu);
       write_freq_data(DCAfreqDataFile,FreqTestArea,num_simu,dca_combination);
/*================================================================================*/
     
/* 4. CLEAR DYNAMICALLY ALLOCATED MEMORY*/
   /*Clear MoveData*/
   for(i = 0; i < num_moves; i++)
         { 
   free(MoveData[i]);
                     }
   free(MoveData);
   
   /*Clear FarmData and dis_matrix*/
   for(i = 0; i < num_farms; i++)
         { 
   free(FarmData[i]);
   free(dis_matrix[i]);
                     }
   free(FarmData);
   free(dis_matrix);
   
   /*Clear dis_array*/
   for(i=0; i < max_dis; i++)
   {
    free(dis_array[i]) ;
    free(dis_array_calf[i]) ;
    free(dis_array_heifer[i]) ;
    free(dis_array_adult[i]) ;
    }
    free(dis_array) ;
    free(dis_array_calf) ;
    free(dis_array_heifer) ;
    free(dis_array_adult) ;
   

 return(0);
 }
             
             
/* END OF MAIN PROGRAM*/
 
/* ########################################################################## */
/* FUNCTION CODE */

/* -------------------------------------------------------------------------- */
/* read_farm_data: READING AND PARSING CSV FARM LIST */
/* -------------------------------------------------------------------------- */
void read_farm_data(char FarmDataFile[], double **FarmData, long int num_farms)
{     
    /* OPEN INPUT FILE */
    FILE *Farms = fopen(FarmDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    long int line_num, island;
    double x_coord, y_coord, farm_id, testarea;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_farms; line_num++)
      { 
         fscanf(Farms, "%lf,%lf,%lf, %lf, %d", &farm_id, &x_coord, &y_coord, &testarea, &island );

         /* STORE VALUES IN FARM LIST */
             FarmData[line_num][0] = farm_id;
             FarmData[line_num][1] = x_coord;
             FarmData[line_num][2] = y_coord;
             FarmData[line_num][3] = testarea;
             FarmData[line_num][4] = island;
      }            
   
   /* CLOSE INPUT FILE */
   fclose(Farms);
   
} 
/* -------------------------------------------------------------------------- */

/*-----------------------------------------------------------------------------*/
/* read_movement_data: READING AND PARSING CSV Movement LIST.
/* -------------------------------------------------------------------------- */
void read_movement_data(char MoveDataFile[], double **MoveData, long int num_moves)
{     
    /* OPEN INPUT FILE */
    FILE *Moves = fopen(MoveDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    long int line_num;
    double src_farm, des_farm, day, batch_cat, move_id;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_moves; line_num++)
      { 
         fscanf(Moves, "%lf,%lf,%lf,%lf,%lf",&src_farm, &des_farm, &day,&batch_cat,&move_id);

         /* STORE VALUES IN FARM LIST */
             MoveData[line_num][0] = src_farm;
             MoveData[line_num][1] = des_farm;
             MoveData[line_num][2] = day;
             MoveData[line_num][3] = batch_cat;
             MoveData[line_num][4] = move_id;


      }            
   
   /* CLOSE INPUT FILE */
   fclose(Moves);
   
} 
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Read covariate pattern that associates predicted distance.
/* -------------------------------------------------------------------------- */
void distance_interval_data(char DistanceIntervalFile[], int **CovPredData, int num_covs, int num_simu)
{     
    /* OPEN INPUT FILE */
    FILE *DisInt = fopen(DistanceIntervalFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    int line_num, col_num, i;
    int **d = (int**)malloc(sizeof(int*) * (num_covs)) ;
          for (i = 0; i < num_covs; i++)
          {
              d[i] = (int*)malloc(sizeof(int) * num_simu) ;
              }
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_covs; line_num++)
      { 
       for(col_num = 0; col_num < num_simu; col_num++)
       {
         fscanf(DisInt, "%d,", &d[line_num][col_num]); // fscanf is not reading a whole line, but only one value??
          CovPredData[line_num][col_num] = d[line_num][col_num];
          
       }
       }
         
   /* CLOSE INPUT FILE */
   fclose(DisInt);
   free(d) ;
   
} 
/*-----------------------------------------------------------------------------*/



/*-----------------------------------------------------------------------------*/
/* Calculate distance between two points.
Input is double and output is int*/
/*-----------------------------------------------------------------------------*/
double calc_dis(double src_x, double src_y, double des_x, double des_y)
{
     int distance_x_y;
     distance_x_y = (int)(roundf(sqrt((src_x - des_x)*(src_x - des_x) + (src_y - des_y)*(src_y - des_y))/1000)) ;
     return(distance_x_y) ;
}

/*-----------------------------------------------------------------------------*/

/* -------------------------------------------------------------------------- */
/* Sorting function*/
/* -------------------------------------------------------------------------- */

   /*Sort by random number of movements*/
   int comp_random(const void* a, const void* b) 
       {
         double **p1 = (double**)a;
         double **p2 = (double**)b;
         double *arr1 = *p1;
         double *arr2 = *p2;

         /* SORT BY Batch_cat ASCENDING */
        
         int diff3 = arr1[3] - arr2[3];
         if (diff3) return diff3;
        /* SORT BY Day ASCENDING */
        int diff2 = arr1[2] - arr2[2];
        if (diff2) return diff2;
         int diff4 = arr1[12] - arr2[12];
         if (diff4) return diff4;

  }
/* -------------------------------------------------------------------------- */     

/* -------------------------------------------------------------------------- */
/* add_stub: ADD MOVEMENT TO FARM LIST */
/* -------------------------------------------------------------------------- */
void add_node_to_day(struct stub_node *day_list[], int day, struct stub_node *node_to_add )
{     

 struct stub_node *current_node1;
 current_node1 = day_list[day];
 if(current_node1 == NULL)
    {
        day_list[day] = node_to_add;
    }
 else
    {
       node_to_add -> next_node = current_node1;
       day_list[day] = node_to_add;
    }

}

/* -------------------------------------------------------------------------- */
/* VISUALIZE INFORMATION FROM LINKED LIST ----------------------------------- */
/* -------------------------------------------------------------------------- */
void visualize_list(struct stub_node *day_list[], int day)  
{

 
 struct stub_node *current_node1;
 current_node1 = day_list[day];
 if(current_node1 != NULL)
    {
       printf("Day %d: ", day );
       while(current_node1 != NULL)
          {
              printf("%d, ", current_node1 -> farm_id);
              current_node1 = current_node1 -> next_node; 
              
          }  
       printf("\n");
   }
 

}
/* -------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------*/
/* RANDOM NUMBER GENERATOR FUNCTION*/
/*-----------------------------------------------------------------------------*/
unsigned int rand_interval(unsigned int min, unsigned int max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;
    
    do
    { 
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

/*------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------- */
/* REMOVE A NODE FROM THE LIST */
/* -------------------------------------------------------------------------- */
int remove_node_from_day(struct stub_node *day_list[], int day, struct stub_node *node_to_remove)
{
  
  if (day_list[day] != NULL)
  {
    struct stub_node *prev_node1;
    struct stub_node *current_node1;
  
    prev_node1 = day_list[day];
    current_node1 = day_list[day];
    while(current_node1 != NULL)
      {
         if(current_node1 == node_to_remove)
           {
                if(current_node1 == day_list[day])
                 {
                    day_list[day] = current_node1 -> next_node;
                    free(current_node1);
                    return (0);
                 }
                else
                 {
                  prev_node1 -> next_node = current_node1 -> next_node;                   
                  free(current_node1);
                  return (0);

                 }
           }
         else
           {
              prev_node1 = current_node1;
              current_node1 = current_node1 -> next_node;  
           }
      }            

      return (0);
   }
   
   return (0);
}
/* -------------------------------------------------------------------------- */

/*-----------------------------------------------------------------------------*/
/*Export CSV file of the frequency of the distance*/
/*------------------------------------------------------------------------------*/
int write_freq_dis(char FreqDisFile[],int **dis_array, int max_dis, int num_simu)
{

	FILE *Freq = fopen(FreqDisFile,"w");
	int line_num, col_num;
	
	for (line_num = 0 ; line_num < max_dis; line_num ++)
	{
		for (col_num =0 ; col_num < num_simu+1; col_num++)
		{
		
	 fprintf(Freq,"%d,",dis_array[line_num][col_num]);
  }
  fprintf(Freq,"\n");
}
	fclose(Freq);
	return 0;
}
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/*Export CSV file of the rewired data*/
/*------------------------------------------------------------------------------*/
int write_rewired_data(char RewiredDataFile[],int **RewiredMoveData, int num_moves)
{

	FILE *Rewired = fopen(RewiredDataFile,"w");
	int line_num;
	
	for (line_num = 0 ; line_num < num_moves; line_num ++)
	{
	 fprintf(Rewired,"%d, %d, %d,%d,%d,%d\n",RewiredMoveData[line_num][0],RewiredMoveData[line_num][1],RewiredMoveData[line_num][2],RewiredMoveData[line_num][3],RewiredMoveData[line_num][5],RewiredMoveData[line_num][8] );
  }
	fclose(Rewired);
	return 0;
}
/* -------------------------------------------------------------------------- */

/*-----------------------------------------------------------------------------*/
/*Export CSV file of the DCA frequency data*/
/*------------------------------------------------------------------------------*/
int write_freq_data(char DCAfreqDataFile[],int **FreqTestArea, int num_simu, int dca_combination)
{

	FILE *DCAfreq = fopen(DCAfreqDataFile,"w");
	int line_num, col_num;
	
	for (line_num = 0 ; line_num < num_simu+1; line_num ++)
	{
		for (col_num = 0;col_num <dca_combination ; col_num++ )
		
		{
		
	 fprintf(DCAfreq,"%d,",FreqTestArea[line_num][col_num]);
  }
  fprintf(DCAfreq,"\n");
}
	fclose(DCAfreq);
	return 0;
}
/* -------------------------------------------------------------------------- */
