#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <ctime>

using namespace std;

#define SORT_RANGE 1000
#define PARTITION_SIZE 10

#define DATA_TAG 0
#define TERMINATE_TAG 1

void perform_master();
void perform_slave(int rank);

int main (int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  
  int rank = MPI::COMM_WORLD.Get_rank();
  std::srand(std::time(0));   // use current time as seed for random generator
  
  if(rank == 0){
    perform_master();
  }
  else{
    perform_slave(rank);
  }

  MPI_Finalize();
  return 0;
}

int * create_array(int size){
  int * list = new int[size];
  for(int i = 0; i < size; i++){
    list[i] = i;
  }
  return list;
}

void shuffle_array(int * list, int size){
  int placeholder;
  int index;
  
  for(int i = size; i > 0; i--){
    index = std::rand() % i;
    placeholder = list[index];
    list[index] = list[i - 1];
    list[i-1] = placeholder;
  }
}

bool check_array(int * list, int size){
  int last = list[0];
  
  for(int i = 1; i < size; i++){
    if(list[i] < last){
      return false;
    }
    last = list[i];
  }
  return true;
}

int * merge_arrays(int * list1, int size1, int * list2, int size2){
  int * list = new int[size1 + size2];
  int point = 0;
  int i = 0;
  int j = 0;
  
  while(true){
    if(i == size1){
      while(j < size2){
        list[point] = list2[j];
        j++;
        point++;
      }
      break;
    }
    
    if(j == size2){
      while(i < size1){
        list[point] = list1[i];
        i++;
        point++;
      }
      break;
    }
    
    if(list1[i] <= list2[j]){
      list[point] = list1[i];
      i++;
    }
    else{
      list[point] = list2[j];
      j++;
    }
    point++;
  }
  return list;
}

void perform_master(){
  int * list = create_array(SORT_RANGE);
  shuffle_array(list, SORT_RANGE);
  int comm_size = MPI::COMM_WORLD.Get_size();
  MPI_Status status;
  int * sorted_list;
  int * list_packet = new int[PARTITION_SIZE];
  int sorted_list_size = 0;
  int next_to_send = 0;
  
  for(int i = 1; i < comm_size; i++){
    MPI_Send(list + next_to_send, PARTITION_SIZE, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
    next_to_send += PARTITION_SIZE;
    cout << "Node " << status.MPI_SOURCE << " sent initial stock.\n";
  }
  
  while(sorted_list_size < SORT_RANGE){
    MPI_Probe(MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(list_packet, PARTITION_SIZE, MPI_INT, status.MPI_SOURCE, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    sorted_list = merge_arrays(sorted_list, sorted_list_size, list_packet, PARTITION_SIZE);
    if(next_to_send < SORT_RANGE){
      MPI_Send(list + next_to_send, PARTITION_SIZE, MPI_INT, status.MPI_SOURCE, DATA_TAG, MPI_COMM_WORLD);
      next_to_send += PARTITION_SIZE;
      cout << "Node " << status.MPI_SOURCE << " sent restock.\n";
      
    }
    else{
      MPI_Send(list, 1, MPI_INT, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
      cout << "Node " << status.MPI_SOURCE << " sent termination.\n";
    }
  }
  cout << sorted_list[0];
  for(int i = 1; i < SORT_RANGE; i++){
    cout << ", "<< sorted_list[i];
  }
}

void perform_slave(int rank){
  int terminate_flag;
  int data_flag;
  int * data = new int[PARTITION_SIZE];
  long shuffles = 0;
  
  while(true){
    MPI_Iprobe(0, TERMINATE_TAG, MPI_COMM_WORLD, &terminate_flag, MPI_STATUS_IGNORE);
    if(terminate_flag){
      cout << "Node " << rank << " terminated.\n";
      return;
    }
    
    MPI_Iprobe(0, DATA_TAG, MPI_COMM_WORLD, &data_flag, MPI_STATUS_IGNORE);
    if(data_flag){
      cout << "Node " << rank << " restocked.\n";
      shuffles = 0;
      MPI_Recv(data, PARTITION_SIZE, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      while(!check_array(data, PARTITION_SIZE)){
        shuffle_array(data, PARTITION_SIZE);
        shuffles++;
      }
      cout << "Node " << rank << " sorted an array in " << shuffles << " shuffles.\n";
      MPI_Send(data, PARTITION_SIZE, MPI_INT, 0, DATA_TAG, MPI_COMM_WORLD);
    }
  }
}