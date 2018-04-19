#!/bin/bash
# +-------------------------------------------------------------------------
# | Copyright (C) 2018 Yunify, Inc.
# +-------------------------------------------------------------------------
# | Licensed under the Apache License, Version 2.0 (the "License");
# | You may not use this work except in compliance with the License.
# | You may obtain a copy of the License in the LICENSE file, or at:
# |
# | http://www.apache.org/licenses/LICENSE-2.0
# |
# | Unless required by applicable law or agreed to in writing, software
# | distributed under the License is distributed on an "AS IS" BASIS,
# | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# | See the License for the specific language governing permissions and
# | limitations under the License.
# +-------------------------------------------------------------------------
#
#
# test case: write_after_seek_file

set -o xtrace
set -o errexit

current_path=$(dirname "$0")
source "$current_path/utils.sh"

# write after seek ahead
FILE_NAME="write_after_seek_ahead.txt"
FILE_TEST="$RUN_DIR/$FILE_NAME"
dd if=/dev/zero of=$FILE_TEST seek=1 count=1 bs=1024
FILE_SIZE=$(stat -c %s ${FILE_TEST})
if [ FILE_SIZE -ne 2048 ]; then
  echo "Error: expected ${FILE_TEST} has length 2048, got ${FILE_SIZE}"
  exit 1
fi

rm_test_file $FILE_NAME