#!/bin/bash
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#                                                                      #
########################################################################

# Hostname is generated by the mpc configure script
#~ HOSTNAME=${SCTK_MIC_USER}@${SCTK_MIC_HOSTNAME}
HOSTNAME=${SCTK_MIC_HOSTNAME}
WORKDIR_TARGET=/tmp/
WORKDIR_HOST=.
OUTPUT=$1.cross_output

# If a failure occurs
function fail {
  echo "ERROR: an error occured while running the cross_execution script"
  exit 1
}

# Copy from host to target
#echo "Running: scp $1 $HOSTNAME:$WORKDIR_TARGET || fail"
scp $1 $HOSTNAME:$WORKDIR_TARGET || fail

# Run binary
#echo "Running: ssh $HOSTNAME "$WORKDIR_TARGET/$1 > $WORKDIR_TARGET/$OUTPUT" || fail"
ssh $HOSTNAME "LD_LIBRARY_PATH=${SCTK_ARCH_LIBRARY_PATH} $WORKDIR_TARGET/$1 $2 > $WORKDIR_TARGET/$OUTPUT" || fail

# Copy from target to host
#echo "Running: scp $HOSTNAME:$WORKDIR_TARGET/$OUTPUT $WORKDIR_HOST || fail"
scp $HOSTNAME:$WORKDIR_TARGET/$OUTPUT $WORKDIR_HOST || fail

# Print the content of the file to stdout
cat $WORKDIR_HOST/$OUTPUT
rm $WORKDIR_HOST/$OUTPUT

exit 0
