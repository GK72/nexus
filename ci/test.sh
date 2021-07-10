#!/bin/bash

echo -e "\n" > "${NXS_WORKLOG}/data"

"${DEPLOY_PATH}/worklog" start
"${DEPLOY_PATH}/worklog" end

if [[ $(cat "${NXS_WORKLOG}/data" | wc -l) -eq 3 ]]; then
    EXITCODE=0
else
    EXITCODE=1
fi

exit $EXITCODE
