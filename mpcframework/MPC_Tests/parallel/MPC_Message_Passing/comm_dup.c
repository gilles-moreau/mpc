#include <mpc.h>

int main (int argc, char** argv){
mpc_mp_communicator_t comm;
MPC_Comm_dup(SCTK_COMM_WORLD,&comm);
MPC_Comm_dup(SCTK_COMM_WORLD,&comm);
return 0;
}
