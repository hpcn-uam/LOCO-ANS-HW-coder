#!/bin/bash
DEF_COLOR="\e[39m"
MSJ_COLOR="\e[32m"
WAR_COLOR="\e[33m"
ERR_COLOR="\e[31m"
Print_Msg(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${MSJ_COLOR} $@ ${DEF_COLOR}"
}

Print_War(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${WAR_COLOR} $@ ${DEF_COLOR}"
}

Print_Err(){
  time=$(date|awk '{print $4}')
  echo -e " ${time}: ${ERR_COLOR} $@ ${DEF_COLOR}"
}


TARGET_BOARD="pynq_z2"

if ! [[ -z $1  ]]; then
  TARGET_BOARD="$1"
fi
echo "TARGET_BOARD: $TARGET_BOARD"

PROJ_NAME="vivado_${TARGET_BOARD}"
REPO_ROOT=$(git rev-parse --show-toplevel)

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

PROJ_ORIG_DIR="$REPO_ROOT/HW_platforms/LOCO_ANS_LS_1"

if [[ "$SCRIPTPATH" != "$PROJ_ORIG_DIR" ]]; then
  read -r -d '' ERROR_MSJ << EOM
Relative path (with in the repository) of the script has changed. \n
create_project.tcl script may need to be re-generated to
reflect these changes. (check ip repo path)
EOM

  Print_Err  $ERROR_MSJ
  exit 1
fi

mkdir -p ${PROJ_NAME}
cd ${PROJ_NAME}

Print_Msg Calling vivado
vivado -mode batch -source ../create_project.tcl -tclargs \
        --origin_dir ./ \
        --project_name ${PROJ_NAME}  \
        --target_board ${TARGET_BOARD}

Print_Msg End of vivado process